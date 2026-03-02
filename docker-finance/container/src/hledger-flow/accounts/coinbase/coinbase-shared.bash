#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2021-2026 Aaron Fiore (Founder, Evergreen Crypto LLC)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.

#
# "Libraries"
#

[ -z "$DOCKER_FINANCE_CONTAINER_REPO" ] && exit 1
source "${DOCKER_FINANCE_CONTAINER_REPO}/src/hledger-flow/lib/lib_preprocess.bash" "$1" "$2"

#
# Implementation
#

[ -z "$global_year" ] && exit 1
[ -z "$global_subaccount" ] && exit 1

[ -z "$global_in_path" ] && exit 1
[ -z "$global_out_path" ] && exit 1

[ -z "$global_in_filename" ] && exit 1

#
# Notes regarding this implementation:
#
#  - Input data MUST be provided by Coinbase SIWC V2 REST API (via `fetch`).
#
#  - Output data will consist of a "universal" CSV format for hledger rules.
#

function add_to_headers()
{
  [ -z "$universal_header" ] && exit 1
  [ -z "$selected_header" ] && exit 1

  local _column_name="$1"
  [ -z "$_column_name" ] && exit 1

  universal_header+=",${_column_name}"
  lib_preprocess::test_header "$_column_name" \
    && selected_header+=",${_column_name}"
}

function join_to_header()
{
  # WARNING: csvjoin *MUST* use -I or else satoshis like 0.00000021 will turn into 21000000 BTC!
  # TODO: will `xan join` be faster and produce the same results?
  csvjoin -I --snifflimit 0 "$1" <(echo "$2")
}

#
# Internal implementation for forming "universal" CSV stream
#

