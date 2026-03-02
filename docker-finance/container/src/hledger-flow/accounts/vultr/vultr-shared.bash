#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2024-2025 Aaron Fiore (Founder, Evergreen Crypto LLC)
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

function parse()
{
  [ -z "$global_year" ] && exit 1
  [ -z "$global_subaccount" ] && exit 1

  [ -z "$global_in_path" ] && exit 1
  [ -z "$global_out_path" ] && exit 1

  #
  # NOTE:
  #
  #  - Upstream does not provide CSV header
  #
  #  - Invoice number is provided as filename
  #    * invoice_12345678.csv
  #

  [ -z "$global_in_filename" ] && exit 1
  local -r _invoice="${global_in_filename:8:8}"

  gawk \
    -v global_year="$global_year" \
    -v global_subaccount="$global_subaccount" \
    -v invoice="$_invoice" \
    '{
       # WARNING:
       #
       #   Because upstream INVOICE DATE will be at beginning of the month,
       #   any January invoices *MUST* also go into the previous year December,
       #   in order to catch Sales Tax issued on the 1st for the previous month.

       if ($2 !~ global_year)
         next

       # Skip $1      # start date

       printf $2 OFS  # end date (date)

       sub(/%/, "%%", $3)
       printf $3 OFS  # description

       # Skip $4

       printf $5 OFS  # quantity

       direction = ($6 ~ /^"-/ ? "IN" : "OUT")
       sub(/^"-/, "\"", $6)
       printf $6 OFS  # amount

       printf invoice OFS  # invoice number

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
