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

[ -z "$global_in_path" ] && exit 1
[ -z "$global_out_path" ] && exit 1

function parse()
{
  lib_preprocess::assert_header "date,blockchain,account_name,subaccount_name,subaccount_address,type,direction,tx_index,tx_hash,token_name,token_id,contract_address,symbol,block_hash,block_number,method_id,from_address,to_address,amount,fees"

  # NOTE: here, global_subaccount is effectively replaced by the blockchain fetch implementation
  gawk -v global_year="$global_year" \
    '{
       if (NR<2 || $1 !~ global_year)
         next

       # TODO: full s/v/aToken support

       # V2
       gsub(/,variableDebtGUSD,/, ",aGUSD,")
       gsub(/,variableDebtUSDC,/, ",aUSDC,")
       gsub(/,variableDebtUSDT,/, ",aUSDT,")
       gsub(/,variableDebtDAI,/, ",aDAI,")

       # V3
       gsub(/,variableDebtEthUSDC,/, ",vEthUSDC,")

       printf $1 OFS   # date
       printf $2 OFS   # blockchain
       printf $3 OFS   # account_name
       printf $4 OFS   # subaccount_name
       printf $5 OFS   # subaccount_address
       printf $6 OFS   # type
       printf $7 OFS   # direction
       printf $8 OFS   # tx_index
       printf $9 OFS   # tx_hash
       printf $10 OFS  # token_name
       printf $11 OFS  # token_id
       printf $12 OFS  # contract_address
       printf $13 OFS  # symbol
       printf $14 OFS  # block_hash
       printf $15 OFS  # block_number
       printf $16 OFS  # method_id
       printf $17 OFS  # from_address
       printf $18 OFS  # to_address

       # TODO: improve 0 removal (see others)
       amount=$19
       if (amount ~ /\./)
         {
           sub("0*$", "", amount)
           sub("\\.$", "", amount)
         }
       printf amount OFS

       fees=$20
       if (fees ~ /\./)
         {
           sub("0*$", "", fees)
           sub("\\.$", "", fees)
         }
       printf fees

       printf "\n"

    }' FS=, OFS=, "$global_in_path" >"$global_out_path"
}

function main()
{
  parse
}

main "$@"

# vim: sw=2 sts=2 si ai et
