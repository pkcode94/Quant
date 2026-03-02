#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2024 Aaron Fiore (Founder, Evergreen Crypto LLC)
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
  lib_preprocess::assert_header "Date,Transaction ID,Type,Received from (disclaimer: may not be accurate - first sender address only),Received amount,Received currency,Sent amount,Sent currency,Fee amount,Fee currency,"

  gawk -v global_year="$global_year" -v global_subaccount="$global_subaccount" \
    '{
       if (NR<2 || $1 !~ global_year)
         next

       # Format: MM/DD/YYYY HH:MM PM UTC
       timestamp=substr($1, 1, 19)  # Remove tail ("UTC")
       sub(/ /, "Z", timestamp)  # Replace first space
       sub(/ /, "", timestamp)  # Remove remaining space

       # Get/set date format
       cmd = "date \"+%F %T %z\" --utc --date="timestamp | getline date

       printf date OFS  # Date (timestamp)
       printf $2 OFS    # Transaction ID
       printf $3 OFS    # Type
       printf $4 OFS    # Received from (disclaimer: may not be accurate - first sender address only)
       printf $5 OFS    # Received amount
       printf $6 OFS    # Received currency
       printf $7 OFS    # Sent amount
       printf $8 OFS    # Sent currency
       printf $9 OFS    # Fee amount
       printf $10 OFS   # Fee currency

       printf ($7 ~ /[1-9]/ ? "OUT" : "IN") OFS  # Direction
       printf global_subaccount

       printf "\n"
    }' FS=, OFS=, "$global_in_path" | tac >"$global_out_path"
}

function main()
{
  parse
}

main "$@"

# vim: sw=2 sts=2 si ai et
