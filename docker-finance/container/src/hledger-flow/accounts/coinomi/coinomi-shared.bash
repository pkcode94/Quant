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

function parse()
{
  lib_preprocess::assert_header "Asset, AccountName,Address,AddressName,Value,Symbol,Fees,InternalTransfer,TransactionID,Time(UTC),Time(ISO8601-UTC),BlockExplorer"

  gawk -v global_year="$global_year" -v global_subaccount="$global_subaccount" \
    '{
       if (NR<2 || $11 !~ global_year)
         next

       printf $1 OFS                          # Asset
       printf $2 OFS                          # AccountName
       printf $3 OFS                          # Address
       printf $4 OFS                          # AddressName

       # For some reason, coinomi includes senders fee for an ETH IN tx...
       # ...and Value is actual value minus fees...
       # ...so, this is adjusted accordingly.

       direction=($5 ~ /^-/ ? "OUT" : "IN"); sub(/^-/, "", $5)
       if (direction == "IN") {amount=$5+$7}
       if (direction == "IN" && $6 ~ /^ETH$/) {amount=$5}
       if (direction == "OUT") {amount=$5-$7}
       printf("%.8f", amount); printf OFS     # Value

       printf $6 OFS                          # Symbol
       printf("%.8f", $7); printf OFS         # Fees

       # ERC-20 txs will (should) include "(Ethereum)" after token name
       # NOTE: fixes GAS double-spends for ERC-20 transfers, apply in rules

       printf ($2 ~ /\(Ethereum\)/ ? "ETH" : $6) OFS  # FeeCurency

       printf $8 OFS                          # InternalTransfer
       printf $9 OFS                          # TransactionID

       # Seconds are only given in the Time(UTC) column... but that given format is unusable...
       #
       # Example:
       #
       #  Time(UTC) = "Mon 03 1 2022 10:18:20"
       #  Time(ISO8601-UTC) = "2022-01-03T10:18Z"
       #
       # So, replace Time(ISO8601-UTC)s hour:minute with Time(UTC)s hour:minute:second and use the ISO8601 column

       split($10, utc, " ");
       split($11, iso_utc, "T");
       date = iso_utc[1] " " utc[5]
       printf $10 OFS                         # Time(UTC)
       printf date OFS                        # Time(ISO8601-UTC)

       printf $12 OFS                         # BlockExplorer

       printf direction OFS
       printf global_subaccount

       printf "\n"

    }' FS=, OFS=, "$global_in_path" >"$global_out_path"

  # If out file has no transactions (such as at the beginning of the year),
  # it will create an empty file in 2-preprocessed. The following hack exists
  # because hledger-flow won't ignore deleted 1-in and 2-preprocessed files
  # after this script has been called.
  #
  # WARNING: must be also applied in rules file
  if [[ ! -s "$global_out_path" ]]; then
    echo "Asset,SKIP,Address,AddressName,0.00000000,Symbol,0.00000000,InternalTransfer,TransactionID,Time_UTC,Time_ISO8601-UTC,BlockExplorer,0.00000000,WalletName" >"$global_out_path"
  fi
}

function main()
{
  parse
}

main "$@"

# vim: sw=2 sts=2 si ai et
