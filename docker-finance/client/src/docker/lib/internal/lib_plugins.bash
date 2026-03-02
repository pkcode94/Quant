#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2024,2026 Aaron Fiore (Founder, Evergreen Crypto LLC)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation  either version 3 of the License  or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not  see <https://www.gnu.org/licenses/>.

[ -z "$DOCKER_FINANCE_CLIENT_REPO" ] && exit 1

#
# "Libraries"
#

# Utilities (a container library (but container is never exposed to client code))
source "${DOCKER_FINANCE_CLIENT_REPO}/container/src/finance/lib/internal/lib_utils.bash" || exit 1

#
# Facade
#

function lib_plugins::plugins()
{
  lib_plugins::__parse_args "$@"
  lib_plugins::__plugins "$@"
  lib_utils::catch $?
}

#
# Implementation
#

function lib_plugins::__parse_args()
{
  #
  # Expected format:
  #
  #   $global_usage repo${global_arg_delim_1}file [args]
  #     - Executes plugin in repository
  #
  #   $global_usage custom${global_arg_delim_1}file [args]
  #     - Executes plugin outside of repository
  #

  [ -z "$global_usage" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_1" ] && lib_utils::die_fatal

  [ -z "$DOCKER_FINANCE_CLIENT_REPO" ] && lib_utils::die_fatal
  local -r _repo="${DOCKER_FINANCE_CLIENT_REPO}/client/plugins/docker"
  [ ! -d "$_repo" ] && lib_utils::die_fatal

  [ -z "$DOCKER_FINANCE_CLIENT_PLUGINS" ] && lib_utils::die_fatal
  local -r _custom="${DOCKER_FINANCE_CLIENT_PLUGINS}/client/docker"
  [ ! -d "$_custom" ] && lib_utils::die_fatal

  local -r _usage="
\e[32mDescription:\e[0m

  Execute a categorical plugin

\e[32mUsage:\e[0m

  $ $global_usage <help | [TAB COMPLETION]> [args]

\e[32mArguments:\e[0m

  [None | help]: show this usage help

  [TAB COMPLETION]: run given shell plugin

    custom = custom plugins in custom plugin location
    repo   = repository plugins in repository location

  [args]: arguments to plugin

\e[32mExamples:\e[0m

  \e[37;2m# See this usage help\e[0m
  $ $global_usage help

  \e[37;2m# The output of tab completion\e[0m
  $ $global_usage \\\t\\\t
    custom/example.bash  help                 repo/bitcoin.bash    repo/example.bash    repo/tor.bash

  \e[37;2m# Execute a repository shell plugin in '${_repo}'\e[0m
  $ $global_usage repo${global_arg_delim_1}example.bash \"I'm in repo\"

  \e[37;2m# Execute a custom shell plugin in '${_custom}'\e[0m
  $ $global_usage custom${global_arg_delim_1}example.bash \"I'm in custom\"
"

  [ $# -eq 0 ] && lib_utils::die_usage "$_usage"

  local -r _arg="$1"

  [[ ! "$_arg" =~ (^repo${global_arg_delim_1}|^custom${global_arg_delim_1}) ]] && lib_utils::die_usage "$_usage"

  local -r _key="${_arg%%${global_arg_delim_1}*}"
  local -r _len="$((${#_key} + 1))"

  local -r _arg_type="${_arg:${_len}}"
  [ -z "$_arg_type" ] && lib_utils::die_usage "$_usage"

  local _path
  case "$_key" in
    repo)
      _path="${_repo}/${_arg_type}"
      ;;
    custom)
      _path="${_custom}/${_arg_type}"
      ;;
    *)
      lib_utils::die_usage "$_usage"
      ;;
  esac
  declare -gr global_arg_path="$_path"
}

function lib_plugins::__plugins()
{
  exec $global_arg_path "${@:2}"
  return $?
}

# vim: sw=2 sts=2 si ai et
