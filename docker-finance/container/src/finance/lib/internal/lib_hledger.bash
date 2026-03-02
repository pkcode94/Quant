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

function lib_hledger::hledger-import()
{
  lib_hledger::__hledger "$@"
  lib_hledger::__hledger-import "$@"
  lib_utils::catch $?
}

function lib_hledger::hledger-cli()
{
  lib_hledger::__hledger "$@"
  lib_hledger::__hledger-cli "$@"
  lib_utils::catch $?
}

function lib_hledger::hledger-ui()
{
  lib_hledger::__hledger "$@"
  lib_hledger::__hledger-ui "$@"
  lib_utils::catch $?
}

function lib_hledger::hledger-vui()
{
  lib_hledger::__hledger "$@"
  lib_hledger::__hledger-vui "$@"
  lib_utils::catch $?
}

function lib_hledger::hledger-web()
{
  lib_hledger::__hledger "$@"
  lib_hledger::__hledger-web "$@"
  lib_utils::catch $?
}

#
# Implementation
#

# Constructor
function lib_hledger::__hledger()
{
  # Base arguments to hledger suite before end-user added
  [ -z "$global_conf_hledger" ] && lib_utils::die_fatal
  [ -z "$global_child_profile_journal" ] && lib_utils::die_fatal

  declare -g global_base_args=("--conf" "$global_conf_hledger" "-f" "$global_child_profile_journal")
  lib_utils::print_debug "${global_base_args[*]}" "$@"
}

function lib_hledger::__parse_hledger-import()
{
  [ -z "$global_usage" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_1" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_2" ] && lib_utils::die_fatal

  local -r _arg="$1"
  local -r _usage="
\e[32mDescription:\e[0m

  Import all accounts' CSV data and produce viable journals

\e[32mUsage:\e[0m

  $ $global_usage [year${global_arg_delim_2}<year>]

\e[32mArguments:\e[0m

  Import all accounts since given year:

    year${global_arg_delim_2}<year>

\e[32mExamples:\e[0m

  \e[37;2m# Import all years since 2018\e[0m
  $ $global_usage year${global_arg_delim_2}2018
"

  if [ "$#" -gt 1 ]; then
    lib_utils::die_usage "$_usage"
  fi

  if [ ! -z "$_arg" ]; then
    if [[ ! "$_arg" =~ ^year[s]?${global_arg_delim_2} ]]; then
      lib_utils::die_usage "$_usage"
    fi

    local _key="${_arg%${global_arg_delim_2}*}"
    local _len="$((${#_key} + 1))"
    if [[ "$_key" =~ ^year[s]?$ ]]; then
      local _arg_year="${_arg:${_len}}"
      if [ -z "$_arg_year" ]; then
        lib_utils::die_usage "$_usage"
      fi
    fi
  fi

  # Arg: year
  # TODO: implement range
  if [ ! -z "$_arg_year" ]; then
    # TODO: 20th century support
    [[ ! "$_arg_year" =~ ^20[0-9][0-9]$ ]] \
      && lib_utils::die_usage "$_usage" \
      || declare -gr global_arg_year="$_arg_year"
  else
    global_arg_year="$(date +%Y)"
    declare -gr global_arg_year
  fi
}

function lib_hledger::__hledger-import()
{
  lib_hledger::__parse_hledger-import "$@"

  time hledger-flow import \
    "$(dirname $global_child_profile_journal)" \
    --start-year "$global_arg_year"
}

function lib_hledger::__hledger-cli()
{
  [ -z "${global_base_args[*]}" ] && lib_utils::die_fatal

  hledger "${global_base_args[@]}" "$@"
}

function lib_hledger::__hledger-ui()
{
  [ -z "${global_base_args[*]}" ] && lib_utils::die_fatal

  hledger-ui "${global_base_args[@]}" "$@"
}

function lib_hledger::__hledger-vui()
{
  [ -z "${global_base_args[*]}" ] && lib_utils::die_fatal
  [ -z "$global_conf_visidata" ] && lib_utils::die_fatal

  local -r _visidata=("--visidata-dir" "$global_conf_visidata" "--motd-url" "file:///dev/null" "--filetype" "csv")
  local -r _hledger=("hledger" "${global_base_args[@]}" "print" "-O" "csv" "$@")

  # If hledger command is valid, output into visidata
  if "${_hledger[@]}" 1>/dev/null; then
    visidata "${_visidata[@]}" < <("${_hledger[@]}")
  fi
}

function lib_hledger::__hledger-web()
{
  [ -z "${global_base_args[*]}" ] && lib_utils::die_fatal

  local -r _url="http://127.0.0.1:5000"

  hledger-web "${global_base_args[@]}" \
    --serve --host=0.0.0.0 --base-url "$_url" --allow=view "$@" 1>/dev/null &

  local -r _pid=$!
  sleep 3s

  if ! ps -p "$_pid" 1>/dev/null; then
    lib_utils::die_fatal "hledger-web failed. See above error message"
  else
    lib_utils::print_info "hledger-web started (PID ${_pid}). Point web browser to $_url"
  fi

}

# vim: sw=2 sts=2 si ai et
