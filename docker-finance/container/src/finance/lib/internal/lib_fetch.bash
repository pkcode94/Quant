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
source "${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/internal/lib_utils.bash" || exit 1

#
# Facade
#

function lib_fetch::fetch()
{
  lib_fetch::__parse_args "$@"
  lib_fetch::__fetch "$@"
  lib_utils::catch $?
}

#
# Implementation
#

function lib_fetch::__parse_args()
{
  [ -z "$global_usage" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_1" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_2" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_3" ] && lib_utils::die_fatal

  local -r _usage="
\e[32mDescription:\e[0m

  Fetch prices, remote accounts and blockchain data

\e[32mUsage:\e[0m

  $ $global_usage <<[all${global_arg_delim_2}<type1[{${global_arg_delim_3}type2${global_arg_delim_3}...}]>]> | <[price${global_arg_delim_2}<id${global_arg_delim_1}ticker[{${global_arg_delim_3}id${global_arg_delim_1}ticker${global_arg_delim_3}...}]>] | [account${global_arg_delim_2}<account1[{${global_arg_delim_3}account2${global_arg_delim_3}...}]>]>> [blockchain${global_arg_delim_2}<blockchain1[{${global_arg_delim_3}blockchain2${global_arg_delim_1}subaccount${global_arg_delim_3}...}]>] [year${global_arg_delim_2}<year|all>]

\e[32mArguments:\e[0m

  All options (fetch type):

    all${global_arg_delim_2}<all|price|account>

  Asset values (see documentation):

    price${global_arg_delim_2}<asset1${global_arg_delim_3}asset2${global_arg_delim_3}...>

    api${global_arg_delim_2}<mobula|coingecko>

  Account(s):

    account${global_arg_delim_2}<account1${global_arg_delim_3}account2${global_arg_delim_3}...>

      Supported account(s):

        - coinbase
        - coinbase-wallet
        - coinomi
        - gemini
        - ledger
        - metamask
        - pera-wallet

  Fetch year:

    year${global_arg_delim_2}<all|year>

  Blockchain(s) w/ optional subaccount:

    (block)chain${global_arg_delim_2}<blockchain[${global_arg_delim_3}blockchain${global_arg_delim_1}subaccount]>

      Supported blockchain(s):

        - algorand
        - arbitrum
        - base
        - ethereum
        - optimism
        - tezos

  Tor (proxy):

    tor${global_arg_delim_2}<{on|true}|{off|false}> (default 'off')

\e[32mExamples:\e[0m

  \e[37;2m#\e[0m
  \e[37;2m# All (account, price)\e[0m
  \e[37;2m#\e[0m

  \e[37;2m# Fetch all types for current (default) year\e[0m
  $ $global_usage all${global_arg_delim_2}all

  \e[37;2m# Fetch all accounts for current (default) year\e[0m
  $ $global_usage all${global_arg_delim_2}account

  \e[37;2m# Fetch all historical daily average prices and accounts for current (default) year\e[0m
  $ $global_usage all${global_arg_delim_2}price${global_arg_delim_3}account

  \e[37;2m# Fetch all historical daily average prices of assets in configuration (since genesis)\e[0m
  $ $global_usage all${global_arg_delim_2}price year${global_arg_delim_2}all

  \e[37;2m# Fetch all historical daily average prices of assets in configuration (since genesis) using Mobula API\e[0m
  $ $global_usage all${global_arg_delim_2}price year${global_arg_delim_2}all api${global_arg_delim_2}mobula

  \e[37;2m#\e[0m
  \e[37;2m# Price\e[0m
  \e[37;2m#\e[0m

  \e[37;2m# Fetch historical daily average prices since genesis for only Bitcoin (using default aggregator)\e[0m
  $ $global_usage price${global_arg_delim_2}bitcoin${global_arg_delim_1}BTC year${global_arg_delim_2}all

  \e[37;2m# Fetch only current year historical daily average prices for Bitcoin and Ethereum from CoinGecko\e[0m
  $ $global_usage price${global_arg_delim_2}bitcoin${global_arg_delim_1}BTC${global_arg_delim_3}ethereum${global_arg_delim_1}ETH api${global_arg_delim_2}coingecko

  \e[37;2m#\e[0m
  \e[37;2m# Account\e[0m
  \e[37;2m#\e[0m

  \e[37;2m# NOTE: use blockchain names, not ticker symbols\e[0m

  \e[37;2m# Fetch multiple accounts for current year\e[0m
  $ $global_usage account${global_arg_delim_2}gemini${global_arg_delim_3}coinbase

  \e[37;2m# Fetch all ethereum subaccounts for all scanner-based accounts, current year\e[0m
  $ $global_usage all${global_arg_delim_2}account blockchain${global_arg_delim_2}ethereum

  \e[37;2m# Fetch ethereum/polygon subaccounts for account metamask, year 2022\e[0m
  $ $global_usage account${global_arg_delim_2}metamask blockchain${global_arg_delim_2}ethereum${global_arg_delim_3}polygon year${global_arg_delim_2}2022

  \e[37;2m# Fetch multiple blockchains' subaccounts for account ledger and metamask, year 2023\e[0m
  $ $global_usage account${global_arg_delim_2}ledger${global_arg_delim_3}metamask blockchain${global_arg_delim_2}ethereum${global_arg_delim_3}base${global_arg_delim_3}tezos${global_arg_delim_3}algorand year${global_arg_delim_2}2023

  \e[37;2m# Same as previous command but with shorthand option 'chain'\e[0m
  $ $global_usage account${global_arg_delim_2}ledger${global_arg_delim_3}metamask chain${global_arg_delim_2}ethereum${global_arg_delim_3}base${global_arg_delim_3}tezos${global_arg_delim_3}algorand year${global_arg_delim_2}2023

  \e[37;2m# Fetch multiple blockchains/subaccounts for ledger\e[0m
  $ $global_usage account${global_arg_delim_2}ledger chain${global_arg_delim_2}ethereum/nano:x-1${global_arg_delim_3}tezos${global_arg_delim_1}nano:s-plus${global_arg_delim_3}algorand${global_arg_delim_1}nano:x-2

  \e[37;2m# Fetch specific blockchain/subaccount/address for metamask\e[0m
  $ $global_usage account${global_arg_delim_2}metamask chain${global_arg_delim_2}ethereum/phone:wallet-1/0x236ba53B56FEE4901cdac3170D17f827DF43E969${global_arg_delim_3}optimism/phone:wallet-3/0x18BFAa3f9Fa06123985d0456b4a911730382009D

  \e[37;2m# Fetch given blochchain-based subaccounts for current year, over the Tor network\e[0m
  $ $global_usage account${global_arg_delim_2}metamask${global_arg_delim_3}ledger blockchain${global_arg_delim_2}ethereum${global_arg_delim_3}polygon${global_arg_delim_3}arbitrum${global_arg_delim_3}tezos${global_arg_delim_3}algorand tor${global_arg_delim_2}on

\e[32mNotes:\e[0m

  - For all commands, you can proxy your fetch through Tor with the \`tor${global_arg_delim_2}on\` option
    * IMPORTANT: client-side \`tor\` plugin must be started *prior* to fetch
"

  #
  # Ensure supported arguments
  #

  [ $# -eq 0 ] && lib_utils::die_usage "$_usage"

  for _arg in "$@"; do
    [[ ! "$_arg" =~ ^all${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^price[s]?${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^api[s]?${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^account[s]?${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^(^block)?chain[s]?${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^year[s]?${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^tor${global_arg_delim_2} ]] \
      && lib_utils::die_usage "$_usage"
  done

  #
  # Parse arguments before testing
  #

  # Parse key for value
  for _arg in "$@"; do

    local _key="${_arg%${global_arg_delim_2}*}"
    local _len="$((${#_key} + 1))"

    if [[ "$_key" =~ ^all$ ]]; then
      local _arg_all="${_arg:${_len}}"
      [ -z "$_arg_all" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^price[s]?$ ]]; then
      local _arg_price="${_arg:${_len}}"
      [ -z "$_arg_price" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^api[s]?$ ]]; then
      local _arg_api="${_arg:${_len}}"
      [ -z "$_arg_api" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^account[s]?$ ]]; then
      local _arg_account="${_arg:${_len}}"
      [ -z "$_arg_account" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^(^block)?chain[s]?$ ]]; then
      local _arg_chain="${_arg:${_len}}"
      [ -z "$_arg_chain" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^year[s]?$ ]]; then
      local _arg_year="${_arg:${_len}}"
      [ -z "$_arg_year" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^tor$ ]]; then
      local _arg_tor="${_arg:${_len}}"
      [ -z "$_arg_tor" ] && lib_utils::die_usage "$_usage"
    fi
  done

  #
  # Test for valid ordering/functionality of argument values
  #

  # Arg: all
  if [ ! -z "$_arg_all" ]; then
    if [[ ! -z "$_arg_price" || ! -z "$_arg_account" || ! -z "$_arg_chain" ]]; then
      lib_utils::die_usage "$_usage"
    fi
  fi

  # Arg: price
  if [ ! -z "$_arg_price" ]; then
    # NOTE:
    #  - price shouldn't test against account or chain because there may be a
    #    simultaneous call to account within the same issuing commandline.
    if [[ ! -z "$_arg_all" ]]; then
      lib_utils::die_usage "$_usage"
    fi
  fi

  # Arg: api
  if [ ! -z "$_arg_api" ]; then
    # Need a valid arg
    if [[ -z "$_arg_all" && -z "$_arg_price" ]]; then
      lib_utils::die_usage "$_usage"
    fi
  fi

  # Arg: account
  if [ ! -z "$_arg_account" ]; then
    if [[ ! -z "$_arg_all" ]]; then
      lib_utils::die_usage "$_usage"
    fi
  fi

  # Arg: chain
  if [ ! -z "$_arg_chain" ]; then
    # Need account arg
    if [ -z "$_arg_account" ]; then
      lib_utils::die_usage "$_usage"
    fi
  fi

  # Arg: year
  if [ ! -z "$_arg_year" ]; then
    # Need a valid arg
    if [[ -z "$_arg_all" && -z "$_arg_price" && -z "$_arg_account" ]]; then
      lib_utils::die_usage "$_usage"
    fi
  fi

  # Arg: tor
  if [ ! -z "$_arg_tor" ]; then
    # Need a valid arg
    if [[ -z "$_arg_all" && -z "$_arg_price" && -z "$_arg_account" ]]; then
      lib_utils::die_usage "$_usage"
    fi
  fi

  #
  # Test argument values, set globals
  #

  IFS="$global_arg_delim_3"

  # Arg: all
  if [ ! -z "$_arg_all" ]; then
    read -ra _read <<<"$_arg_all"
    for _arg in "${_read[@]}"; do
      # Supported values
      [[ ! "$_arg" =~ ^all$|^price[s]?$|^account[s]?$ ]] \
        && lib_utils::die_usage "$_usage"

      # If all=all then no need for all={price,account} and price= or account=
      [[ "$_arg" == "all" && (! -z "$_arg_price" || ! -z "$_arg_account") ]] \
        || [[ "$_arg" == "all" && "${#_read[@]}" -gt 1 ]] \
        && lib_utils::die_usage "$_usage"
    done
    declare -gr global_arg_all=("${_read[@]}")
  fi

  # Arg: price
  if [ ! -z "$_arg_price" ]; then
    read -ra _read <<<"$_arg_price"
    declare -gr global_arg_price=("${_read[@]}")
  fi

  # Arg: api
  if [ ! -z "$_arg_api" ]; then
    read -ra _read <<<"$_arg_api"
    for _arg in "${_read[@]}"; do
      if [[ ! "$_arg" =~ ^coingecko$|^mobula$ ]]; then
        lib_utils::die_usage "$_usage"
      fi
    done
    declare -gr global_arg_api=("${_read[@]}")
  else
    declare -gr global_arg_api=("coingecko" "mobula")
  fi

  # Arg: account
  if [ ! -z "$_arg_account" ]; then
    read -ra _read <<<"$_arg_account"
    # TODO: make readonly
    declare -g global_arg_account=("${_read[@]}")
  fi

  # Arg: chain
  if [ ! -z "$_arg_chain" ]; then
    read -ra _read <<<"$_arg_chain"
    declare -gr global_arg_chain=("${_read[@]}")
  fi

  # Arg: year
  # TODO: implement range
  if [ ! -z "$_arg_year" ]; then
    # TODO: 20th century support
    if [[ ! "$_arg_year" =~ ^20[0-9][0-9]$ && "$_arg_year" != "all" ]]; then
      lib_utils::die_usage "$_usage"
    fi
    # TODO: implement "all" for "accounts", if possible
    if [[ "$_arg_year" == "all" && ("${global_arg_all[*]}" =~ account || ! -z "${global_arg_account[*]}") ]]; then
      _arg_year="$(date +%Y)"
      lib_utils::print_warning "year 'all' is not supported with type 'account', using year '${_arg_year}'"
    fi
    declare -gr global_arg_year="$_arg_year"
  else
    global_arg_year="$(date +%Y)" # Set default current
    declare -gr global_arg_year
  fi

  # Arg: tor
  if [ ! -z "$_arg_tor" ]; then
    [[ ! "$_arg_tor" =~ ^on$|^true$|^off$|^false$ ]] && lib_utils::die_usage "$_usage"
    declare -gr global_arg_tor="$_arg_tor"
  fi
}

# TODO: complete __fetch() rewrite
function lib_fetch::__fetch()
{
  # Supported remote fetch accounts
  local -r _supported_accounts=(
    "coinbase"
    "coinbase-wallet"
    "coinomi"
    "gemini"
    "ledger"
    "metamask"
    "pera-wallet"
  )

  [ -z "$global_parent_profile" ] && lib_utils::die_fatal
  [ -z "$global_child_profile" ] && lib_utils::die_fatal

  # TODO: global args should be set in arg parsing and made readonly
  # Fetch only given accounts
  if [ ! -z "$global_arg_account" ]; then
    for _account in "${global_arg_account[@]}"; do
      local _value
      _value="$(lib_fetch::__parse_yaml "get-value" "account.${_account}" 2>/dev/null)"
      if [[ "$_value" == "null" ]]; then
        lib_utils::print_warning "account.${_account} not found, skipping!"
      else
        local _accounts+=("$_account")
      fi
    done
    global_arg_account=("${_accounts[@]}")
  elif [ ! -z "${global_arg_all[*]}" ]; then
    global_arg_account=("${_supported_accounts[@]}")
  fi

  # Fetch all supported accounts and/or prices
  if [ ! -z "${global_arg_all[*]}" ]; then
    for _all in "${global_arg_all[@]}"; do
      if [ "$_all" == "all" ]; then
        time lib_fetch::__fetch_price
        time lib_fetch::__fetch_account
      fi
      if [[ "$_all" =~ ^price[s]?$ ]]; then
        time lib_fetch::__fetch_price
      elif [[ "$_all" =~ ^account[s]?$ ]]; then
        time lib_fetch::__fetch_account
      fi
    done
  else
    if [ ! -z "${global_arg_price[*]}" ]; then
      time lib_fetch::__fetch_price
    fi
    if [ ! -z "${global_arg_account[*]}" ]; then
      time lib_fetch::__fetch_account
    fi
  fi
}

function lib_fetch::__fetch_account()
{
  # Cycle and fetch through all accounts
  # NOTE: account name must match internal fetch impl basename
  for _account in "${global_arg_account[@]}"; do

    #
    # Set preliminary properties based on account
    #

    local _need_key=false
    local _need_passphrase=false
    local _need_secret=false

    case "$_account" in
      coinbase | gemini)
        _need_key=true
        _need_secret=true
        ;;
    esac

    #
    # Test and set if configuration contains supported members
    #

    local _members=("key" "passphrase" "secret" "subaccount")

    for _member in "${_members[@]}"; do

      # Get member value
      local _value
      _value=$(lib_fetch::__parse_yaml "get-values" "account.${_account}.${_member}")

      if [ $? -eq 0 ]; then

        # Sanitize for 'yq'-specific caveats
        local _sanitized
        if [[ "$_value" == "null" ]]; then
          _sanitized=""
        else
          # Values may contain 'yq'-interpreted characters (newlines, etc.)
          # NOTE: this is needed for at least Coinbase's CDP secret
          _sanitized="$(echo "$_value" | sed -e 's:^[ \t]*::' -e '/^$/d' -e "s:^'::" -e "s:'$::")"
        fi

        case "$_member" in
          key)
            local _api_key="$_sanitized"
            [[ -z "$_api_key" && $_need_key == true ]] \
              && lib_utils::die_fatal "$_account - empty fetch ${_member}!"
            ;;
          passphrase)
            local _api_passphrase="$_sanitized"
            [[ -z "$_api_passphrase" && $_need_passphrase == true ]] \
              && lib_utils::die_fatal "$_account - empty fetch ${_member}!"
            ;;
          secret)
            local _api_secret="$_sanitized"
            [[ -z "$_api_secret" && $_need_secret == true ]] \
              && lib_utils::die_fatal "$_account - empty fetch ${_member}!"
            ;;
          subaccount)
            # NOTE: member value for subaccount will *always* need subaccount
            local _api_subaccount="$_sanitized"
            [ -z "$_api_subaccount" ] \
              && lib_utils::die_fatal "$_account - empty fetch ${_member}!"
            ;;
          *)
            lib_utils::die_fatal "Unsupported member"
            ;;
        esac
      else
        lib_utils::print_warning "$_account is unavailable, skipping!"
        continue 2
      fi
    done

    #
    # Set output dir for internally fetched files
    #
    # NOTE:
    #   - subaccount may include custom ticker list with format:
    #
    #       subaccount/{CUR1,CUR2,CUR3,etc.}
    #

    # Parse out base subaccount for out dir
    #   (we'll pass entire string to internal fetch impl)
    # TODO: parse/cut before setting var
    [ -z "$global_child_profile_flow" ] && lib_utils::die_fatal

    local _sub
    _sub="$(echo $_api_subaccount | cut -d'/' -f1)"
    local _api_out_dir="${global_child_profile_flow}/import/${global_child_profile}/${_account}/${_sub}/1-in/${global_arg_year}"

    lib_utils::print_custom "\n"
    lib_utils::print_info "Fetching '${_account}' in year '${global_arg_year}' for '${global_parent_profile}/${global_child_profile}' ..."
    lib_utils::print_custom "\n"

    #
    # Chain (blockchain) / addresses provided
    #

    # TODO: clarify, add more/better comments

    # If subaccount is list of addresses, create 'blockchain:address' format (for internal fetch impl)
    if echo "$_api_subaccount" | head -n2 | grep ":*$" | tail -n1 | grep -q "^- "; then

      # Parse out the separate blockchains
      local _blockchain
      _blockchain="$(echo "$_api_subaccount" | grep -E ":$" | cut -d':' -f1)"

      local _parsed_csv
      _parsed_csv=$(
        echo "$_blockchain" | while read _key; do
          local _addresses
          _addresses=$(echo "$_api_subaccount" | yq -M -y --indentless -e ".${_key}") # TODO: use lib
          echo "$_addresses" | while read _value; do
            echo "${_key}:${_value}" | sed -e 's/:- /\//g' -e "s:'::g"
          done
        done
      )

      _api_subaccount=$(
        echo "$_parsed_csv" | while read _line; do
          if [ -z "${global_arg_chain[*]}" ]; then
            echo "$_line"
          else
            for _arg in "${global_arg_chain[@]}"; do
              echo "$_line" | grep -i "$_arg"
            done
          fi
        done | paste -d',' -s
      )

      # Reset API out dir
      # TODO: HACK: gives dummy subaccount which indicates blockchain subaccount
      _api_out_dir="${global_child_profile_flow}/import/${global_child_profile}/${_account}/:/1-in/${global_arg_year}"
    fi

    #
    # Execute
    #

    # TODO: new approach to passing environment to internal impl.
    #   (there are limits: https://man7.org/linux/man-pages/man2/execve.2.html)

    API_KEY="$_api_key" \
      API_PASSPHRASE="$_api_passphrase" \
      API_SECRET="$_api_secret" \
      API_SUBACCOUNT="$_api_subaccount" \
      API_OUT_DIR="$_api_out_dir" \
      API_FETCH_YEAR="$global_arg_year" \
      lib_fetch::__fetch_exec "account" "$_account"

    local _return=$?
    if [ $_return -eq 0 ]; then
      lib_utils::print_custom "\n"
      lib_utils::print_info "Fetching '$_account' completed successfully!"
    else
      return $_return
    fi
  done

  if [ $? -eq 0 ]; then
    lib_utils::print_custom "\n"
    lib_utils::print_info "Done!"
  fi
}

function lib_fetch::__parse_price()
{
  local _master_prices="$1"
  lib_utils::print_debug "$_master_prices"

  # Parse master file into respective years
  [ ! -f "$_master_prices" ] && lib_utils::die_fatal "Master prices not found!"

  #
  # Master prices line format:
  #
  #   P 2023/01/01 USD 1
  #

  lib_utils::print_normal "\n  ─ Parsing master prices file"

  # Temporary storage
  local _tmp_dir
  _tmp_dir="$(mktemp -d -p /tmp docker-finance_XXX)"

  lib_utils::print_custom "    \e[32m│\e[0m\n"
  echo "$(<$_master_prices)" \
    | gawk '{print $2}' FS=" |/" \
    | sort -u \
    | while read _year; do

      lib_utils::print_custom "    \e[32m├─\e[34m\e[1;3m $_year \e[0m\n"

      # Ensure to only process given year(s) (in case underlying impl doesn't)
      if [[ "$global_arg_year" == "all" || "$_year" == "$global_arg_year" ]]; then

        # Prices year location
        local _sub_dir
        _sub_dir="$(dirname $_master_prices)/${_year}"
        [ ! -d "$_sub_dir" ] && mkdir -p "$_sub_dir"

        # Prices year journal
        local _journal="${_sub_dir}/prices.journal"

        # Newly fetched prices for given year
        local _tmp_file
        _tmp_file="$(mktemp -p $_tmp_dir ${_year}_XXX)"

        lib_utils::print_debug "$_tmp_file"
        grep " ${_year}/" "$_master_prices" >"$_tmp_file"

        # If journal exists, remove lines that match first 3 columns of fetched
        [ -f "$_journal" ] && echo "$(<$_tmp_file)" \
          | cut -d' ' -f-3 \
          | sort -u \
          | while read _line; do
            lib_utils::print_debug "$_line"
            # NOTE: end space needed or else USD* will be deleted
            sed -i "\:^${_line} :d" "$_journal"
          done

        # Add newly fetched lines to journal and sort
        echo "$(<$_tmp_file)" >>"$_journal"
        lib_utils::print_debug "$_journal"
        sort -u -o "$_journal" "$_journal"
      fi
    done

  local _return=$?

  # Cleanup
  [ -d "$_tmp_dir" ] && rm -fr "$_tmp_dir"

  return $?
}

function lib_fetch::__fetch_price()
{
  # Reconstruct global API arg into only available API(s)
  local _arg_api=()

  #
  # Test configuration for supported members
  #

  for _api in "${global_arg_api[@]}"; do

    if [[ $(lib_fetch::__parse_yaml "get-value" "price.${_api}" 2>/dev/null) == "null" ]]; then
      lib_utils::print_warning "price.${_api} is not set"
      continue
    fi

    local _members=("key" "asset")
    for _member in "${_members[@]}"; do

      local _value
      _value="$(lib_fetch::__parse_yaml "get-value" "price.${_api}.${_member}" 2>/dev/null)"

      case "$_member" in
        key)
          if [[ "$_value" == "null" ]]; then
            lib_utils::print_warning "price.${_api}.${_member} is not set"
          fi
          ;;
        asset)
          if [[ "$_value" =~ (^null$|^[^- ]) ]]; then
            lib_utils::die_fatal "${_api} has invalid asset sequence"
          fi
          ;;
        *)
          lib_utils::die_fatal "price.${_api}.${_member} is invalid"
          ;;
      esac

    done

    # Available API(s)
    _arg_api+=("$_api")

  done

  [ ${#_arg_api[@]} -eq 0 ] && lib_utils::die_fatal "No price API has been set"

  #
  # Prepare given assets and filesystem layout
  #

  # Prices master file
  # NOTE: base dir of all price files (subdirs will be by year)
  local -r _prices_dir="${global_child_profile_flow}/prices"
  [ ! -d "$_prices_dir" ] && mkdir -p "$_prices_dir"
  local -r _master_prices="${_prices_dir}/master.prices"

  for _api in "${_arg_api[@]}"; do

    # Get API key from configuration
    local _key
    _key="$(lib_fetch::__parse_yaml "get-value" "price.${_api}.key")"

    # Get assets from configuration
    if [ -z "${global_arg_price[*]}" ]; then
      # Parse out each asset
      while read _line; do
        local _assets+=("$_line")
      done < <(lib_fetch::__parse_yaml "get-values" "price.${_api}.asset" | sed 's:^- ::')
    else
      # Get assets from CLI
      local _assets=("${global_arg_price[*]}")
    fi

    lib_utils::print_custom "\n"
    lib_utils::print_info \
      "Fetching prices from '${_api}' in year '${global_arg_year}' for '${global_parent_profile}/${global_child_profile}' ..."
    lib_utils::print_custom "\n"

    #
    # Execute fetch for master prices file creation
    #

    declare -x API_FETCH_YEAR="$global_arg_year"
    lib_utils::print_debug "$API_FETCH_YEAR"

    declare -x API_PRICES_PATH="$_master_prices"
    lib_utils::print_debug "$API_PRICES_PATH"

    declare -x API_PRICES_API="$_api"
    lib_utils::print_debug "$API_PRICES_API"
    unset _api

    declare -x API_PRICES_KEY="$_key"
    lib_utils::print_debug "$API_PRICES_KEY"
    unset _key

    declare -x API_PRICES_ASSETS="${_assets[*]}"
    lib_utils::print_debug "$API_PRICES_ASSETS"
    unset _assets

    lib_fetch::__fetch_exec "prices" "crypto" || return $?

    #
    # Parse master prices file into final journal
    #

    lib_fetch::__parse_price "$API_PRICES_PATH" || return $?

  done

  local -r _return=$?

  if [ $_return -eq 0 ]; then
    lib_utils::print_custom "    \e[32m│\e[0m\n"
    lib_utils::print_custom "\n"
    lib_utils::print_info "Done!"
  fi

  # Cleanup
  [ -f "$API_PRICES_PATH" ] && rm -f "$API_PRICES_PATH"

  return $_return
}

function lib_fetch::__parse_yaml()
{
  [[ -z "$1" || -z "$2" ]] && lib_utils::die_fatal
  local _action="$1"
  local _append="$2"

  [ -z "$global_conf_fetch" ] && lib_utils::die_fatal
  local -r _raw=".${global_parent_profile}.${global_child_profile}.${_append}"

  # `yq` (kislyuk's) requires quotes around keys/members that contain '-'
  local _arg="$_raw"
  [[ "$_arg" =~ '-' ]] && _arg="$(sed -e 's:\.:"\.":g' -e 's:"\.:\.:' -e "s: *$:\":" <(echo "$_raw"))"

  # Pseudo-substitution of `shyaml` functionality
  local _ifs="$IFS"
  IFS=' '
  local -r _cmd=("yq" "-M" "-y" "--indentless" "-e" "$_arg" "$global_conf_fetch")
  local -r _grep=("grep" "-v" "^\.\.\.$")
  case "$_action" in
    get-value)
      "${_cmd[@]}" | "${_grep[@]}" -m2
      ;;
    get-values)
      "${_cmd[@]}" | "${_grep[@]}"
      ;;
    *)
      lib_utils::die_fatal "Unsupported YAML action"
      ;;
  esac
  IFS="$_ifs"
}

function lib_fetch::__fetch_exec()
{
  local _type="$1"
  local _subtype="$2"
  [ -z "$_type" ] && lib_utils::die_fatal
  [ -z "$_subtype" ] && lib_utils::die_fatal

  # `fetch` deps location (PHP)
  local _deps="/usr/local/lib/php"

  # `fetch` impl location (PHP)
  local _impl="${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/internal/fetch"

  # NOTE: must reset IFS after executing
  local _ifs="$IFS"
  IFS=' '

  #
  # Setup fetch
  #

  local _exec=()
  if [[ "$global_arg_tor" =~ ^on$|^true$ ]]; then
    lib_utils::deps_check "proxychains"
    _exec+=("proxychains") # Wrap with proxychains
  fi

  # Test for Tor plugin
  if [ ! -z "${_exec[*]}" ]; then
    lib_utils::deps_check "curl"
    if ! "${_exec[@]}" curl -s "https://check.torproject.org" &>/dev/null; then
      lib_utils::die_fatal "Tor plugin not started (or needs restart)"
    fi
  fi

  # TODO: remove no-limit after internal fetching writes per-paginated instead of per-fetch
  lib_utils::deps_check "php"
  _exec+=("php" "-d" "memory_limit=\"-1\"" "-d" "include_path=\"${_deps}:${_impl}\"")
  lib_utils::print_debug "${_exec[@]}"

  #
  # Execute fetch
  #

  __DFI_PHP_DEPS__="${_deps}" __DFI_PHP_ROOT__="${_impl}" "${_exec[@]}" "${_impl}/fetch.php" "$_type" "$_subtype"
  local -r _return=$?

  IFS="$_ifs"
  return $_return
}

# vim: sw=2 sts=2 si ai et
