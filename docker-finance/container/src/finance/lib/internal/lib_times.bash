#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2024 Aaron Fiore (Founder, Evergreen Crypto LLC)
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

#
# "Libraries"
#

[ -z "$DOCKER_FINANCE_CONTAINER_REPO" ] && exit 1
source "${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/internal/lib_utils.bash" || exit 1

#
# Facade
#

function lib_times::times()
{
  lib_times::__parse_args "$@"
  lib_times::__times "$@"
  lib_utils::catch $?
}

#
# Implementation
#

function lib_times::__parse_args()
{
  [ -z "$global_usage" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_1" ] && lib_utils::die_fatal
  [ -z "$global_child_profile" ] && lib_utils::die_fatal
  [ -z "$global_parent_profile" ] && lib_utils::die_fatal

  local -r _usage="
\e[32mDescription:\e[0m

  Keep track of time on a per-subprofile basis

\e[32mUsage:\e[0m

  $ $global_usage <cmd> [args]

\e[32mExamples:\e[0m

  \e[37;2m# Start the timer for '${global_parent_profile}${global_arg_delim_1}${global_child_profile}' with description 'Task 1' of type 'consulting' and comment 'My long comment'\e[0m
  $ $global_usage start desc='Task 1' comment='My long comment' tag:type=consulting

  \e[37;2m# Stop the timer for '${global_parent_profile}${global_arg_delim_1}${global_child_profile}'\e[0m
  $ $global_usage stop

  \e[37;2m# Print summary for '${global_parent_profile}${global_arg_delim_1}${global_child_profile}' of all times related to tag:type=consulting\e[0m
  $ $global_usage summary tag:type=consulting

  \e[37;2m# Export all times and all tags for '${global_parent_profile}${global_arg_delim_1}${global_child_profile}'\e[0m
  $ $global_usage export

  \e[37;2m# List all available commands\e[0m
  $ $global_usage help
"

  if [ "$#" -eq 0 ]; then
    lib_utils::die_usage "$_usage"
  fi
}

function lib_times::__times()
{
  [ -z "$DOCKER_FINANCE_CONTAINER_FLOW" ] && lib_utils::die_fatal

  # NOTE:
  #   - Currently a wrapper to `timew`
  #
  #   - Automatically tags profile/subprofile
  #     * keeps time organized on a per-subprofile basis
  #
  #   - All time-related data is stored in '${_path}'
  #     * `timew` database is stored in '${_timewdb}'

  local -r _path="${DOCKER_FINANCE_CONTAINER_FLOW}/times"
  local -r _timewdb="${_path}/timew"

  local _command=("timew")
  lib_utils::deps_check "${_command[*]}"

  local -r _tag="${global_parent_profile}${global_arg_delim_1}${global_child_profile}"

  case "$1" in
    help)
      _command+=("$@")
      ;;
    *)
      _command+=("$@")
      _command+=("profile=${_tag}")
      ;;
  esac

  lib_utils::print_debug "TIMEWARRIORDB=${_timewdb}"
  TIMEWARRIORDB="${_timewdb}" "${_command[@]}"

  return $?
}

# vim: sw=2 sts=2 si ai et
