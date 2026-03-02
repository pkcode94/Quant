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

function parse()
{
  # NOTE: skipping comments, header begins at line 3
  lib_preprocess::assert_header "TransactionDate,TransactionType,SecurityType,Symbol,Quantity,Amount,Price,Commission,Description" "$(sed -n '3p' $global_in_path)"

  gawk -v global_year="$global_year" -v global_subaccount="$global_subaccount" \
    '{
       if (NR<4 || !NF)
         next

       # Date format is MM/DD/YY
       yy=substr($1, 7, 2)
       global_yy=substr(global_year, 3, 2)

       if (yy != global_yy)
         next

       gsub(/^$/, "")
       gsub(/, ,/, ",USD,")

       printf $1 OFS  # TransactionDate
       printf $2 OFS  # TransactionType
       printf $3 OFS  # SecurityType
       printf $4 OFS  # Symbol
       printf $5 OFS  # Quantity

       direction=($6 ~ /^-/ ? "OUT" : "IN")
       sub(/^-/, "", $6)
       printf $6 OFS  # Amount

       printf $7 OFS  # Price
       printf $8 OFS  # Comission

       sub(/%/, "%%", $9)
       printf $9 OFS  # Description

       printf direction OFS
       printf global_subaccount

       printf "\n"

  }' FS=, OFS=, "$global_in_path" >"$global_out_path"
}

function main()
{
  parse
}

main "$@"

# vim: sw=2 sts=2 si ai et
