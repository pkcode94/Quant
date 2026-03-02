#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2021-2025 Aaron Fiore (Founder, Evergreen Crypto LLC)
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
#   UID,Date,Type,OrderType,CurrencyOne,CurrencyTwo,Amount,Proceeds,Fees,Destination,TXID,Direction,Subaccount
#

function parse_deposit()
{
  # NOTE: headers can be variable, not possible to assert
  xan select "id,completedAt,currencySymbol,quantity,cryptoAddress,txId" "$global_in_path" \
    | gawk -v global_year="$global_year" -v global_subaccount="$global_subaccount" \
      '{
         if (NR<2 || $2 !~ global_year)
           next

         cmd = "date --utc \"+%F %T\" --date="$2 | getline date

         printf $1 OFS                   # UID
         printf date OFS                 # Date
         printf "DEPOSIT" OFS            # Type
         printf OFS                      # OrderType
         printf $3 OFS                   # CurrencyOne
         printf OFS                      # CurrencyTwo
         printf("%.8f", $4); printf OFS  # Amount
         printf OFS                      # Proceeds
         printf OFS                      # Fees
         printf OFS                      # Cost-basis
         printf $5 OFS                   # Destination
         printf $6 OFS                   # TXID
         printf "IN" OFS                 # Direction
         printf global_subaccount        # Subaccount

         printf "\n"

      }' FS=, OFS=, >"$global_out_path"
}

function parse_trade()
{
  # NOTE:
  #  - Commissions are paid in base currency (one on the left)
  #  - Proceeds are divided by quantity.
  #    WARNING: don't use limit because market orders will be empty

  # NOTE: headers can be variable, not possible to assert
  xan select "id,closedAt,direction,marketSymbol,fillQuantity,proceeds,commission" "$global_in_path" \
    | gawk -v global_year="$global_year" -v global_subaccount="$global_subaccount" \
      '{
         if (NR<2 || $2 !~ global_year)
           next

         cmd = "date --utc \"+%F %T\" --date="$2 | getline date

         # For trades, Bittrex doesnt create separate columns for pairs.
         # So, turn BTC-USD into two separate columns.
         sub(/-/, ",", $4)

         # TODO: HACK: see #51 and respective lib_taxes work-around
         # Calculate cost-basis
         switch ($3)
           {
             case "BUY":
               cost_basis=sprintf("%.8f", $6 + $7)
               break
             case "SELL":
               cost_basis=sprintf("%.8f", $6 - $7)
               break
             default:
               printf "FATAL: unsupported order type: " $3
               print $0
               exit
           }

         printf $1 OFS                   # UID
         printf date OFS                 # Date
         printf "TRADE" OFS              # Type
         printf $3 OFS                   # OrderType
         printf $4 OFS                   # CurrencyOne
         # see above                     # CurrencyTwo
         printf("%.8f", $5); printf OFS  # Amount
         printf("%.8f", $6); printf OFS  # Proceeds
         printf("%.8f", $7); printf OFS  # Fees
         printf cost_basis OFS           # Cost-basis
         printf OFS                      # Destination
         printf OFS                      # TXID
         printf OFS                      # Direction
         printf global_subaccount        # Subaccount

         printf "\n"

      }' FS=, OFS=, >"$global_out_path"
}

function parse_withdrawal()
{
  # NOTE: headers can be variable, not possible to assert
  xan select "id,completedAt,currencySymbol,quantity,txCost,cryptoAddress,txId" "$global_in_path" \
    | gawk -v global_year="$global_year" -v global_subaccount="$global_subaccount" \
      '{
         if (NR<2 || $2 !~ global_year)
           next

         cmd = "date --utc \"+%F %T\" --date="$2 | getline date

         printf $1 OFS                   # UID
         printf date OFS                 # Date
         printf "WITHDRAWAL" OFS         # Type
         printf OFS                      # OrderType
         printf $3 OFS                   # CurrencyOne
         printf OFS                      # CurrencyTwo
         printf("%.8f", $4); printf OFS  # Amount
         printf OFS                      # Proceeds
         printf("%.8f", $5); printf OFS  # Fees
         printf OFS                      # Cost-basis
         printf $6 OFS                   # Destination
         printf $7 OFS                   # TXID
         printf "OUT" OFS                # Direction
         printf global_subaccount        # Subaccount

         printf "\n"

      }' FS=, OFS=, >"$global_out_path"
}

function main()
{
  # REST API provided files
  lib_preprocess::test_header "source" && parse_deposit && return 0
  lib_preprocess::test_header "direction" && parse_trade && return 0
  lib_preprocess::test_header "txCost" && parse_withdrawal && return 0
}

main "$@"

# vim: sw=2 sts=2 si ai et
