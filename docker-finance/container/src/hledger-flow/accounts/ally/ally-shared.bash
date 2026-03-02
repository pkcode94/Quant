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

function parse()
{
  lib_preprocess::assert_header "Date, Time, Amount, Type, Description"

  gawk --csv \
    -v global_year="$global_year" \
    -v global_subaccount="$global_subaccount" \
    '{
       if (NR<2 || $1 !~ global_year)
         next

       printf $1 OFS  # Date
       printf $2 OFS  # Time

       sub(/^-/, "", $3)
       printf $3 OFS  # Amount

       printf $4 OFS  # Type

       # Description
       sub(/^ /, "", $5)  # may have an empty space in front of line
       sub(/%/, "%%", $5) # remove potential interpretation
       gsub(/"/, "", $5)  # remove nested quote(s)
       printf "\"" $5 "\"" OFS

       printf ($4 ~ /^Withdrawal$/ ? "OUT" : "IN") OFS  # Direction
       printf global_subaccount

       printf "\n"
    }' OFS=, "$global_in_path" >"$global_out_path"
}

function main()
{
  parse
}

main "$@"

# vim: sw=2 sts=2 si ai et
