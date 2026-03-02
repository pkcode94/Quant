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

source "${DOCKER_FINANCE_CLIENT_REPO}/container/src/finance/lib/internal/lib_utils.bash" || exit 1

#
# Implementation
#

function lib_license::__parse_args()
{
  [ -z "$global_usage" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_2" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_3" ] && lib_utils::die_fatal

  local -r _usage="
\e[32mDescription:\e[0m

  Add license text to a given file

\e[32mUsage:\e[0m

  $ $global_usage file${global_arg_delim_2}<file>

\e[32mArguments:\e[0m

  File path:

    file${global_arg_delim_2}<path to file>

\e[32mExamples:\e[0m

  \e[37;2m# Add license to given file\e[0m
  $ $global_usage file${global_arg_delim_2}/path/to/file.ext
"

  #
  # Ensure supported arguments
  #

  [ $# -eq 0 ] && lib_utils::die_usage "$_usage"

  for _arg in "$@"; do
    [[ ! "$_arg" =~ ^file[s]?${global_arg_delim_2} ]] \
      && lib_utils::die_usage "$_usage"
  done

  #
  # Parse arguments before testing
  #

  # Parse key for value
  for _arg in "$@"; do
    local _key="${_arg%${global_arg_delim_2}*}"
    local _len="$((${#_key} + 1))"

    if [[ "$_key" =~ ^file[s]?$ ]]; then
      local _arg_file="${_arg:${_len}}"
      [ -z "$_arg_file" ] && lib_utils::die_usage "$_usage"
    fi
  done

  #
  # Test argument values, set globals
  #

  IFS="$global_arg_delim_3"

  # Arg: file
  if [ ! -z "$_arg_file" ]; then
    readonly global_arg_file="$_arg_file" # TODO: make array when multiple files implemented
  fi
}

function lib_license::__get_file_type()
{
  local -r _file="$(basename $1)"
  local -r _file_type="${_file##*.}"

  if [ -z "$_file_type" ]; then
    lib_utils::die_fatal "File type not detected for '${_file}'"
  elif [[ "$_file_type" == "in" ]]; then
    # Dockerfile may have OS/platform in filename
    if [[ "$_file" =~ Dockerfile ]]; then
      readonly global_file_type="docker"
    else
      # Get real extension of .in file
      # TODO: optimize: remove cut, use bash
      global_file_type="$(echo $_file | cut -d'.' -f2)"
      readonly global_file_type
    fi
  else
    readonly global_file_type="$_file_type"
  fi

  case "$global_file_type" in
    bash | yaml | yml | docker | rules | clang-format)
      readonly global_comment_type="#"
      ;;
    cc | hh | C | php) # .C = ROOT macro file
      readonly global_comment_type="//"
      ;;
    csv)
      readonly global_comment_type="//!"
      ;;
    md)
      # NOTE: impl MUST complete this comment (see below)
      readonly global_comment_type="[//]: # ("
      ;;
    *)
      lib_utils::die_fatal "Unsupported file type '${global_file_type}' for '${_file}'"
      ;;
  esac
}

function lib_license::__license()
{
  local _deps=("gawk")
  lib_utils::deps_check "${_deps[@]}"

  [ ! -f "$global_arg_file" ] \
    && lib_utils::die_fatal "File not found '${global_arg_file}'"

  lib_license::__get_file_type "$global_arg_file"

  # TODO: add year= and author= arg options
  local -r _text="docker-finance | modern accounting for the power-user

Copyright (C) <year> <author>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>."

  # NOTE: docker-finance `.in` files will have docker-finance version on 1st line
  case "$global_file_type" in
    cc | hh | C | yaml | yml | docker | rules | clang-format)
      # Has no expectations of input
      local _sed_args=("-e" "s\^\\$global_comment_type \g" "-e" "s: *$::g")
      local _awk_args=('BEGIN{print text"\n"}{print}')
      ;;
    bash | php | csv)
      local _sed_args=("-e" "s\^\\$global_comment_type \g" "-e" "s: *$::g")
      local _awk_args=(
        '{
           # Expects first 3 input lines as:
           #
           #  1  file tag (bash shebang, php opening tag, docker-finance version info, etc.)
           #  2
           #  3  code or text

           # Save first line if needed
           if (NR==1 && NF) {first = $0}

           # First line then license text
           if (NR<2)
             {
               if (global_file_type == "csv")
                 {
                   print text
                   print first
                 }
               else
                 {
                   print first
                   print "\n"text"\n"
                 }
             }

           # Remaining file
           if (NR>2) {print}
        }')
      ;;
    md)
      # - Create empty first line
      # - Replace pre-existing (' and ')' with '[' and ']'
      # - Add ')' to EOL
      local _sed_args=("-e" "s:(:[:g" "-e" "s:):]:g" "-e" "s\^\\${global_comment_type}\g" "-e" "s: *$:):g")
      local _awk_args=(
        '{
           # Save first line if needed
           if (NR==1 && NF) {first = $0}

           # Uncomment if text itself does not have newline at beginning
           #if (NR==1 && NF) {print ""}

           # License text then first line
           if (NR<2) {print "\n"text"\n"; print first"\n"}

           # Remaining file
           if (NR>2) {print}
        }')
      ;;
    *)
      lib_utils::die_fatal "Unsupported file type '${global_file_type}' for '${global_arg_file}'"
      ;;
  esac

  # Setup temp
  local -r _tmp_dir="$(mktemp -d -p /tmp docker-finance_XXX)"
  local -r _tmp_file="$(mktemp -p $_tmp_dir license_XXX)"

  # Add text
  lib_utils::print_normal "Writing to '${global_arg_file}' as type '${global_file_type}'"

  # Execute
  gawk \
    -v global_file_type="$global_file_type" \
    -v text="$(echo -e $_text | sed "${_sed_args[@]}")" \
    "${_awk_args[@]}" "$global_arg_file" >"$_tmp_file" \
    && mv -f "$_tmp_file" "$global_arg_file"

  # Cleanup
  [ -d "$_tmp_dir" ] && rmdir "$_tmp_dir"

  if [ $? -ne 0 ]; then
    lib_utils::die_fatal "Did not exist successfully"
  fi
}

function lib_license::license()
{
  lib_license::__parse_args "$@"
  lib_license::__license && lib_utils::print_info "License written successfully!"
}

# vim: sw=2 sts=2 si ai et
