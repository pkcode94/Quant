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
  lib_preprocess::assert_header "Transaction Date,Posted Date,Card No.,Description,Category,Debit,Credit"

  gawk --csv \
    -v global_year="$global_year" \
    -v global_subaccount="$global_subaccount" \
    '{
       if (NR<2 || !NF || $1 !~ global_year)
         next

       printf $1 OFS  # Transaction Date
       printf $2 OFS  # Posted Date
       printf $3 OFS  # Card No.

       # Description
       sub(/%/, "%%", $4)
       gsub(/"/, "", $4)
       printf "\"" $4 "\"" OFS

       printf "\"" $5 "\"" OFS  # Category

       # Debit = OUT, Credit = IN
       direction=($6 ~ /[1-9]/ ? "OUT" : "IN")

       # Amount
       if (direction == "OUT") {printf $6 OFS}  # Debit
       else {printf $7 OFS}  # Credit

       printf direction OFS
       printf global_subaccount

       printf "\n"

    }' OFS=, "$global_in_path" >"$global_out_path"
}

function parse_bank()
{
  lib_preprocess::assert_header "Account Number,Transaction Description,Transaction Date,Transaction Type,Transaction Amount,Balance"

  gawk --csv \
    -v global_year="$global_year" \
    -v global_subaccount="$global_subaccount" \
    '{
       if (NR>1 || NF)
         {
           # Date format is MM/DD/YY
           yy=substr($3, 7, 2)
           global_yy=substr(global_year, 3, 2)

           if (yy != global_yy)
             next

           # Remain consistent w/ pre-2024 format
           # (will be added in rules)
           sub(/^-/, "", $5)

           printf $1 OFS  # Account Number

           # Transaction Description
           sub(/%/, "%%", $2)
           gsub(/"/, "", $2)
           printf "\"" $2 "\"" OFS

           printf $3 OFS  # Transaction Date
           printf "\"" $4 "\"" OFS  # Transaction Type
           printf $5 OFS  # Transaction Amount
           printf $6 OFS  # Balance

           direction=($4 ~ /^Debit$/ ? "OUT" : "IN")
           printf direction OFS

           printf global_subaccount

           printf "\n"
        }

    }' OFS=, "$global_in_path" >"$global_out_path"
}

function main()
{
  lib_preprocess::test_header "Card No." && parse_credit && return 0
  lib_preprocess::test_header "Account Number" && parse_bank && return 0
}

main "$@"

# vim: sw=2 sts=2 si ai et
