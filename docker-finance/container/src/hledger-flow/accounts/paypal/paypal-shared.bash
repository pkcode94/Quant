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

#
# Unified output format for all file types:
#
#   timestamp,description,type,status,currency,amount,code,balance,direction,subaccount,in_value,in_ticker,out_value,out_ticker,tx_fee_value,tx_fee_ticker,fiat_value
#

function parse_report()
{
  # Supports "Balance Affecting" reports
  lib_preprocess::assert_header "\"Date\",\"Time\",\"TimeZone\",\"Name\",\"Type\",\"Status\",\"Currency\",\"Amount\",\"Fees\",\"Total\",\"Exchange Rate\",\"Receipt ID\",\"Balance\",\"Transaction ID\",\"Item Title\""

  # Paypal is known to allow commas within description ("Name") and amounts
  # NOTE: using custom timezone offset instead of provided timezone
  xan select "Date,Time,Name,Type,Status,Currency,Amount,Fees,Total,Exchange Rate,Receipt ID,Balance,Transaction ID,Item Title" "$global_in_path" \
    | gawk --csv \
      -v global_year="$global_year" \
      -v global_subaccount="$global_subaccount" \
      '{
         if (NR<2 || $1 !~ global_year)
           next

         # timestamp format must be:
         #
         #   MM/DD/YYYY HH:MM:SS -> YYYY-MM-DD HH:MM:SS
         #
         mm=substr($1, 1, 2)
         dd=substr($1, 4, 2)
         yyyy=substr($1, 7, 4)

         date_time = yyyy "-" mm "-" dd "T" $2

         # Scrap existing alphanum timezone for a timezone offset
         cmd = "date \"+%F %T %z\" --date="date_time | getline timestamp

         # Print
         printf timestamp OFS  # timestamp

         # description (Name)
         sub(/%/, "%%", $3)
         gsub(/"/, "", $3)
         printf "\"" $3 "\"" OFS

         printf "\"" $4 "\"" OFS  # type
         printf "\"" $5 "\"" OFS  # status
         printf "\"" $6 "\"" OFS  # currency

         # amount
         direction=($7 ~ /^-/ ? "OUT" : "IN")
         sub(/^-/, "", $7)
         gsub(/,/, "", $7)
         printf "\"" $7 "\"" OFS

         # fees
         sub(/^-/, "", $8)
         gsub(/,/, "", $8)
         printf "\"" $8 "\"" OFS

         # total
         sub(/^-/, "", $9)
         gsub(/,/, "", $9)
         printf "\"" $9 "\"" OFS

         # exchange rate
         gsub(/,/, "", $10)
         printf "\"" $10 "\"" OFS

         printf "\"" $11 "\"" OFS  # code (Receipt ID)
         printf "\"" $12 "\"" OFS  # balance

         printf $13 OFS            # txid (Transaction ID)

         # title (Item Title)
         sub(/%/, "%%", $14)
         gsub(/"/, "", $14)
         printf "\"" $14 "\"" OFS

         printf direction OFS
         printf global_subaccount OFS

         printf OFS  # in_value
         printf OFS  # in_ticker
         printf OFS  # out_value
         printf OFS  # out_ticker
         printf OFS  # tx_fee_value
         printf OFS  # tx_fee_ticker
                     # fiat_value

         printf "\n"
      }' OFS=, >"$global_out_path"
}

# NOTE: crypto transactions CSV, not crypto statement CSV
function parse_crypto()
{
  # NOTE: skipping comments, header begins at line 4
  # TODO: assert_header needs to work with spaces
  #lib_preprocess::assert_header "DateTime,Transaction Type,Asset In (Quantity),Asset In (Currency),Asset Out (Quantity),Asset Out (Currency),Transaction Fee (Quantity),Transaction Fee (Currency),Market Value (USD)" "$(sed -n '4p' $global_in_path)"

  # TODO: refactor into gawk after assert_header is fixed
  xan select "DateTime,Transaction Type,Asset In (Quantity),Asset In (Currency),Asset Out (Quantity),Asset Out (Currency),Transaction Fee (Quantity),Transaction Fee (Currency),Market Value (USD)" <(tail +4 "$global_in_path") \
    | gawk --csv \
      -v global_year="$global_year" \
      -v global_subaccount="$global_subaccount" \
      '{
         if (NR<2 || $1 !~ global_year)
           next

         # Cleanup DateTime
         sub(/\.[[:alnum:]]*/, " ", $1);

         # Gets timezone (offset)
         cmd = "date \"+%F %T %z\" --date="$1 | getline timestamp

         printf timestamp OFS  # timestamp
         printf "Crypto" OFS   # description (Name)
         printf $2 OFS         # type
         printf OFS            # status
         printf OFS            # currency
         printf OFS            # amount
         printf OFS            # fees
         printf OFS            # total
         printf OFS            # exchange rate
         printf OFS            # code (Receipt ID)
         printf OFS            # balance
         printf OFS            # txid (Transaction ID)
         printf OFS            # title (Item Title)

         printf OFS            # direction
         printf global_subaccount OFS

         printf $3 OFS            # in_value
         printf "\"" $4 "\"" OFS  # in_ticker

         printf $5 OFS            # out_value
         printf "\"" $6 "\"" OFS  # out_ticker

         printf $7 OFS            # tx_fee_value
         printf "\"" $8 "\"" OFS  # tx_fee_ticker

         printf $9      # fiat_value

         printf "\n"
      }' OFS=, >"$global_out_path"
}

function main()
{
  # Crypto transactions will have commentary in the first line
  lib_preprocess::test_header "\"Date\"" \
    && parse_report \
    || parse_crypto
  return $?
}

main "$@"

# vim: sw=2 sts=2 si ai et
