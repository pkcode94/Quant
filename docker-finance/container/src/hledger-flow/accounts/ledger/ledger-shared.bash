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

[ -z "$global_in_path" ] && exit 1
[ -z "$global_out_path" ] && exit 1

function parse()
{
  lib_preprocess::assert_header "Operation Date,Status,Currency Ticker,Operation Type,Operation Amount,Operation Fees,Operation Hash,Account Name,Account xpub,Countervalue Ticker,Countervalue at Operation Date,Countervalue at CSV Export"

  # NOTE: subaccount (account label) is entered within app.
  #  Label your subaccount as subaccount1:subaccount2:etc
  gawk --csv \
    -v global_year="$global_year" \
    '{
       if (NR<2 || $1 !~ global_year)
         next

       # App exports CR per-line (breaking below)
       gsub(/\r/, "")

       # Cleanup time
       sub(/T/, " ", $1)
       sub(/\.000Z/, "", $1)
       sub(/\.001Z/, "", $1)

       printf $1 OFS                    # Operation Date
       printf "\"" $2 "\"" OFS          # Status
       printf "\"" $3 "\"" OFS          # Currency Ticker
       printf "\"" $4 "\"" OFS          # Operation Type

       # Operation Amount (minus fee w/ this regexp)
       amount=($4 ~ /^OUT$|^DELEGATE$/ ? $5-$6 : $5)
       printf("%.8f", amount); printf OFS

       printf("%.8f", $6); printf OFS   # Operation Fees
       printf $7 OFS                    # Operation Hash

       # Account Name (subaccount)
       gsub(/"/, "", $8)
       printf "\"" $8 "\"" OFS

       printf $9 OFS                    # Account xpub
       printf "\"" $10 "\"" OFS         # Countervalue Ticker
       printf $11 OFS                   # Countervalue at Operation Date
       printf $12 OFS                   # Countervalue at CSV Export

       direction=($4 ~ /^OUT$|^DELEGATE$/ ? "OUT" : "IN")
       printf direction

       printf "\n"

    }' OFS=, "$global_in_path" >"$global_out_path"
}

function main()
{
  parse
}

main "$@"

# vim: sw=2 sts=2 si ai et
