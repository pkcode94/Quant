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

#
# Unified format between all file types:
#
#   UID,Date,Type,SubType,CurrencyOne,CurrencyOneAmount,CurrencyTwo,CurrencyTwoAmount,RateCurrency,RateAmount,Direction,Subaccount
#

# TODO: fees are in transaction type, be sure to include that in rules file
function parse_transaction()
{
  lib_preprocess::assert_header "Cryptocurrency,Amount,Transaction Type,Confirmed At"

  gawk -v global_year="$global_year" -v global_subaccount="$global_subaccount" \
    '{
       if (NR<2 || $4 !~ global_year)
         next

       # Trades are included in transactions... including ACH (USD) trades...
       UID=""; if ($1 ~ /^USD$/ || $3 ~ /Trade$/) {UID="SKIP"} # NOTE: Trade is not ^ because there are multiple Trade types

       # NOTE: BIA "Withdrawals" are not real withdrawals, so they are left positive
       amount = $2

       direction="IN"
       if (amount ~ /^-/) {direction="OUT"}  # WARNING: anchor "-" at beginning of line or else E-N amounts will be broken

       # Remove sign for printing
       sub(/^-/, "", amount)

       printf UID OFS                      # UID
       printf $4 OFS                       # Date
       printf "TRANSACTION" OFS            # Type
       printf $3 OFS                       # SubType
       printf $1 OFS                       # CurrencyOne
       printf("%.8f", amount); printf OFS  # CurrencyOneAmount
       printf OFS                          # CurrencyTwo
       printf OFS                          # CurrencyTwoAmount
       printf OFS                          # RateCurrency
       printf OFS                          # RateAmount

       printf direction OFS                # Direction
       printf global_subaccount            # Subaccount

       printf "\n"

    }' FS=, OFS=, "$global_in_path" >"$global_out_path"

  # If out file has no transactions (and only trades), such as at the beginning
  # of the year, it will create empty file in 2-preprocessed. The following hack
  # exists because hledger-flow won't ignore deleted 1-in and 2-preprocessed
  # files after this script has been called.
  #
  # WARNING: must be also applied in rules file
  if [ ! -s "$global_out_path" ]; then
    echo "SKIP,date,type,sub_type,currency_one,currency_one_amount,currency_two,currency_two_amount,rate_currency,rate_amount" >"$global_out_path"
  fi
}

function parse_trade()
{
  lib_preprocess::assert_header "Trade ID,Date,Buy Quantity,Buy Currency,Sold Quantity,Sold Currency,Rate Amount,Rate Currency,Type,Frequency,Destination"

  gawk -v global_year="$global_year" -v global_subaccount="$global_subaccount" \
    '{
       if (NR<2 || $2 !~ global_year)
         next

       printf $1 OFS                   # UID
       printf $2 OFS                   # Date
       printf $9 OFS                   # Type
       printf $10 OFS                  # SubType
       printf toupper($4); printf OFS  # CurrencryOne
       printf("%.8f", $3); printf OFS  # CurrencyOneAmount
       printf toupper($6); printf OFS  # CurrencyTwo
       printf("%.8f", $5); printf OFS  # CurrencyTwoAmount
       printf toupper($8); printf OFS  # RateCurrency
       printf("%.8f", $7); printf OFS  # RateAmount

       printf OFS                      # Direction
       printf global_subaccount        # Subaccount

       printf "\n";

    }' FS=, OFS=, "$global_in_path" >"$global_out_path"
}

function main()
{
  lib_preprocess::test_header "Confirmed At" && parse_transaction && return 0
  lib_preprocess::test_header "Trade ID" && parse_trade && return 0
}

main "$@"

# vim: sw=2 sts=2 si ai et
