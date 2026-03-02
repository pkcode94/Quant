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
# Printing
#

function lib_utils::print_custom()
{
  echo -e -n "$@" # NOTE: no newline
}

function lib_utils::print_normal()
{
  lib_utils::__print "\e[0m" "$*"
}

function lib_utils::print_info()
{
  lib_utils::__print "\e[32;1m" "[INFO] $*"
}

function lib_utils::print_warning()
{
  lib_utils::__print "\e[33;1m" "[WARN] $*" >&2
}

function lib_utils::print_error()
{
  lib_utils::__print "\e[31;1m" "[ERROR] $*" >&2
}

function lib_utils::print_fatal()
{
  lib_utils::__print "\e[41;1m" "[FATAL] $*" >&2
}

function lib_utils::print_debug()
{
  # shellcheck disable=SC2154
  if [[ "$DOCKER_FINANCE_DEBUG" =~ ^1$|^2$ ]]; then
    local _debug="${BASH_SOURCE[1]##*/}:${BASH_LINENO[0]} -> ${FUNCNAME[1]}"
    lib_utils::__print "\e[33m" "[DEBUG] ${_debug} -> $*"
  else
    return 0
  fi
}

function lib_utils::__print()
{
  local _color="$1"
  local _message="$2"
  echo -e "${_color}${_message}\e[0m"
}

#
# Generic error handler
#

function lib_utils::die_usage()
{
  local _message="$1"
  local _code=$2
  [ -z $_code ] && _code=2
  lib_utils::print_custom "${_message}\n" >&2
  exit "$_code"
}

function lib_utils::die_fatal()
{
  local _message="$1"
  local _code=$2
  [ -z "$_message" ] && _message="See source file for details"
  [ -z $_code ] && _code=1
  lib_utils::print_fatal \
    "\n\n  ${_message}\n\n${BASH_SOURCE[1]##*/}:${BASH_LINENO[0]} -> ${FUNCNAME[1]}\n"
  exit "$_code"
}

function lib_utils::catch()
{
  local _code=$1
  local _message="$2"
  local _debug="${BASH_SOURCE[1]##*/}:${BASH_LINENO[0]} -> ${FUNCNAME[1]}"
  if [ $_code -ne 0 ]; then
    if [ ! -z "$_message" ]; then
      lib_utils::print_error "${_debug}\n\n  ${_message}\n"
    else
      lib_utils::print_error "${_debug}"
    fi
  fi
  return $_code
}

#
# Dependencies checking
#

function lib_utils::deps_check()
{
  local _deps=("$@")
  for _dep in "${_deps[@]}"; do
    if ! hash "$_dep" &>/dev/null; then
      lib_utils::die_fatal "Missing dependency: '${_dep}'"
    fi
  done
}

# vim: sw=2 sts=2 si ai et
