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
source "${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/internal/lib_utils.bash" || exit 1

#
# Facade
#

function lib_edit::edit()
{
  lib_edit::__parse_args "$@"
  lib_edit::__edit
  lib_utils::catch $?
}

#
# Implementation
#

function lib_edit::__parse_args()
{
  [ -z "$global_usage" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_1" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_2" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_3" ] && lib_utils::die_fatal

  local -r _usage="
\e[32mDescription:\e[0m

  Edit container configurations, metadata and journals

\e[32mUsage:\e[0m

  $ $global_usage <type${global_arg_delim_2}<type>> [account${global_arg_delim_2}<account[${global_arg_delim_1}subaccount]>]

\e[32mArguments:\e[0m

  Configuration type:

    type${global_arg_delim_2}<fetch|hledger|{add|iadd|manual}|meta|{shell|subscript}|rules|preprocess>

  Account:

    account${global_arg_delim_2}<user-defined for rules|preprocess (see examples)>

\e[32mExamples:\e[0m

  \e[37;2m# Edit fetch configuration\e[0m
  $ $global_usage type${global_arg_delim_2}fetch

  \e[37;2m# Edit hledger configuration\e[0m
  $ $global_usage type${global_arg_delim_2}hledger

  \e[37;2m# Edit meta and subscript files\e[0m
  $ $global_usage type${global_arg_delim_2}meta${global_arg_delim_3}subscript

  \e[37;2m# Edit _manual_ journal for year 2023\e[0m
  $ $global_usage type${global_arg_delim_2}manual year=2023

  \e[37;2m# Edit subaccount rules for acccount${global_arg_delim_1}subaccount\e[0m
  $ $global_usage type${global_arg_delim_2}rules account${global_arg_delim_2}gemini${global_arg_delim_1}exchange

  \e[37;2m# Edit shared rules for multiple accounts\e[0m
  $ $global_usage type${global_arg_delim_2}rules account${global_arg_delim_2}electrum${global_arg_delim_3}ethereum-based

  \e[37;2m# Edit subaccount rules for multiple account${global_arg_delim_1}subaccount\e[0m
  $ $global_usage account${global_arg_delim_2}ledger${global_arg_delim_1}nano${global_arg_delim_3}trezor${global_arg_delim_1}model type${global_arg_delim_2}rules

  \e[37;2m# Edit subaccount preprocess and rules for multiple account${global_arg_delim_1}subaccount\e[0m
  $ $global_usage account${global_arg_delim_2}coinbase${global_arg_delim_1}platform${global_arg_delim_3}electrum${global_arg_delim_1}wallet-1${global_arg_delim_3}coinomi${global_arg_delim_1}wallet-1 type${global_arg_delim_2}preprocess${global_arg_delim_3}rules
"

  #
  # Ensure supported arguments
  #

  [ $# -eq 0 ] && lib_utils::die_usage "$_usage"

  for _arg in "$@"; do
    [[ ! "$_arg" =~ ^type[s]?${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^account[s]?${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^year[s]?${global_arg_delim_2} ]] \
      && lib_utils::die_usage "$_usage"
  done

  #
  # Parse arguments before testing
  #

  # Parse key for value
  for _arg in "$@"; do

    local _key="${_arg%${global_arg_delim_2}*}"
    local _len="$((${#_key} + 1))"

    if [[ "$_key" =~ ^type[s]?$ ]]; then
      local _arg_type="${_arg:${_len}}"
      [ -z "$_arg_type" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^account[s]?$ ]]; then
      local _arg_account="${_arg:${_len}}"
      [ -z "$_arg_account" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^year[s]?$ ]]; then
      local _arg_year="${_arg:${_len}}"
      [ -z "$_arg_year" ] && lib_utils::die_usage "$_usage"
    fi
  done

  #
  # Test for valid ordering/functionality of argument values
  #

  # Arg: account
  if [ ! -z "$_arg_account" ]; then
    if [ -z "$_arg_type" ]; then
      lib_utils::die_usage "$_usage"
    fi
  fi

  # Arg: year
  if [ ! -z "$_arg_year" ]; then
    if [[ -z "$_arg_type" ]]; then
      lib_utils::die_usage "$_usage"
    fi
  fi

  #
  # Test argument values, set globals
  #

  IFS="$global_arg_delim_3"

  # Arg: type
  if [ ! -z "$_arg_type" ]; then
    read -ra _read <<<"$_arg_type"

    for _type in "${_read[@]}"; do
      if [[ ! "$_type" =~ ^fetch$|^hledger$|^add$|^iadd$|^manual$|^meta$|^preprocess$|^rules$|^shell$|^subscript$ ]]; then
        lib_utils::die_usage "$_usage"
      fi
      if [[ ! -z "$_arg_account" ]]; then
        if [[ ! "$_type" =~ ^preprocess$|^rules$ ]]; then
          lib_utils::die_usage "$_usage"
        fi
      fi
      if [[ -z "$_arg_account" ]]; then
        if [[ "$_type" =~ ^preprocess$|^rules$ ]]; then
          lib_utils::die_usage "$_usage"
        fi
      fi
    done

    declare -gr global_arg_type=("${_read[@]}")
  fi

  # Arg: account
  if [ ! -z "$_arg_account" ]; then
    read -ra _read <<<"$_arg_account"
    declare -gr global_arg_account=("${_read[@]}")
  fi

  # Arg: year
  # TODO: implement range
  if [ ! -z "$_arg_year" ]; then
    # TODO: 20th century support
    if [[ ! "$_arg_year" =~ ^20[0-9][0-9]$ || ! ${global_arg_type[*]} =~ ^manual$ ]]; then
      lib_utils::die_usage "$_usage"
    fi
    declare -gr global_arg_year="$_arg_year"
  else
    global_arg_year="$(date +%Y)" # Set default current
    declare -gr global_arg_year
  fi
}

function lib_edit::__edit()
{
  [ -z "$EDITOR" ] && lib_utils::die_fatal
  lib_utils::deps_check "$(basename $EDITOR)"

  for _type in "${global_arg_type[@]}"; do
    case "$_type" in
      fetch)
        [ -z "$global_conf_fetch" ] && lib_utils::die_fatal

        local _dir
        _dir="$(dirname $global_conf_fetch)"
        [ ! -d "$_dir" ] && mkdir -p "$_dir"

        local _file
        _file="$(basename $global_conf_fetch)"

        local _path
        _path="${_dir}/${_file}"
        [ ! -f "$_path" ] && touch "$_path"

        $EDITOR "$_path" || lib_utils::die_fatal
        ;;
      hledger)
        [ -z "$global_conf_hledger" ] && lib_utils::die_fatal

        local _dir
        _dir="$(dirname $global_conf_hledger)"
        [ ! -d "$_dir" ] && mkdir -p "$_dir"

        local _file
        _file="$(basename $global_conf_hledger)"

        local _path
        _path="${_dir}/${_file}"
        [ ! -f "$_path" ] && touch "$_path"

        $EDITOR "$_path" || lib_utils::die_fatal
        ;;
      add | iadd | manual)
        [ -z "$global_child_profile" ] && lib_utils::die_fatal
        [ -z "$global_child_profile_flow" ] && lib_utils::die_fatal

        local _path="${global_child_profile_flow}/import/${global_child_profile}/_manual_/${global_arg_year}"
        [ ! -d "$_path" ] && mkdir -p "$_path"

        _path+="/post-import.journal"
        [ ! -f "$_path" ] && touch "$_path"

        case "$_type" in
          add)
            hledger -f "$_path" add || lib_utils::die_fatal
            ;;
          iadd)
            lib_utils::deps_check "hledger-iadd"
            hledger-iadd -f "$_path" || lib_utils::die_fatal
            ;;
          manual)
            $EDITOR "$_path" || lib_utils::die_fatal
            ;;
          *)
            lib_utils::die_fatal
            ;;
        esac
        ;;
      meta)
        [ -z "$global_conf_meta" ] && lib_utils::die_fatal
        [ -z "$global_conf_visidata" ] && lib_utils::die_fatal

        local _dir
        _dir="$(dirname $global_conf_meta)"
        [ ! -d "$_dir" ] && mkdir -p "$_dir"

        local _file
        _file="$(basename $global_conf_meta)"

        local _path
        _path="${_dir}/${_file}"
        [ ! -f "$_path" ] && touch "$_path"

        # NOTE:
        #  - Expects comments to begin with format: //!
        #  - If saved to original, opening with `--skip` will clobber the
        #    original file's comments.
        local -r _skip="$(grep -E "^//!" $_path | wc -l)"
        local -r _visidata=("--visidata-dir" "$global_conf_visidata" "--quitguard" "--motd-url" "file:///dev/null" "--filetype" "csv" "--skip" "$_skip" "$_path")

        lib_utils::deps_check "visidata"
        visidata "${_visidata[@]}" || lib_utils::die_fatal

        # TODO: HACK: visidata saves w/ DOS-style carriage...
        #   ...but there seems to be no option out of this.
        sed -i 's:\r::g' "$_path" || lib_utils::die_fatal
        ;;
      preprocess | rules)
        # Run all paths through one editor instance
        local _paths=()

        # Prepare account/account-shared.* or account/subaccount/account-subaccount.*
        for _account in "${global_arg_account[@]}"; do
          local _acct="${_account%${global_arg_delim_1}*}"

          # Attempting to parse out an empty arg will result in dup account as sub
          [[ "$_account" =~ ${global_arg_delim_1} ]] \
            && local _sub="${_account#*${global_arg_delim_1}}"

          local _path="${global_child_profile_flow}/import/${global_child_profile}/"
          [ ! -z "$_sub" ] && _path+="${_acct}/${_sub}/"

          case "$_type" in
            preprocess)
              [ -z "$_sub" ] \
                && local _file="${_acct}-shared.bash" \
                || local _file="$_type"
              ;;
            rules)
              [ -z "$_sub" ] \
                && local _file="${_acct}-shared.${_type}" \
                || local _file="${_acct}-${_sub}.${_type}"
              ;;
          esac

          _path+="$_file"
          if [ ! -f "$_path" ]; then
            lib_utils::die_fatal "File not found: $_path"
          else
            _paths+=("$_path")
          fi
        done

        # Execute
        $EDITOR "${_paths[@]}" || lib_utils::die_fatal
        ;;
      shell | subscript)
        [ -z "$global_conf_subscript" ] && lib_utils::die_fatal

        local _dir
        _dir="$(dirname $global_conf_subscript)"
        [ ! -d "$_dir" ] && mkdir -p "$_dir"

        local _file
        _file="$(basename $global_conf_subscript)"

        local _path
        _path="${_dir}/${_file}"
        [ ! -f "$_path" ] && touch "$_path"

        $EDITOR "$_path" || lib_utils::die_fatal
        ;;
    esac
  done
}

# vim: sw=2 sts=2 si ai et
