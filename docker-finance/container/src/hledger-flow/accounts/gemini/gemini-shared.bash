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
# Unified format between all file types:
#
#   UID,Date,Type,OrderType,CurrencyOne,CurrencyTwo,Amount,Price,Fees,Destination,TXID,Direction,Subaccount
#

function parse_transfers()
{
  # NOTE: header can be variable, ineffective to assert_header
  # TODO: optimize: do successive test_headers and whichever wins, use that string so `xan select` is avoided
  local _header=""

  if lib_preprocess::test_header "info_feeCurrency"; then

    _header="info_eid,info_timestampms,info_type,info_currency,info_feeCurrency,info_amount,info_feeAmount"

    # On-chain
    lib_preprocess::test_header "info_destination" && _header+=",info_destination"
    lib_preprocess::test_header "info_txHash" && _header+=",info_txHash"

    # Off-chain (withdrawal)
    lib_preprocess::test_header "info_method" && _header+=",info_method"

    xan select "$_header" "$global_in_path" \
      | gawk -v global_year="$global_year" -v global_subaccount="$global_subaccount" -M -v PREC=100 \
        '{
           if (NR<2)
             next

           cmd = "date --utc \"+%F %T\" --date=@$(expr $(echo "$2"/1000 | bc))" | getline date

           if (date !~ global_year)
             next

           # NOTE: If info_method isnt ACH/Wire/SEN then it should be GUSD (exception being CreditCard)
           if ($10 == "ACH" || $10 == "Wire" || $10 == "SEN") {$4="USD"}
           else {gsub(/,USD,/,",GUSD,")}

           # Get/set info_method (if available)
           if ($10 == "ACH" || $10 == "Wire" || $10 == "SEN" || $10 == "CreditCard") {$8=$10}

           printf $1 OFS           # UID (info_eid)
           printf date OFS         # Date
           printf "TRANSFER" OFS   # Type
           printf $3 OFS           # OrderType (info_type)
           printf $4 OFS           # CurrencyOne (info_currency)
           printf $5 OFS           # CurrencyTwo (info_feeCurrency)

           # Amount
           # TODO: getline an gawk remove-zeros script
           # NOTE: .18 requires PREC passed to gawk
           info_amount=sprintf("%.18f", $6)
           if (info_amount ~ /^[0-9]*\.[0-9]+/)
             {
               sub(/0+$/, "", info_amount)
               sub(/\.$/, "", info_amount)
               sub(/^\./, "0.", info_amount)
             }
           printf info_amount OFS

           printf OFS              # Price (N/A)

           # Fees
           info_feeAmount=sprintf("%.18f", $7)
           if (info_feeAmount ~ /^[0-9]*\.[0-9]+/)
             {
               sub(/0+$/, "", info_feeAmount)
               sub(/\.$/, "", info_feeAmount)
               sub(/^\./, "0.", info_feeAmount)
             }
           printf info_feeAmount OFS

           printf OFS             # Cost-basis (N/A)
           printf $8 OFS          # Destination (info_destination)
           printf $9 OFS          # TXID (info_txHash)

           direction=($3 ~ /^Withdrawal$/ ? "OUT" : "IN")
           printf direction OFS

           printf global_subaccount

           printf "\n"

        }' FS=, OFS=, >"$global_out_path"
  else
    # A base string that will work with transfers that are only deposits but not yet withdrawals
    _header="info_eid,info_timestampms,info_type,info_currency,info_amount"

    lib_preprocess::test_header "info_destination" && _header+=",info_destination"
    lib_preprocess::test_header "info_txHash" && _header+=",info_txHash"

    # Off-chain withdrawal
    lib_preprocess::test_header "info_method" && _header+=",info_method"

    xan select "$_header" "$global_in_path" \
      | gawk -v global_year="$global_year" -v global_subaccount="$global_subaccount" -M -v PREC=100 \
        '{
           if (NR<2)
             next

           cmd = "date --utc \"+%F %T\" --date=@$(expr $(echo "$2"/1000 | bc))" | getline date

           if (date !~ global_year)
             next

           # NOTE: If info_method isnt ACH/Wire/SEN then it should be GUSD (exception being CreditCard)
           if ($8 == "ACH" || $8 == "Wire" || $8 == "SEN") {$4="USD"}
           else {gsub(/,USD,/,",GUSD,")}

           # Get/set info_method (if available)
           if ($8 == "ACH" || $8 == "Wire" || $8 == "SEN" || $8 == "CreditCard") {$6=$8}

           printf $1 OFS          # UID (info_eid)
           printf date OFS        # Date
           printf "TRANSFER" OFS  # Type
           printf $3 OFS          # OrderType (info_type)
           printf $4 OFS          # CurrencyOne (info_currency)
           printf OFS             # CurrencyTwo (N/A)

           # Amount
           direction=($5 ~ /^-/ ? "OUT" : "IN")
           sub(/^-/, "", $5)
           # TODO: getline an gawk remove-zeros script
           # NOTE: .18 will need PREC passed to gawk
           info_amount=sprintf("%.18f", $5)
           if (info_amount ~ /^[0-9]*\.[0-9]+/)
             {
               sub(/0+$/, "", info_amount)
               sub(/\.$/, "", info_amount)
               sub(/^\./, "0.", info_amount)
             }
           printf info_amount OFS

           printf OFS             # Price (N/A)
           printf OFS             # Fees (N/A)
           printf OFS             # Cost-basis (N/A)
           printf $6 OFS          # Destination (info_destination)
           printf $7 OFS          # TXID (info_txHash)

           printf direction OFS
           printf global_subaccount

           printf "\n"

        }' FS=, OFS=, >"$global_out_path"
  fi
}

