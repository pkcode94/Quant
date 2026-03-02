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
source "${DOCKER_FINANCE_CONTAINER_REPO}/src/hledger-flow/lib/lib_utils.bash" || exit 1

#
# Implementation
#

[ -z "$global_year" ] && exit 1
[ -z "$global_subaccount" ] && exit 1

[ -z "$global_in_path" ] && exit 1
[ -z "$global_out_path" ] && exit 1

function btcpayserver::print_warning()
{
  [ -z "$global_account" ] && lib_utils::die_fatal
  lib_utils::print_warning "${global_account}: '$1' report is not supported"
}

#
# BTCPay Server v2
#

# NOTE/TODO:
#
# TL;DR:
#
#  - As of v2.2.0, *MUST* install the "Legacy Invoice Export" plugin. TODO: use v2.2.0 format w/out plugin
#
#  - *MUST* export both "Legacy Invoice" report and "Wallets" report for an accurate balance.
#    * The "Legacy Invoice" report currently does *not* include any outgoing txs (refunds or transfers).
#
# WARNING:
#
#  - If using a watch-ony wallet, consider accounting entirely here (via btcpayserver) or
#    entirely via that other wallet (e.g., electrum). Mixing the two may cause tax accounting issues.
#
# CAUTION:
#
#  - The "Wallets" report contains the txid that the "Payments" report *should* include (but doesn't).
#    * The "Legacy Invoice" report provides both.
#
#  - The "Wallets" report does *not* contain a description for refunds versus transfers.
#    * This is done in "Payouts" report or "Refunds" report.
#
#  - The "Payouts" report contains the destination address whereas "Refunds" does not.
#    * "Payouts" places the Invoice ID in the 'Source' field.
#    * "Refunds" places the Invoice ID in the 'InvoiceId' field.

# "Legacy Invoice" report
function btcpayserver::legacy()
{
  gawk --csv \
    -v global_year="$global_year" \
    -v global_subaccount="$global_subaccount" \
    '{
       if (NR<2 || $1 !~ global_year)
         next

       # ReceivedDate (w/ local timezone added)
       sub(/ /, "T", $1)  # HACK: makes arg-friendly by removing space
       cmd = "date \"+%F %T %z\" --date="$1 | getline date
       printf date OFS

       printf $2 OFS   # StoreId
       printf $3 OFS   # OrderId
       printf $4 OFS   # InvoiceId

       # InvoiceCreatedDate (w/ local timezone added)
       sub(/ /, "T", $5)
       cmd = "date \"+%F %T %z\" --date="$5 | getline date
       printf date OFS

       # InvoiceExpirationDate (w/ local timezone added)
       sub(/ /, "T", $6)
       cmd = "date \"+%F %T %z\" --date="$6 | getline date
       printf date OFS

       # InvoiceMonitoringDate (w/ local timezone added)
       sub(/ /, "T", $7)
       cmd = "date \"+%F %T %z\" --date="$7 | getline date
       printf date OFS

       # PaymentId
       # NOTE: BTCPay Server will append the block index as "-N" to the txid
       txid=substr($8, 1, 64); printf txid OFS
       ind=substr($8, 66, 2); printf ind OFS

       printf $9 OFS   # Destination
       printf $10 OFS  # PaymentType
       printf $11 OFS  # CryptoCode

       # Paid
       printf("%.8f", $12); printf OFS

       printf $13 OFS  # NetworkFee
       printf $14 OFS  # ConversionRate
       printf $15 OFS  # PaidCurrency
       printf $16 OFS  # InvoiceCurrency
       printf $17 OFS  # InvoiceDue
       printf $18 OFS  # InvoicePrice
       printf $19 OFS  # InvoiceTaxIncluded
       printf $20 OFS  # InvoiceTip
       printf $21 OFS  # InvoiceSubtotal
       printf $22 OFS  # InvoiceItemCode

       # InvoiceItemDesc
       sub(/%/, "%%", $23)
       gsub(/"/, "", $23)
       printf "\"" $23 "\"" OFS

       printf $24 OFS  # InvoiceFullStatus
       printf $25 OFS  # InvoiceStatus
       printf $26 OFS  # InvoiceExceptionStatus
       printf $27 OFS  # BuyerEmail
       printf $28 OFS  # Accounted

       # WARNING: appears to be always IN (see notes regarding "Wallets" report)
       printf "IN" OFS  # Direction
       printf global_subaccount  # Subaccount

       printf "\n"
    }' OFS=, "$global_in_path"
}

# "Payments" report
function btcpayserver::payments()
{
  btcpayserver::print_warning "Payments"
}

# "Payouts" report
function btcpayserver::payouts()
{
  btcpayserver::print_warning "Payouts"
}

# "Refunds" report
function btcpayserver::refunds()
{
  btcpayserver::print_warning "Refunds"
}

