#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2021-2024 Aaron Fiore (Founder, Evergreen Crypto LLC)
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

# WARNING: this only supports WebUI CSV download and may be very out of date
function parse()
{
  lib_preprocess::assert_header "\"txid\",\"refid\",\"time\",\"type\",\"subtype\",\"aclass\",\"asset\",\"amount\",\"fee\",\"balance\""

  gawk -v global_year="$global_year" -v global_subaccount="$global_subaccount" \
    '{
       if (NR<2 || $3 !~ global_year)
         next

       gsub(/"/, "")

       # TODO: HACK to get around Kraken prepending X and Z for (most?) assets
       if (length($7) == 4)
         {
           sub(/^[Z,X]/, "", $7)
           sub(/XBT/, "BTC", $7)
         }

       printf $1 OFS   # txid
       printf $2 OFS   # refid
       printf $3 OFS   # time
       printf $4 OFS   # type
       printf $5 OFS   # subtype
       printf $6 OFS   # aclass
       printf $7 OFS   # asset
       printf("%.8f", $8); printf OFS   # amount
       printf("%.8f", $9); printf OFS   # fee
       printf("%.8f", $10); printf OFS  # balance

       if ($4 ~ /^deposit$/) {direction="IN"}
       if ($4 ~ /^withdrawal$/) {direction="OUT"}

       if ($4 ~ /^trade$/ && $8 ~ /^[0-9]/) {direction="BUY"}
       if ($4 ~ /^trade$/ && $8 ~ /^-/) {direction="SELL"}

       if ($4 ~ /^settled$/) {direction="SETTLED"}
       if ($4 ~ /^margin$/) {direction="MARGIN"}

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
