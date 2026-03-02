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

# Shared preprocess file to be used across all accounts
#
# 1. MUST be sourced from individual account preprocess files
# 2. MUST be passed current input path as its 1st argument
# 3. MUST be passed current output path as its 2nd argument

[[ -z "$1" || -z "$2" ]] && exit 1

#
# Globals
#

global_in_path="$1"

# shellcheck disable=SC2034
global_out_path="$2"

# in-dir of subaccount (holds years). Needed for various internal scripts (UTC date, etc.)
# shellcheck disable=SC2034
global_in_dir="$(dirname $global_in_path)"

# Account dir from path
global_account="$(echo $global_in_path | gawk -F"/" '{print $(NF-4)}')"

# Subaccount dir from path
global_subaccount="$(echo $global_in_path | gawk -F"/" '{print $(NF-3)}')"

# Year dir from path (parses and pass on data into appropriate year)
global_year="$(echo $global_in_path | gawk -F"/" '{print $(NF-1)}')"

# In-file filename (not path)
global_in_filename="$(basename $global_in_path)"

#
# Library (implementation)
#

# Print filename currently being processed
echo -e "    \e[32m│\e[0m"
echo -e "    \e[32m├─\e[34m\e[1;3m ${global_account}/${global_subaccount}\e[0m"
echo -e "    \e[32m│  └─\e[34;3m ${global_year}\e[0m"
echo -e "    \e[32m│     └─\e[37;2m ${global_in_filename}\e[0m"

# Test if column(s) exist in given CSV header
function lib_preprocess::test_header()
{
  local _column="$1"
  local _header
  _header="$(lib_preprocess::__sanitize_header $2)"
  [[ "$_header" =~ (^${_column}$|^${_column},|,${_column},|,${_column}$) ]] && return 0 || return 2
}

# Assert header in given CSV header
function lib_preprocess::assert_header()
{
  local _string="$1"
  local _header
  _header="$(lib_preprocess::__sanitize_header $2)"
  if [ "$_string" != "$_header" ]; then
    echo -e "
\e[41mFATAL: string does not match header in '${global_in_path}'\e[0m\n
STRING:\n
\t$_string
\n
$(echo $_string | hexdump -C)
\n
HEADER:\n
\t$_header
\n
$(echo $_header | hexdump -C)
\n" >&2
    exit 2
  fi
}

# Sanitize header because some CSVs have BOM or EOF CR
# TODO: this appears to break headers with SPACES within column
function lib_preprocess::__sanitize_header()
{
  # CSV file or header string
  local _header="$1"

  # Sanitizer
  local _sanitize=(sed -n -e $'1s:^\ufeff::g' -e 's:\x0d$::' -e '1p')

  # Default assume in-file
  [ -z "$_header" ] && _header="$global_in_path"

  # If header "doesn't exist" (impossible), a header string (from that file) was passed instead
  [ -f "$_header" ] && "${_sanitize[@]}" "$_header" || (echo "$_header" | "${_sanitize[@]}")
}

# vim: sw=2 sts=2 si ai et
