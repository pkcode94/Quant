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
  lib_preprocess::assert_header "Transaction,Description,Token Quantity,Value,Status,Date,Listing,Url,Transaction ID"

  # NOTE: addresses contain commas
  sed 's:, : - :g' "$global_in_path" \
    | gawk -v global_year="$global_year" -v global_subaccount="$global_subaccount" \
      '{
         if (NR<2 || $6 !~ global_year)
           next

         cmd = "date --utc \"+%F\" --date="$6 | getline date

         printf $1 OFS    # Transaction
         printf $2 OFS    # Description
         printf $3 OFS    # Quantity

         # Value
         sub(/\$/, "", $4);
         direction=($4 ~ /^-/ ? "OUT" : "IN")
         sub(/^-/, "", $4)
         printf("%.8f", $4); printf OFS

         printf $5 OFS    # Status
         printf date OFS  # Date

         printf $7 OFS    # Listing
         printf $8 OFS    # Url
         printf $9 OFS    # Transaction ID

         printf direction OFS
         printf global_subaccount

         printf "\n"

      }' FS=, OFS=, >"$global_out_path"
}

function main()
{
  parse
}

main "$@"

# vim: sw=2 sts=2 si ai et
