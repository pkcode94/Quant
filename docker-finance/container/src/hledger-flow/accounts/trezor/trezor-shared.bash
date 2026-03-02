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

[ -z "$global_in_filename" ] && exit 1

function parse()
{
  lib_preprocess::assert_header "Timestamp;Date;Time;Type;Transaction ID;Fee;Fee unit;Address;Label;Amount;Amount unit;Fiat (USD);Other"

  # Get subaccount(s) from filename
  local _subaccount
  _subaccount="$(echo $global_in_filename | cut -d'_' -f1)"

  # NOTE: subaccount (account label) is entered within the app
  gawk \
    -v global_year="$global_year" \
    -v global_subaccount="$global_subaccount" \
    -v subaccount="$_subaccount" \
    '{
       if (NR<2)
         next

       cmd = "date --utc \"+%F %T\" --date=@"$1 | getline date

       if (date !~ global_year)
         next

       # Timestamp provides UTC. UTC is used even though Time provides
       # local timezone because fetching outside of the app provides UTC
       printf date OFS                  # Timestamp
       # skip                           # Date (N/A)
       # skip                           # Time (N/A)

       # Type                                           # TODO: are there more types?
       direction=($4 ~ /^RECV$/ ? "IN" : "OUT")
       printf $4 OFS

       printf $5 OFS                    # Transaction ID
       printf $6 OFS                    # Fee
       printf "\"" $7 "\"" OFS          # Fee unit
       printf $8 OFS                    # Address

       # Label
       sub(/%/, "%%", $9)
       gsub(/"/, "", $9)
       printf "\"" $9 "\"" OFS

       printf("%.8f", $10); printf OFS  # Amount        # TODO: more than 8 places for Ethereum, etc.?
       printf "\"" $11 "\"" OFS         # Amount unit
       printf "\"" $12 "\"" OFS         # Fiat (USD)
       printf "\"" $13 "\"" OFS         # Other

       printf direction OFS
       printf global_subaccount":"subaccount

       printf "\n"

    }' FS=\; OFS=, "$global_in_path" >"$global_out_path"
}

function main()
{
  parse
}

main "$@"

# vim: sw=2 sts=2 si ai et