function parse_trades()
{
  # TODO: Gemini bug? In rare cases, Gemini may include client_order_id ("echo back")
  # even though docker-finance never supplies an identifier nor place any orders.
  # In all cases, this column will be ignored but must also allow the header assertion to pass.
  local _client_order_id=""
  lib_preprocess::test_header "client_order_id" && _client_order_id=",client_order_id"

  lib_preprocess::assert_header "price,amount,timestamp,timestampms,type,aggressor,fee_currency,fee_amount,tid,order_id,exchange,is_auction_fill,is_clearing_fill,symbol${_client_order_id}"

  # Get symbol from base pair trade. Example: bchbtc-Trades.csv will return bch
  local _account_currency
  _account_currency="$(echo $global_in_filename | cut -d"-" -f1 | sed 's:...$::')"

  gawk --csv \
    -v account_currency="$_account_currency" \
    -v global_year="$global_year" \
    -v global_subaccount="$global_subaccount" \
    '{
       if (NR<2)
         next

       cmd = "date --utc \"+%F %T\" --date=@$(expr $(echo "$4"/1000 | bc))" | getline date

       if (date !~ global_year)
         next

       printf $9 OFS                # UID (tid)
       printf date OFS              # Date
       printf "TRADE" OFS           # Type
       printf "\"" $5 "\"" OFS      # OrderType (type)

       # CurrencyOne (account_currency)
       printf "\"" toupper(account_currency) "\"" OFS

       # CurrencyTwo (fee_currency)
       printf "\"" $7 "\"" OFS

       # TODO: getline an gawk remove-zeros script
       amount=sprintf("%.8f", $2)
       if (amount ~ /^[0-9]*\.[0-9]+/)
         {
           sub(/0+$/, "", amount)
           sub(/\.$/, "", amount)
           sub(/^\./, "0.", amount)
         }
       printf amount OFS  # Amount

       cost=sprintf("%.8f", $1*$2)
       if (cost ~ /^[0-9]*\.[0-9]+/)
         {
           sub(/0+$/, "", cost)
           sub(/\.$/, "", cost)
           sub(/^\./, "0.", cost)
         }
       printf cost OFS  # Price  (will be used as a "total" for @@, not price)

       fee_amount=sprintf("%.8f", $8)
       if (fee_amount ~ /^[0-9]*\.[0-9]+/)
         {
           sub(/0+$/, "", fee_amount)
           sub(/\.$/, "", fee_amount)
           sub(/^\./, "0.", fee_amount)
         }
       printf fee_amount OFS  # Fees

       # TODO: HACK: see #51 and respective lib_taxes work-around
       # Calculate cost-basis
       switch ($5)
          {
            case "Buy":
              cost_basis=sprintf("%.8f", cost + fee_amount)
              break
            case "Sell":
              cost_basis=sprintf("%.8f", cost - fee_amount)
              break
            default:
              printf "FATAL: unsupported order type: " $5
              print $0
              exit
          }
       printf cost_basis OFS

       printf OFS      # Destination (N/A)
       printf $10 OFS  # TXID (tid)

       # Direction
       printf OFS

       printf global_subaccount

       printf "\n"

    }' OFS=, "$global_in_path" >"$global_out_path"
}

