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
# Execute
#

if [ $UID -lt 1000 ]; then
  [ $UID -eq 0 ] \
    && lib_utils::die_fatal "Cannot run as root!" \
    || lib_utils::print_warning "Running as system user"
fi

# IMPORTANT: keep umask for security
umask go-rwx

# Top-level caller
global_basename="dfi"
declare -rx global_basename

# Dependencies
if [ -z "$EDITOR" ]; then
  lib_utils::die_fatal "Shell EDITOR is not set, export EDITOR in your shell"
fi

# Globals (argument delimiters)
# NOTE: argument parsing will use this unified format
[ -z "$global_arg_delim_1" ] && declare -rx global_arg_delim_1="/"
[ -z "$global_arg_delim_2" ] && declare -rx global_arg_delim_2="="
[ -z "$global_arg_delim_3" ] && declare -rx global_arg_delim_3=","

# "Constructor" for finance environment
function lib_finance::finance()
{
  # Ensure profile/subprofile
  [[ ! $1 =~ $global_arg_delim_1 ]] && return 2
  IFS="$global_arg_delim_1" read -ra _profile <<<"$1"
  declare -grx global_parent_profile="${_profile[0]}"
  declare -grx global_child_profile="${_profile[1]}"

  # Subcommand
  declare -grx global_arg_subcommand="$2"

  # Globals (convenience)
  declare -grx global_usage="$global_basename ${global_parent_profile}${global_arg_delim_1}${global_child_profile} $global_arg_subcommand"

  # Globals (implementation)
  [ -z "$DOCKER_FINANCE_CONTAINER_FLOW" ] && lib_utils::die_fatal
  declare -grx global_child_profile_flow="${DOCKER_FINANCE_CONTAINER_FLOW}/profiles/${global_parent_profile}/${global_child_profile}"

  if [ ! -d "$global_child_profile_flow" ]; then
    lib_utils::die_fatal "Profiles '${global_parent_profile}${global_arg_delim_1}${global_child_profile}' do not exist!"
  fi

  declare -grx global_child_profile_journal="${global_child_profile_flow}/all-years.journal"
  declare -grx global_hledger_cmd=("hledger" "-f" "$global_child_profile_journal")

  # Globals (configuration)
  declare -grx global_conf_fetch="${global_child_profile_flow}/conf.d/fetch/fetch.yaml"
  declare -grx global_conf_hledger="${global_child_profile_flow}/conf.d/hledger/hledger.conf"
  declare -grx global_conf_meta="${global_child_profile_flow}/conf.d/meta/meta.csv"
  declare -grx global_conf_subscript="${global_child_profile_flow}/conf.d/shell/subscript.bash"
  declare -grx global_conf_visidata="${global_child_profile_flow}/conf.d/visidata" # dir for config/addons

  # Implementation "libraries" (requires previously set globals)
  source "${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/internal/lib_edit.bash" || exit 1
  source "${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/internal/lib_fetch.bash" || exit 1
  source "${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/internal/lib_hledger.bash" || exit 1
  source "${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/internal/lib_meta.bash" || exit 1
  source "${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/internal/lib_plugins.bash" || exit 1
  source "${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/internal/lib_reports.bash" || exit 1
  source "${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/internal/lib_root.bash" || exit 1
  source "${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/internal/lib_taxes.bash" || exit 1
  source "${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/internal/lib_times.bash" || exit 1
}

#
# Library implementation facades
#

# The "all" command which fetches, imports, generates taxes and reports for given year
function lib_finance::all()
{
  time lib_finance::__all "$@"
  lib_utils::catch $?
}

function lib_finance::__all()
{
  local _arg="$1"
  local _usage="
\e[32mDescription:\e[0m

  The 'all' command which fetches and imports accounts, and then generates taxes and reports

\e[32mUsage:\e[0m

  $ $global_usage year${global_arg_delim_2}<year>

\e[32mExamples:\e[0m

  \e[37;2m# Fetch, import, generates taxes and reports for year 2023\e[0m
  $ $global_usage year${global_arg_delim_2}2023
"

  if [ "$#" -gt 1 ]; then
    lib_utils::die_usage "$_usage"
  fi

  # Get/Set year
  if [ ! -z "$_arg" ]; then
    if [[ ! "$_arg" =~ ^year[s]?${global_arg_delim_2} ]]; then
      lib_utils::die_usage "$_usage"
    fi

    # Parse key for value
    local _key="${_arg%${global_arg_delim_2}*}"
    local _len="$((${#_key} + 1))"
    if [[ "$_key" =~ ^year[s]?$ ]]; then
      local _arg_year="${_arg:${_len}}"
      if [ -z "$_arg_year" ]; then
        lib_utils::die_usage "$_usage"
      fi
    fi
  else
    _arg_year="$(date +%Y)"
  fi

  # Execute
  lib_utils::print_custom "\n\t\e[1mFetching 'all' for year ${_arg_year}\e[0m\n"

  (lib_finance::fetch year${global_arg_delim_2}"${_arg_year}" all=all) \
    && (lib_finance::import year${global_arg_delim_2}"${_arg_year}") \
    && (lib_finance::taxes year${global_arg_delim_2}"${_arg_year}" all=all) \
    && (lib_finance::reports year${global_arg_delim_2}"${_arg_year}" all=all)

  [ $? -eq 0 ] \
    && lib_utils::print_custom "\n\t\e[1mFetching 'all' for year ${_arg_year} completed successfully!\e[0m\n"

  return $?
}

function lib_finance::meta()
{
  lib_meta::meta "$@"
  lib_utils::catch $?
}

function lib_finance::fetch()
{
  lib_fetch::fetch "$@"
  lib_utils::catch $?
}

function lib_finance::import()
{
  lib_hledger::hledger-import "$@"
  lib_utils::catch $?
}

function lib_finance::hledger()
{
  lib_hledger::hledger-cli "$@"
  lib_utils::catch $?
}

function lib_finance::hledger-ui()
{
  lib_hledger::hledger-ui "$@"
  lib_utils::catch $?
}

function lib_finance::hledger-vui()
{
  lib_hledger::hledger-vui "$@"
  lib_utils::catch $?
}

function lib_finance::hledger-web()
{
  lib_hledger::hledger-web "$@"
  lib_utils::catch $?
}

function lib_finance::plugins()
{
  lib_plugins::plugins "$@"
  lib_utils::catch $?
}

function lib_finance::reports()
{
  lib_reports::reports "$@"
  lib_utils::catch $?
}

function lib_finance::root()
{
  lib_root::root "$@"
  lib_utils::catch $?
}

function lib_finance::taxes()
{
  lib_taxes::taxes "$@"
  lib_utils::catch $?
}

function lib_finance::times()
{
  lib_times::times "$@"
  lib_utils::catch $?
}

function lib_finance::edit()
{
  lib_edit::edit "$@"
  lib_utils::catch $?
}

# vim: sw=2 sts=2 si ai et
