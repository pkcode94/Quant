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

[ -z "$DOCKER_FINANCE_CLIENT_REPO" ] && exit 1

#
# "Libraries"
#

source "${DOCKER_FINANCE_CLIENT_REPO}/client/src/docker/lib/internal/lib_docker.bash" || exit 1
source "${DOCKER_FINANCE_CLIENT_REPO}/container/src/finance/lib/internal/lib_utils.bash" || exit 1

#
# Implementation
#

function lib_doxygen::__parse_args()
{
  [ -z "$global_usage" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_2" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_3" ] && lib_utils::die_fatal

  # TODO: pass `doxygen` arguments to gen
  local -r _usage="
\e[32mDescription:\e[0m

  Manage Doxygen documentation

\e[32mUsage:\e[0m

  $ $global_usage <gen|rm>

\e[32mExamples:\e[0m

  \e[37;2m# Generate Doxygen\e[0m
  $ $global_usage gen

  \e[37;2m# Remove previously generated Doxygen\e[0m
  $ $global_usage rm
"

  [[ $# -eq 0 || $# -gt 1 ]] && lib_utils::die_usage "$_usage"

  for _arg in "$@"; do
    [[ ! "$_arg" =~ ^gen$|^rm$ ]] \
      && lib_utils::die_usage "$_usage"
  done

  declare -gr global_arg_cmd="$1"
}

function lib_doxygen::__doxygen()
{
  [ -z "$DOCKER_FINANCE_CLIENT_REPO" ] && lib_utils::die_fatal
  local -r _dir="${DOCKER_FINANCE_CLIENT_REPO}/client/Doxygen"

  case "$global_arg_cmd" in
    gen)
      lib_docker::__run "doxygen ${_dir}/Doxyfile \
        && echo -e \"\nNow, open your browser to:\n\n  file://${_dir}/html/index.html\n\""
      ;;
    rm)
      lib_docker::__run "rm --verbose -fr ${_dir}/html"
      ;;
  esac
}

function lib_doxygen::doxygen()
{
  lib_doxygen::__parse_args "$@"
  lib_doxygen::__doxygen
}

# vim: sw=2 sts=2 si ai et
