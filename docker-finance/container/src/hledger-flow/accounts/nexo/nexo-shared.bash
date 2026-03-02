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

# NOTE: prior to late 2022, Nexo provided column "Outstanding Loan" (but it's since been removed)

function parse()
{
  lib_preprocess::assert_header "Transaction,Type,Input Currency,Input Amount,Output Currency,Output Amount,USD Equivalent,Details,Date / Time"

  gawk -v global_year="$global_year" -v global_subaccount="$global_subaccount" \
    '{
       if (NR<2 || $9 !~ global_year)
         next

       # NOTE: they use NEXONEXO for NEXO that isnt a deposit...
       sub(/NEXONEXO/, "NEXO", $3)

       # Remove symbol in "USD Equivalent"
       sub(/\$/, "", $7)

       # Create new column: separate "Details" into "Status,Details"
       gsub(/"/, "", $8)
       sub(/ \/ /, ",", $8)

       # Cleanup "Details" (still seen as $8 instead of $9)
       sub(/%/, " percent", $8)
       sub(/,Interest$/, ",Interest Paid", $8)

       # Print
       printf $1 OFS                   # Transaction
       printf $2 OFS                   # Type
       printf $3 OFS                   # Input Currency

       # Input Amount
       direction=($4 ~ /^-/ ? "OUT" : "IN")
       sub(/^-/, "", $4)
       printf("%.8f", $4); printf OFS

       printf $5 OFS                   # Output Currency

       sub(/^-/, "", $6)
       printf("%.8f", $6); printf OFS  # Output Amount

       printf("%.8f", $7); printf OFS  # USD Equivalent
       printf $8 OFS                   # Status
       printf $9 OFS                   # Details
       printf $10                      # Date / Time (FS is included)

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
