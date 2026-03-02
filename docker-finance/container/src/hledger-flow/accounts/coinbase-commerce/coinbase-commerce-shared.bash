#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2021-2024 Aaron Fiore (Founder, Evergreen Crypto LLC)
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

function parse()
{
  # NOTE: skipping comments, header begins at line 7
  # TODO: this breaks because of spaces(?)
  #lib_preprocess::assert_header "TRANSACTION COMPLETED,TRANSACTION INITIATED,TRANSACTION TYPE,PRODUCT NAME OR INVOICE ID,AMOUNT REQUESTED,TRANSACTION ID CODE,STATUS,ASSET USED FOR PAYMENT,EXCHANGE RATE AT TIME OF TRANSACTION (USD-CRYPTO ASSET),SUBTOTAL IN CRYPTO,SUBTOTAL IN FIAT,ASSET USED FOR FEES,FEE EXCHANGE RATE AT TIME OF TRANSACTION (USD-CRYPTO ASSET),COINBASE FEES IN CRYPTO,COINBASE FEES IN FIAT,NETWORK FEES IN CRYPTO,NETWORK FEES IN FIAT,CONVERSION STATUS,CONVERSION EXCHANGE RATE (USD-CRYPTO ASSET),CONVERTED VALUE IN FIAT,RECEIVER ADDRESS,ETHEREUM HOMESTEAD ADDRESS,METADATA,SYSTEM ID,TRANSACTION HASH" "$(sed -n '7p' $global_in_path)"

  gawk -v global_year="$global_year" -v global_subaccount="$global_subaccount" \
    '{
       if (NR>7)
         {
           if ($2 ~ global_year)
             {
               # Cleanup extra quotations
               gsub(/""/, "")

               # Cleanup metadata column
               gsub(/, /, ";")

               # Cleanup the time
               sub(/ UTC/, "", $2)

               # $ added later
               sub(/[$]/, "", $5)
               sub(/[$]/, "", $11)

               # Coinbase decided to put the asset ticker within the various
               #   crypto amount columns instead of using only numbers...
               sub(/ [[:alnum:]]*/, "", $10)
               sub(/ [[:alnum:]]*/, "", $14)
               sub(/ [[:alnum:]]*/, "", $16)

               printf $1 OFS   # tx_completed (TRANSACTION COMPLETED)
               printf $2 OFS   # tx_initiated (TRANSACTION INITIATED)
               printf $3 OFS   # tx_type (TRANSACTION TYPE)
               printf $4 OFS   # pid (PRODUCT NAME OR INVOICE ID)
               printf $5 OFS   # amount_requested (AMOUNT REQUESTED)
               printf $6 OFS   # tid_code (TRANSACTION ID CODE)
               printf $7 OFS   # status_ (STATUS)
               printf $8 OFS   # payment_asset (ASSET USED FOR PAYMENT)
               printf $9 OFS   # payment_exchange_rate (USD-CRYPTO ASSET)
               printf $10 OFS  # subtotal_crypto (SUBTOTAL IN CRYPTO)
               printf $11 OFS  # subtotal_fiat (SUBTOTAL IN FIAT)
               printf $12 OFS  # fee_asset (ASSET USED FOR FEES)
               printf $13 OFS  # fee_exchange_rate (USD-CRYPTO ASSET)
               printf $14 OFS  # coinbase_fee_crypto (COINBASE FEES IN CRYPTO)
               printf $15 OFS  # coinbase_fee_fiat (COINBASE FEES IN FIAT)
               printf $16 OFS  # network_fee_crypto (NETWORK FEES IN CRYPTO)
               printf $17 OFS  # network_fee_fiat (NETWORK FEES IN FIAT)
               printf $18 OFS  # conversion_status (CONVERSION STATUS)
               printf $19 OFS  # conversion_exchange_rate (USD-CRYPTO ASSET)
               printf $20 OFS  # conversion_fiat_value (CONVERTED VALUE IN FIAT)
               printf $21 OFS  # receiver_address (RECEIVER ADDRESS)
               printf $22 OFS  # eth_homestead (ETHEREUM HOMESTEAD ADDRESS)
               printf $23 OFS  # metadata (METADATA)
               printf $24 OFS  # system_id (SYSTEM ID)
               printf $25 OFS  # txid (TRANSACTION HASH)

               # Direction
               printf ($3 ~ /^Withdrawal$/ ? "OUT" : "IN") OFS

               printf global_subaccount

               printf "\n";
             }
         }
    }' FS=, OFS=, "$global_in_path" >"$global_out_path"
}

function main()
{
  parse
}

main "$@"

# vim: sw=2 sts=2 si ai et
