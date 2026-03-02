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

# TODO: optimize

# NOTE: 18 decimal places (as provided) are used mostly for
# accurate ETH postings but is also useful all-around

# App UI/Web UI provided file
function parse_ui()
{
  lib_preprocess::assert_header "Internal id, Date and time, Transaction type, Coin type, Coin amount, USD Value, Original Reward Coin, Reward Amount In Original Coin, Confirmed"

  xan search -s " Date and time" "$global_year" "$global_in_path" \
    | sed -r 's/ ([0-9]+),/ \1/' \
    | gawk -v global_subaccount="$global_subaccount" -M -v PREC=100 \
      '{
         if (NR<2)
           next

         cmd = "date \"+%F %T\" --date="$2 | getline date

         printf $1 OFS                    # Internal id
         printf date OFS                  # Date and time

         # NOTE: Transfers to Withold account seem to be marked as... nothing
         if ($3 == "" ) {$3="Withold"}
         printf $3 OFS                    # Transaction type

         printf $4 OFS                    # Coin type

         # Set direction before sign removal
         direction=($5 ~ /^-/ ? "OUT" : "IN")

         # Remove all instances of sign
         sub(/^-/, "", $5)
         sub(/^-/, "", $6)

         # TODO: getline an gawk remove-zeros script
         amount_coin=sprintf("%.18f", $5)
         if (amount_coin ~ /^[\-0-9]*\.[0-9]+/)
           {
             sub(/0+$/, "", amount_coin)
             sub(/\.$/, "", amount_coin)
             sub(/^\./, "0.", amount_coin)
           }
         printf amount_coin OFS           # Coin amount

         # NOTE: USD could be up to 32 places... completely unnesessary
         amount_usd=sprintf("%.18f", $6)  # USD Value
         if (amount_usd ~ /^[\-0-9]*\.[0-9]+/)
           {
             sub(/0+$/, "", amount_usd)
             sub(/\.$/, "", amount_usd)
             sub(/^\./, "0.", amount_usd)
           }
         printf amount_usd OFS            # USD Value

         printf $7 OFS                    # Original Reward Coin

         interest_amount=sprintf("%.18f", $8)
         if (interest_amount ~ /^[\-0-9]*\.[0-9]+/)
           {
             sub(/0+$/, "", interest_amount)
             sub(/\.$/, "", interest_amount)
             sub(/^\./, "0.", interest_amount)
           }
         printf interest_amount OFS       # Reward Amount In Original Coin

         printf $9 OFS                    # Confirmed
         printf OFS                       # N/A (so rules work with REST API provided "tx_id")

         printf direction OFS
         printf global_subaccount

         printf "\n"

      }' FS=, OFS=, >"$global_out_path"
}

# REST API provided file
function parse_api()
{
  lib_preprocess::assert_header "id,amount,amount_precise,amount_usd,coin,state,nature,time,tx_id,original_interest_coin,interest_amount_in_original_coin"

  xan select "id,time,nature,coin,amount_precise,amount_usd,original_interest_coin,interest_amount_in_original_coin,state,tx_id" "$global_in_path" \
    | xan search -s "time" "$global_year" \
    | gawk -v global_subaccount="$global_subaccount" -M -v PREC=100 \
      '{
         if (NR<2)
           next

         cmd = "date \"+%F %T\" --utc --date="$2 | getline date

         printf $1 OFS                    # id
         printf date OFS                  # time

         # NOTE: Transfers to Withold account seem to be marked as... nothing
         if ($3 == "" ) {$3="Withold"}
         printf $3 OFS                    # nature

         printf $4 OFS                    # coin

         # Set direction before sign removal
         direction=($5 ~ /^-/ ? "OUT" : "IN")

         # Remove all instances of sign
         sub(/^-/, "", $5)
         sub(/^-/, "", $6)

         # TODO: getline an gawk remove-zeros script
         amount_precise=sprintf("%.18f", $5)
         if (amount_precise ~ /^[\-0-9]*\.[0-9]+/)
           {
             sub(/0+$/, "", amount_precise)
             sub(/\.$/, "", amount_precise)
             sub(/^\./, "0.", amount_precise)
           }
         printf amount_precise OFS        # amount_precise

         # NOTE: USD could be up to 32 places... completely unnesessary
         amount_usd=sprintf("%.18f", $6)
         if (amount_usd ~ /^[\-0-9]*\.[0-9]+/)
           {
             sub(/0+$/, "", amount_usd)
             sub(/\.$/, "", amount_usd)
             sub(/^\./, "0.", amount_usd)
           }
         printf amount_usd OFS            # amount_usd

         printf $7 OFS                    # original_interest_coin

         interest_amount=sprintf("%.18f", $8)
         if (interest_amount ~ /^[\-0-9]*\.[0-9]+/)
           {
             sub(/0+$/, "", interest_amount)
             sub(/\.$/, "", interest_amount)
             sub(/^\./, "0.", interest_amount)
           }
         printf interest_amount OFS

         printf $9 OFS                    # state
         printf $10 OFS                   # tx_id

         printf direction OFS
         printf global_subaccount

         printf "\n"

      }' FS=, OFS=, >"$global_out_path"
}

function main()
{
  lib_preprocess::test_header "Internal id" && parse_ui && return 0
  lib_preprocess::test_header "nature" && parse_api && return 0
}

main "$@"

# vim: sw=2 sts=2 si ai et
