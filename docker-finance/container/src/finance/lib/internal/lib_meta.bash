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

function lib_meta::meta()
{
  lib_meta::__parse_args "$@"
  lib_meta::__meta "$@"
  lib_utils::catch $?
}

#
# Implementation
#

function lib_meta::__parse_args()
{
  [ -z "$global_usage" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_1" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_2" ] && lib_utils::die_fatal

  [ -z "$global_parent_profile" ] && lib_utils::die_fatal
  [ -z "$global_child_profile" ] && lib_utils::die_fatal

  [ -z "$global_conf_meta" ] && lib_utils::die_fatal

  # NOTE: expects comments to begin with format: //!
  global_meta_header="$(grep -vE "^//!" $global_conf_meta | head -n1)"
  declare -gr global_meta_header

  local -r _usage="
\e[32mDescription:\e[0m

  Search and view financial metadata

\e[32mUsage:\e[0m

  $ $global_usage <column>${global_arg_delim_2}<pattern> [<column>${global_arg_delim_2}<pattern>...]

\e[32mAvailable columns:\e[0m

$(echo $global_meta_header | xan headers -)

\e[32mExamples:\e[0m

  \e[37;2m# View all P2SH Bitcoin addresses on Gemini exchange for ${global_parent_profile}${global_arg_delim_1}${global_child_profile}\e[0m
  $ $global_usage account${global_arg_delim_2}gemini ticker${global_arg_delim_2}btc format${global_arg_delim_2}p2sh
"

  [ $# -eq 0 ] && lib_utils::die_usage "$_usage"

  # Read arguments and file header
  IFS=' ' read -ra global_meta_args <<<"$@"
  IFS=',' read -ra _cols <<<"$global_meta_header" # WARNING: CSV expected, not TSV

  # Will store valid arguments for later implementation
  local _validated=()

  # Cycle through arguments and compare against existing file header
  for _arg in "${global_meta_args[@]}"; do

    # Parse key
    local _key="${_arg%${global_arg_delim_2}*}"

    # Validate args against actual header
    for _col in "${_cols[@]}"; do
      [[ "$_key" =~ $_col ]] && _validated+=("$_key")
    done

  done

  # Validated will be same size as arguments
  [[ ${#_validated[@]} -ne ${#global_meta_args[@]} ]] \
    && lib_utils::die_usage "$_usage"
}

function lib_meta::__meta()
{
  [ -z "$global_conf_visidata" ] && lib_utils::die_fatal

  lib_utils::deps_check "visidata"

  # TODO: can visidata regex multiple columns from the commandline?
  local _tmp_dir
  _tmp_dir="$(mktemp -d -p /tmp docker-finance_XXX)"

  local _base_file
  _base_file="$(mktemp -p $_tmp_dir file_XXX)"

  # Skip anything before header (e.g., file comments)
  sed -ne "/^${global_meta_header}/,$ p" "$global_conf_meta" >"$_base_file"

  # Parse and iterate through all given columns
  for _arg in "${global_meta_args[@]}"; do
    # TODO: need regexp anchor at end? Makes searching more difficult
    #   but will make columns clearer (Ex., coinbase versus coinbase-pro, etc.)
    # Nth iteration (select column and pattern)
    local _tmp_file
    _tmp_file="$(mktemp -p $_tmp_dir ${_arg}_XXX)"
    xan search -i -s \
      "${_arg%${global_arg_delim_2}*}" "${_arg#*${global_arg_delim_2}}" \
      "$_base_file" >"$_tmp_file"

    # Reset source
    _base_file="$_tmp_file"
  done

  # Display as stream
  local -r _visidata=("--visidata-dir" "$global_conf_visidata" "--motd-url" "file:///dev/null" "--filetype" "csv")
  visidata "${_visidata[@]}" < <(cat "$_base_file")

  # Enforce cleanup
  [ -d "$_tmp_dir" ] && rm -fr "$_tmp_dir"
}

# vim: sw=2 sts=2 si ai et
