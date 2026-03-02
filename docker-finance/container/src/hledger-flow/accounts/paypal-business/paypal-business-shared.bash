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
  local _header="\"Date\",\"Time\",\"TimeZone\",\"Name\",\"Type\",\"Status\",\"Currency\",\"Gross\",\"Fee\",\"Net\",\"From Email Address\",\"To Email Address\",\"Transaction ID\",\"Shipping Address\",\"Address Status\",\"Item Title\",\"Item ID\",\"Shipping and Handling Amount\",\"Insurance Amount\",\"Sales Tax\",\"Option 1 Name\",\"Option 1 Value\",\"Option 2 Name\",\"Option 2 Value\",\"Reference Txn ID\",\"Invoice Number\",\"Custom Number\",\"Quantity\",\"Receipt ID\",\"Balance\",\"Address Line 1\",\"Address Line 2/District/Neighborhood\",\"Town/City\",\"State/Province/Region/County/Territory/Prefecture/Republic\",\"Zip/Postal Code\",\"Country\",\"Contact Phone Number\",\"Subject\",\"Note\",\"Country Code\",\"Balance Impact\""

  lib_preprocess::assert_header "$_header"

  # Paypal is known to allow commas within description ("Name") and amounts
  # TODO: refactor xan/sed -> gawk (the sed line should remove non-csv commas and quotations)
  xan select "$(echo "$_header" | sed 's:"::g')" $global_in_path \
    | gawk --csv \
      -v global_year="$global_year" \
      -v global_subaccount="$global_subaccount" \
      '{
         if (NR<2 || $1 !~ global_year)
           next

         # Need an integer if empty
         if ($8 == "") {$8 = "0"}
         if ($9 == "") {$9 = "0"}
         if ($10 == "") {$10 = "0"}

         printf $1 " " $2 OFS  # Date/Time

         # Name (description)
         sub(/%/, "%%", $4)
         gsub(/"/, "", $4)
         printf "\"" $4 "\"" OFS

         printf "\"" $5 "\"" OFS  # Type
         printf "\"" $6 "\"" OFS  # Status
         printf "\"" $7 "\"" OFS  # Currency

         # direction will be used in rules
         sub(/^-/, "", $8)
         printf $8 OFS

         printf $11 OFS        # From Email Address
         printf $12 OFS        # To Email Address
         printf $13 OFS        # Transaction ID
         printf $(NF-4) OFS    # Contact Phone Number

         # Note
         sub(/%/, "%%", $(NF-2))
         gsub(/"/, "", $(NF-2))
         printf "\"" $(NF-2) "\"" OFS

         printf $(NF) OFS      # Balance Impact

         # Direction
         printf ($(NF) ~ "Debit" ? "OUT" : "IN") OFS

         printf global_subaccount

         printf "\n"

      }' OFS=, >"$global_out_path"
}

function main()
{
  parse
}

main "$@"

# vim: sw=2 sts=2 si ai et
