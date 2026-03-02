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

[ -z "$global_in_path" ] && exit 1
[ -z "$global_out_path" ] && exit 1

function parse()
{
  lib_preprocess::assert_header "account_name,subaccount_name,subaccount_address,hash,type,status,block,time,volume,fee,sender,alias,receiver,direction,delegate"

  # NOTE: here, global_subaccount is effectively replaced by the blockchain fetch implementation
  gawk -v global_year="$global_year" \
    '{
       if (NR<2 || $8 !~ global_year)
         next

       sub(/T/, " ", $8)
       sub(/Z/, "", $8)

       printf $1 OFS   # account_name
       printf $2 OFS   # subaccount_name
       printf $3 OFS   # subaccount_address
       printf $4 OFS   # hash
       printf $5 OFS   # type
       printf $6 OFS   # status
       printf $7 OFS   # block
       printf $8 OFS   # time
       printf $9 OFS   # volume
       printf $10 OFS  # fee
       printf $11 OFS  # sender
       printf $12 OFS  # alias (sender)
       printf $13 OFS  # receiver
       printf $14 OFS  # direction
       printf $15      # delegate

       printf "\n"

     }' FS=, OFS=, "$global_in_path" >"$global_out_path"
}

function main()
{
  parse
}

main "$@"

# vim: sw=2 sts=2 si ai et