# "Sales" report
function btcpayserver::sales()
{
  btcpayserver::print_warning "Sales"
  # TODO: Accounting for this needs more consideration,
  #  as these are individual products sold w/ only fiat value given.
}

# "Wallets" report
function btcpayserver::wallets()
{
  gawk --csv \
    -v global_year="$global_year" \
    -v global_subaccount="$global_subaccount" \
    '{
       if (NR<2 || $1 !~ global_year)
         next

       # All "IN"s are removed and handled by legacy invoice
       if ($6 !~ /^-/)
         next

       # Strip sign from amount (using direction instead)
       sub(/^-/, "", $6)

       # Date (ReceivedDate w/ local timezone added)
       sub(/ /, "T", $1)  # HACK: makes arg-friendly by removing space
       cmd = "date \"+%F %T %z\" --date="$1 | getline date
       printf date OFS

       printf OFS     # (StoreId)
       printf OFS     # (OrderId)
       printf $4 OFS  # InvoiceId (InvoiceId)
       printf OFS     # (InvoiceCreatedDate)
       printf OFS     # (InvoiceExpirationDate)
       printf OFS     # (InvoiceMonitoringDate)

       # (PaymentId)
       printf $3 OFS  # TransactionId (txid)
       printf OFS     # (Block Index)

       printf OFS     # (Destination)
       printf OFS     # (PaymentType)
       printf $2 OFS  # Crypto (CryptoCode)

       # BalanceChange (Paid)
       # NOTE: must provide as subtotal, so subtract fee
       printf("%.8f", $6 - $7); printf OFS

       # Fee (NetworkFee)
       printf("%.8f", $7); printf OFS

       printf OFS     # (ConversionRate)

       # FeeRate (PaidCurrency)
       printf("%.3f", $8); printf OFS

       # Rate (XXX) (InvoiceCurrency)
       printf("%.3f", $9); printf OFS

       printf OFS     # (InvoiceDue)
       printf OFS     # (InvoicePrice)
       printf OFS     # (InvoiceTaxIncluded)
       printf OFS     # (InvoiceTip)
       printf OFS     # (InvoiceSubtotal)
       printf OFS     # (InvoiceItemCode)
       printf OFS     # (InvoiceItemDesc)
       printf OFS     # (InvoiceFullStatus)
       printf OFS     # (InvoiceStatus)
       printf OFS     # (InvoiceExceptionStatus)
       printf OFS     # (BuyerEmail)
       printf $5 OFS  # Confirmed (Accounted)

       # All "IN"s are removed and handled by legacy invoice
       printf "OUT" OFS  # Direction
       printf global_subaccount  # Subaccount

       printf "\n"
    }' OFS=, "$global_in_path"
}

function btcpayserver::parse()
{
  lib_preprocess::test_header "ReceivedDate,StoreId,OrderId,InvoiceId,InvoiceCreatedDate,InvoiceExpirationDate,InvoiceMonitoringDate,PaymentId,Destination,PaymentType,CryptoCode,Paid,NetworkFee,ConversionRate,PaidCurrency,InvoiceCurrency,InvoiceDue,InvoicePrice,InvoiceTaxIncluded,InvoiceTip,InvoiceSubtotal,InvoiceItemCode,InvoiceItemDesc,InvoiceFullStatus,InvoiceStatus,InvoiceExceptionStatus,BuyerEmail,Accounted" \
    && btcpayserver::legacy

  lib_preprocess::test_header "Date,InvoiceId,OrderId,Category,PaymentMethodId,Confirmed,Address,PaymentCurrency,PaymentAmount,PaymentMethodFee,LightningAddress,InvoiceCurrency,InvoiceCurrencyAmount,Rate" \
    && btcpayserver::payments

  lib_preprocess::test_header "Date,Source,State,PaymentType,Currency,Amount,OriginalCurrency,OriginalAmount,Destination" \
    && btcpayserver::payouts

  lib_preprocess::test_header "Date,InvoiceId,Currency,Completed,Awaiting,Limit,FullyPaid" \
    && btcpayserver::refunds

  lib_preprocess::test_header "Date,InvoiceId,State,AppId,Product,Quantity,CurrencyAmount,Currency" \
    && btcpayserver::sales

  # TODO: don't hardcode USD (upstream should make the currency in "Rate (XXX)" into a separate column).
  #lib_preprocess::test_header "Date,Crypto,TransactionId,InvoiceId,Confirmed,BalanceChange,Fee,FeeRate,Rate (USD)" \
  lib_preprocess::test_header "Date,Crypto,TransactionId,InvoiceId,Confirmed,BalanceChange,Fee,FeeRate" \
    && btcpayserver::wallets
}

function main()
{
  # WARNING: upstream produces carriage return all EOL
  btcpayserver::parse | sed -e 's:\x0d::g' >"$global_out_path" || lib_utils::catch $?
}

main "$@"

# vim: sw=2 sts=2 si ai et
