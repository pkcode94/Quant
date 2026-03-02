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

# Since electrum 3.3.0
function parse_without-lightning-header()
{
  lib_preprocess::assert_header "transaction_hash,label,confirmations,value,fiat_value,fee,fiat_fee,timestamp"

  gawk --csv \
    -v global_year="$global_year" \
    -v global_subaccount="$global_subaccount" \
    '{
       if (NR<2 || $8 !~ global_year)
         next

       #
       # transaction_hash (oc_transaction_hash)
       #

       printf $1 OFS

       #
       # ln_payment_hash (N/A)
       #

       printf OFS

       #
       # label
       #

       sub(/%/, "%%", $2)
       printf "\""; printf $2; printf "\"" OFS

       #
       # confirmations
       #

       printf $3 OFS

       #
       # value (amount_chain_bc)
       #

       direction = ($4 ~ /^-/ ? "OUT" : "IN"); sub(/^-/, "", $4)
       value = $4 - $6  # Remove included network fee
       printf("%.8f", value); printf OFS

       #
       # amount_lightning_bc (N/A)
       #

       printf OFS

       #
       # fiat_value
       #

       if ($5 !~ /^No Data$/)
         {
           sub(/^-/, "", $5)
           printf("%.8f", $5)
         }
       printf OFS

       #
       # fee (network_fee_satoshi)
       #

       printf("%.8f", $6)
       printf OFS

       #
       # fiat_fee
       #

       if ($7 !~ /^No Data$/)
         {
           printf("%.8f", $7)
         }
       printf OFS

       #
       # timestamp
       #

       sub(/ /, "T", $8)  # HACK: makes arg-friendly by removing space
       cmd = "date \"+%F %T %z\" --date="$8 | getline date
       printf date OFS

       #
       # Direction, subaccount
       #

       printf direction OFS
       printf global_subaccount

       printf "\n"

    }' OFS=, "$global_in_path" >"$global_out_path"
}

# Since electrum 4.6.2
# TODO: *WARNING*: lightning support is a WIP due to an unresolved electrum lightning history bug (see docker-finance #227)
function parse_with-lightning-header()
{
  lib_preprocess::assert_header "oc_transaction_hash,ln_payment_hash,label,confirmations,amount_chain_bc,amount_lightning_bc,fiat_value,network_fee_satoshi,fiat_fee,timestamp"

  gawk --csv \
    -v global_year="$global_year" \
    -v global_subaccount="$global_subaccount" \
    '{
       if (NR<2 || $10 !~ global_year)
         next

       #
       # oc_transaction_hash
       #

       printf $1 OFS

       #
       # ln_payment_hash
       #

       printf $2 OFS

       #
       # label
       #

       sub(/%/, "%%", $3)
       print "\""; printf $3; printf "\"" OFS

       #
       # confirmations
       #

       printf $4 OFS

       #
       # amount_chain_bc
       #

       direction = ($5 ~ /^-/ ? "OUT" : "IN"); sub(/^-/, "", $5)

       # Network fee is now in satoshis
       network_fee = $8 * 0.00000001

       # Remove included network fee
       oc_amount = ($5 > 0 ? $5 - network_fee : $5)

       printf("%.8f", oc_amount); printf OFS

       #
       # amount_lightning_bc
       #

       # NOTE: opening a channel will be positive but it is not an IN
       if (direction == "IN")
         {
           if ($6 ~ /^-/)
             {
               direction = "OUT"
             }
           else
             {
               direction = "IN"
             }
         }
       sub(/^-/, "", $6)

       # If lightning hash exists, is not opening/closing a channel
       if ($2 ~ /[a-z0-9]/)
         {
           ln_amount = ($6 > 0 ? $6 - network_fee : $6)
         }
       else
         {
           ln_amount = $6
         }
       printf("%.8f", ln_amount); printf OFS

       #
       # fiat_value
       #

       if ($7 !~ /^No Data$|^$/)
         {
           sub(/^-/, "", $7)
           printf("%.8f", $7)
         }
       printf OFS

       #
       # network_fee_satoshi
       #

       printf("%.8f", network_fee); printf OFS

       #
       # fiat_fee
       #

       if ($9 !~ /^No Data$|^$/)
         {
           printf("%.8f", $9)
         }
       printf OFS

       #
       # timestamp
       #

       sub(/ /, "T", $10)  # HACK: makes arg-friendly by removing space
       cmd = "date \"+%F %T %z\" --date="$10 | getline date
       printf date OFS

       #
       # Direction, subaccount
       #

       printf direction OFS
       printf global_subaccount

       printf "\n"

    }' OFS=, "$global_in_path" >"$global_out_path"
}

function main()
{
  lib_preprocess::test_header "transaction_hash,label,confirmations,value,fiat_value,fee,fiat_fee,timestamp" \
    && parse_without-lightning-header && return 0

  lib_preprocess::test_header "oc_transaction_hash,ln_payment_hash,label,confirmations,amount_chain_bc,amount_lightning_bc,fiat_value,network_fee_satoshi,fiat_fee,timestamp" \
    && parse_with-lightning-header
}

main "$@"

# vim: sw=2 sts=2 si ai et