function parse_earn()
{
  lib_preprocess::assert_header "earnTransactionId,transactionId,transactionType,amountCurrency,amount,priceCurrency,priceAmount,dateTime"

  gawk --csv \
    -v global_year="$global_year" \
    -v global_subaccount="$global_subaccount" \
    -M -v PREC=100 \
    '{
       if (NR<2)
         next

       cmd = "date --utc \"+%F %T\" --date=@$(expr $(echo "$8"/1000 | bc))" | getline date

       if (date !~ global_year)
         next

       printf $1 OFS          # UID (earnTransactionId)
       printf date OFS        # Date
       printf "INTEREST" OFS  # Type
       printf "EARN" OFS      # OrderType (type)
       printf "\"" $4 "\""OFS # CurrencyOne (amountCurrency)
       printf OFS             # CurrencyTwo

       # Amount
       # TODO: getline an gawk remove-zeros script
       # NOTE: .22 requires PREC passed to gawk
       amount=sprintf("%.22f", $5)
       if (amount ~ /^[0-9]*\.[0-9]+/)
         {
           sub(/0+$/, "", amount)
           sub(/\.$/, "", amount)
           sub(/^\./, "0.", amount)
         }
       printf amount OFS

       # Price
       priceAmount=sprintf("%.22f", $7*$5)
       if (priceAmount ~ /^[0-9]*\.[0-9]+/)
         {
           sub(/0+$/, "", priceAmount)
           sub(/\.$/, "", priceAmount)
           sub(/^\./, "0.", priceAmount)
         }
       printf priceAmount OFS

       printf OFS  # Fees
       printf OFS  # Cost-basis
       printf OFS  # Destination
       printf OFS  # TXID

       # Direction
       printf "IN" OFS

       printf global_subaccount

       printf "\n";

    }' OFS=, "$global_in_path" >"$global_out_path"
}

function parse_card()
{
  lib_preprocess::assert_header "Reference Number,Transaction Post Date,Description of Transaction,Transaction Type,Amount"

  gawk --csv \
    -v global_year="$global_year" \
    -v global_subaccount="$global_subaccount" \
    '{
       if (NR<2)
         next

       # Date format is MM/DD/YY
       yy=substr($2, 7, 2)
       global_yy=substr(global_year, 3, 2)

       if (yy != global_yy)
         next

       printf $1 OFS  # Reference Number
       printf $2 OFS  # Transaction Post Date

       # Description of Transaction
       gsub(/%/, "%%", $3)
       gsub(/"/, "", $3)
       printf "\"" $3 "\"" OFS

       printf "\"" $4 "\"" OFS  # Transaction Type
       printf $5 OFS  # Amount

       direction=($5 ~ /^-/ ? "IN" : "OUT")
       printf direction OFS

       printf global_subaccount

       printf "\n";

    }' OFS=, "$global_in_path" >"$global_out_path"
}

function main()
{
  # Transfers
  lib_preprocess::test_header "info_eid" && parse_transfers && return 0

  # Trades (may come in either form)
  lib_preprocess::test_header "tid" && parse_trades && return 0
  lib_preprocess::test_header "info_tid" && parse_trades && return 0

  # Earn
  lib_preprocess::test_header "earnTransactionId" && parse_earn && return 0

  # Card
  lib_preprocess::test_header "Reference Number" && parse_card && return 0
}

main "$@"

# vim: sw=2 sts=2 si ai et
