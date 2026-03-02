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

[ -z "$global_in_filename" ] && exit 1

#
# Unified format between all file types:
#
#   Date,Type,Currency,Amount,Price,Subtotal,Total,Fees,Notes,Amount2,Destination,TransferID,TradeID,OrderID
#

account_id="$(echo $global_in_filename | cut -d'_' -f1)"
account_currency="$(echo $global_in_filename | gawk -F'_' '{sub(/\..*/, "", $2); print $2}')"

function parse_trade()
{
  # NOTE: header is variable, cannot assert reliably

  # There may be missing columns so fill in with placeholder data
  lib_preprocess::test_header "details_transfer_id" || _details_transfer_id="details_transfer_id"
  lib_preprocess::test_header "details_transfer_type" || _details_transfer_type="details_transfer_type"

  # TODO: refactor joining into lib_preprocess::add_to_header (check coinbase usage too)
  cat "$global_in_path" \
    | if [ ! -z "$_details_transfer_id" ]; then csvjoin -I --snifflimit 0 - <(echo "$_details_transfer_id"); else cat; fi \
    | if [ ! -z "$_details_transfer_type" ]; then csvjoin -I --snifflimit 0 - <(echo "$_details_transfer_type"); else cat; fi \
    | xan select "id,amount,balance,created_at,type,details_transfer_id,details_transfer_type,details_order_id,details_product_id,details_trade_id" \
    | gawk -v account_id="$account_id" -v account_currency="$account_currency" \
      -v global_year="$global_year" -v global_subaccount="$global_subaccount" \
      '{
         if (NR<2 || $4 !~ global_year)
           next

         sub(/T/, " ", $4)
         sub(/Z/, "", $4)
         sub(/\..*/, "", $4)

         id = $1
         amount = $2
         balance = $3
         created_at = $4
         type = $5
         details_transfer_id = $6
         details_transfer_type = $7
         details_order_id = $8
         details_product_id = $9
         details_trade_id = $10

         printf account_id OFS
         printf account_currency OFS
         printf id OFS
         printf("%.8f", amount); printf OFS
         printf("%.8f", balance); printf OFS
         printf created_at OFS
         printf type OFS
         printf details_transfer_id OFS
         printf details_transfer_type OFS
         printf details_order_id OFS
         printf details_product_id OFS
         printf details_trade_id OFS

         # Make rules file happy, these will have null values and OFS
         printf destination OFS
         printf subtotal OFS
         printf fee OFS
         printf total OFS
         printf txid OFS

         if (details_transfer_type ~ /^deposit$/) {direction="IN"}
         if (details_transfer_type ~ /^withdraw$/) {direction="OUT"}

         printf direction OFS
         printf global_subaccount

         printf "\n"

       }' FS=, OFS=, >"$global_out_path"
}

function parse_withdrawal()
{
  lib_preprocess::assert_header "type,account_id,created_at,destination,subtotal,fee,total,txid"

  gawk -v account_id="$account_id" -v account_currency="$account_currency" \
    -v global_year="$global_year" -v global_subaccount="$global_subaccount" \
    '{
       if (NR<2 || $3 !~ global_year)
         next

       if (account_id != $2)
         {
           print "FATAL: account_id = " account_id
           exit 1
         }

       sub(/T/, " ", $3)
       sub(/Z/, "", $3)
       sub(/\..*/, "", $3)

       # Set
       id = ""
       amount = ""
       balance = ""
       created_at = $3

       type = "transfer"
       details_transfer_id = ""
       details_transfer_type = $1
       details_order_id = ""
       details_product_id = ""
       details_trade_id = ""

       # Get
       printf account_id OFS
       printf account_currency OFS
       printf id OFS
       printf amount OFS
       printf balance OFS
       printf created_at OFS
       printf type OFS
       printf details_transfer_id OFS
       printf details_transfer_type OFS
       printf details_order_id OFS
       printf details_product_id OFS
       printf details_trade_id OFS

       destination = $4
       subtotal = $5
       fee = $6
       total = $7
       txid = $8

       printf destination OFS
       printf subtotal OFS
       printf fee OFS
       printf total OFS
       printf txid OFS

       # Fetch impl produces "transfer_out" (NOTE: *all* withdrawals will be "OUT")
       if (details_transfer_type ~ /^transfer_out$/) {direction="OUT"}

       printf direction OFS
       printf global_subaccount

       printf "\n"

  }' FS=, OFS=, "$global_in_path" >"$global_out_path"
}

function main()
{
  # REST API provided files
  lib_preprocess::test_header "details_trade_id" && parse_trade && return 0
  lib_preprocess::test_header "destination" && parse_withdrawal && return 0
}

main "$@"

# vim: sw=2 sts=2 si ai et
