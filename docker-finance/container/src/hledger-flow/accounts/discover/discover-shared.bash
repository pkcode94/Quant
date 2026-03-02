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

function parse_credit()
{
  lib_preprocess::assert_header "Trans. Date,Post Date,Description,Amount,Category"

  # NOTE: sed needed to remove CR. TODO: use gawk
  sed 's:\r::g' "$global_in_path" \
    | gawk --csv \
      -v global_year="$global_year" \
      -v global_subaccount="$global_subaccount" \
      '{
         if (NR<2 || $1 !~ global_year)
           next

         printf $1 OFS  # Trans. Date
         printf $2 OFS  # Post Date

         # Description
         sub(/%/, "%%", $3)
         gsub(/"/, "", $3)
         printf "\"" $3 "\"" OFS

         # Flip the amount signs for proper liabilities accounting
         direction=($4 > 0 ? "OUT" : "IN")
         sub("-", "", $4)
         printf $4 OFS  # Amount

         printf "\"" $5 "\"" OFS  # Category

         printf direction OFS
         printf global_subaccount

         printf "\n"

      }' OFS=, >"$global_out_path"
}

function parse_bank()
{
  lib_preprocess::assert_header "Transaction Date,Transaction Description,Transaction Type,Debit,Credit,Balance"

  gawk --csv \
    -v global_year="$global_year" \
    -v global_subaccount="$global_subaccount" \
    '{
       if (NR<2 || $1 !~ global_year)
         next

       printf $1 OFS  # Transaction Date

       # Transaction Description
       sub(/%/, "%%", $2)
       gsub(/"/, "", $2)
       printf "\"" $2 "\"" OFS

       printf $3 OFS  # Transaction Type

       sub(/\$/,"",$4);
       sub(/\$/,"",$5);

       # Amount (Debit/Credit)
       direction=($4 > 0 ? "OUT" : "IN")
       sub("-", "", $4)
       printf (direction == "OUT" ? $4 : $5) OFS

       printf $6 OFS  # Balance

       printf direction OFS
       printf global_subaccount

       printf "\n"

    }' OFS=, "$global_in_path" >"$global_out_path"
}

function main()
{
  lib_preprocess::test_header "Trans. Date" && parse_credit && return 0
  lib_preprocess::test_header "Debit" && parse_bank && return 0
}

main "$@"

# vim: sw=2 sts=2 si ai et