function __parse()
{
  gawk --csv \
    -v global_year="$global_year" \
    -v global_subaccount="$global_subaccount" \
    '{
       if (NR<2 || $9 !~ global_year)
         next

       # Remove `-` for calculations (must re-add in rules)
       direction=($5 ~ /^-/ ? "OUT" : "IN")
       sub(/^-/, "", $5)
       sub(/^-/, "", $7)

       # Cleanup timestamp
       sub(/T/, " ", $9);
       sub(/Z/, "", $9);
       sub(/\+.*/, "", $9);

       # Print [info] object for rules consumption
       printf $1 OFS                    # account_id (prepended column)
       printf $2 OFS                    # info_id (coinbase_id)
       printf $3 OFS                    # info_type
       printf $4 OFS                    # info_status
       printf $5 OFS                    # info_amount_amount
       printf "\"" $6 "\"" OFS          # info_amount_currency
       printf $7 OFS                    # info_native_amount_amount
       printf "\"" $8 "\"" OFS          # info_native_amount_currency
       printf $9 OFS                    # info_created_at
       printf $10 OFS                   # info_resource
       printf $11 OFS                   # info_resource_path
       printf $12 OFS                   # info_description
       printf $13 OFS                   # info_network_status
       printf "\"" $14 "\"" OFS         # info_network_network_name
       printf $15 OFS                   # info_network_hash (txid)
       printf("%.8f", $16); printf OFS  # info_network_transaction_fee_amount
       printf "\"" $17 "\"" OFS         # info_network_transaction_fee_currency
       printf $18 OFS                   # info_to_resource
       printf $19 OFS                   # info_to_address
       printf $20 OFS                   # info_to_email
       printf $21 OFS                   # info_from_resource
       printf $22 OFS                   # info_from_resource_path
       printf $23 OFS                   # info_from_id
       printf $24 OFS                   # info_from_name
       printf $25 OFS                   # info_cancelable
       printf $26 OFS                   # info_idem
       printf("%.8f", $27); printf OFS  # info_buy_total_amount
       printf "\"" $28 "\"" OFS         # info_buy_total_currency
       printf("%.8f", $29); printf OFS  # info_buy_subtotal_amount
       printf "\"" $30 "\"" OFS         # info_buy_subtotal_currency
       printf("%.8f", $31); printf OFS  # info_buy_fee_amount
       printf "\"" $32 "\"" OFS         # info_buy_fee_currency
       printf $33 OFS                   # info_buy_id

       # info_buy_payment_method_name
       gsub(/"/, "", $34);
       printf "\"" $34 "\"" OFS

       printf("%.8f", $35); printf OFS  # info_sell_total_amount
       printf "\"" $36 "\"" OFS         # info_sell_total_currency
       printf("%.8f", $37); printf OFS  # info_sell_subtotal_amount
       printf "\"" $38 "\"" OFS         # info_sell_subtotal_currency
       printf("%.8f", $39); printf OFS  # info_sell_fee_amount
       printf "\"" $40 "\"" OFS         # info_sell_fee_currency
       printf $41 OFS                   # info_sell_id

       # info_sell_payment_method_name
       gsub(/"/, "", $42);
       printf "\"" $42 "\"" OFS

       printf("%.8f", $43); printf OFS  # info_trade_fee_amount
       printf "\"" $44 "\"" OFS         # info_trade_fee_currency
       printf $45 OFS                   # info_trade_id

       # info_trade_payment_method_name
       gsub(/"/, "", $46);
       printf "\"" $46 "\"" OFS

       printf("%.8f", $47); printf OFS  # info_advanced_trade_fill_fill_price
       printf $48 OFS                   # info_advanced_trade_fill_product_id
       printf $49 OFS                   # info_advanced_trade_fill_order_id
       printf("%.8f", $50); printf OFS  # info_advanced_trade_fill_commission
       printf $51 OFS                   # info_advanced_trade_fill_order_side

       #
       # Add new columns to calculate fees against native currency price
       #
       # WARNING:
       #
       #  - `info_amount_amount` and/or `info_native_amount_amount` may be
       #    given as 0 because Coinbase does not display the full value of
       #    either column for token values (satoshi, gwei, etc.) valued less
       #    than a $0.00.
       #
       #    Quite literally, the only value information that Coinbase will
       #    provide for these transactions is 0 or equivalent; so, an accurate
       #    calculation cannot be made nor inferred because the only other
       #    usable information is the fee. But, since it is unknown what
       #    percentage the fee-tier is for these transactions, no further
       #    calculations can be made.
       #
       #    With that said, any 0-value transactions should skip the following
       #    calculations as to avoid a divide by zero error and, instead, be
       #    managed within the rules file.
       #

       if ($7 > 0 && $5 > 0) {printf("%.8f", $7 / $5)}; printf OFS           # native_amount_price
       if ($7 > 0 && $5 > 0) {printf("%.8f", ($7 / $5) * $16)}; printf OFS   # native_network_transaction_fee_amount

       #
       # Add new column to calculate the difference between amount and network fee,
       #   i.e., amount_amount - network_transaction_fee_amount
       #

       if ($16 > 0) {printf("%.8f", $5 - $16)}; printf OFS  # network_transaction_amount_amount

       #
       # Advanced Trade: add new column for calculating real value amount
       #
       #  - Works with any pairing (not only fiat)
       #

       # Multiply amount_amount by advanced_trade_fill_fill_price
       real_value_amount = $5 * $47
       printf("%.8f", real_value_amount); printf OFS  # advanced_trade_fill_real_value_amount

       #
       # Advanced Trade: add new columns for trade pairing
       #

       split($48, pair, "-");  # Pair exists as advanced_trade_fill_product_id
       printf "\"" pair[1] "\"" OFS  # advanced_trade_fill_pair_lhs (left-hand side of the pair)
       printf "\"" pair[2] "\"" OFS  # advanced_trade_fill_pair_rhs (right-hand side of the pair)

       #
       # Advanced Trade: add column for cost-basis (comment2)
       #

       # NOTE: sale/proceeds (real_value_amount) will have the fee removed by default
       cost_basis=(direction ~ /^IN$/ ? real_value_amount + $50 : real_value_amount)
       printf("%.8f", cost_basis); printf OFS  # advanced_trade_fill_cost_basis_amount

       printf direction OFS
       printf global_subaccount

       printf "\n";

    }' OFS=,
}

#
# Notes regarding the parsing process:
#
#   0. Since given headers may be variable, it's not possible to assert a single header.
#
#       - `fetch`-provided input CSVs are a beast. Headers are inconsistent and complex.
#
#   1. Create a "selected" header which describes the given header (the header given by `fetch`).
#
#   2. Select all of the columns in said "selected" header.
#
#   3. Join the "universal" header (which contains the entirety of all possible entries)
#
#       - Input streams with duplicate header columns will be joined with a new column name (e.g., `col2` instead of `col`).
#
#   4. Do a final select on input stream using the "universal" header.
#
#       - This will only select the correct columns while also removing duplicates (e.g., said `col2`)
#
#   5. Have a drink while contemplating life.
#

function parse()
{
  #
  # Create account ID as first column, followed by TXID
  #

  # WARNING: Existing filename *MUST* have the following format (where X is account ID):
  #
  #   XXXXXXXXXXXXX-XXXX-XXXX-XXXXXXXXXXXX_transactions.csv
  #

  # TODO: fetch/preprocess: Coinbase API provides `account` type `vault`.
  #   This can be prepended to an account file so it's known that it's a vault
  #   (versus a wallet, which can also be prepended).

  local -r _account_id="${global_in_filename:0:-17}"

  #
  # Reconstruct [info] object (raw ledger) with "universal" header
  #

  # Required Coinbase schema, per documentation, that appears to exist amongst all entries
  declare -g universal_header="info_id,info_type,info_status,info_amount_amount,info_amount_currency,info_native_amount_amount,info_native_amount_currency,info_created_at,info_resource,info_resource_path"

  # Selected header will always have minimum requirements
  declare -g selected_header+="$universal_header"

  # `description`
  add_to_headers "info_description"

  # `network`
  add_to_headers "info_network_status"
  add_to_headers "info_network_network_name"
  add_to_headers "info_network_hash"
  add_to_headers "info_network_transaction_fee_amount"
  add_to_headers "info_network_transaction_fee_currency"

  # `to`
  add_to_headers "info_to_resource"
  add_to_headers "info_to_address"
  add_to_headers "info_to_email"

  # `from`
  add_to_headers "info_from_resource"
  add_to_headers "info_from_resource_path"
  add_to_headers "info_from_id"
  add_to_headers "info_from_name"

  # Remaining SEND related
  add_to_headers "info_cancelable"
  add_to_headers "info_idem"

  # `buy`
  add_to_headers "info_buy_total_amount"
  add_to_headers "info_buy_total_currency"
  add_to_headers "info_buy_subtotal_amount"
  add_to_headers "info_buy_subtotal_currency"
  add_to_headers "info_buy_fee_amount"
  add_to_headers "info_buy_fee_currency"
  add_to_headers "info_buy_id"
  add_to_headers "info_buy_payment_method_name"

  # `sell`
  add_to_headers "info_sell_total_amount"
  add_to_headers "info_sell_total_currency"
  add_to_headers "info_sell_subtotal_amount"
  add_to_headers "info_sell_subtotal_currency"
  add_to_headers "info_sell_fee_amount"
  add_to_headers "info_sell_fee_currency"
  add_to_headers "info_sell_id"
  add_to_headers "info_sell_payment_method_name"

  # `trade`
  add_to_headers "info_trade_fee_amount"
  add_to_headers "info_trade_fee_currency"
  add_to_headers "info_trade_id"
  add_to_headers "info_trade_payment_method_name"

  # `advanced_trade_fill`
  # Note: Coinbase appears to always present this in an ordered set
  add_to_headers "info_advanced_trade_fill_fill_price"
  add_to_headers "info_advanced_trade_fill_product_id"
  add_to_headers "info_advanced_trade_fill_order_id"
  add_to_headers "info_advanced_trade_fill_commission"
  add_to_headers "info_advanced_trade_fill_order_side"

  #
  # Finalize the "universal" format and parse
  #

  # NOTE: prepends account_id to header (this will now be the first column)

  xan select "$selected_header" "$global_in_path" \
    | join_to_header - "$universal_header" \
    | xan select "$universal_header" \
    | sed -e "s:^:${_account_id},:g" -e "1 s:^${_account_id},:account_id,:g" \
    | __parse >"$global_out_path"
}

function main()
{
  parse
}

main "$@"

# vim: sw=2 sts=2 si ai et
