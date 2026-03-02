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
source "${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/lib_finance.bash"

[[ -z "$global_parent_profile" || -z "$global_arg_delim_1" || -z "$global_child_profile" ]] && lib_utils::die_fatal
instance="${global_parent_profile}${global_arg_delim_1}${global_child_profile}"

# Initialize "constructor"
lib_finance::finance "$instance" || lib_utils::die_fatal

#
# Implementation
#

lib_utils::deps_check "jq" # *should* be provided by dependency `yq`

function __format_time()
{
  [[ -z "$1" || -z "$2" ]] && lib_utils::die_fatal

  local -r _direction="$1"
  local -r _timestamp="$2"

  # Expects timewarrior *exported* time format
  echo -n "$_direction $(echo ${_timestamp:0:4}-${_timestamp:4:2}-${_timestamp:6:2} ${_timestamp:9:2}:${_timestamp:11:2}:${_timestamp:13:2})"
}

# - Converts exported timewarrior JSON schema into hledger timeclock format
# - Expects the following timewarrior style tags in the format of `key=value`
#
#     desc="My Description"
#     comment="My Comment"
#     tag:type=value
#     tag:...
#
# TODO: optimize: export only needed once if the majority of parsing is done within gawk
function main()
{
  # Stop tracking or else format breaks
  lib_finance::times stop &>/dev/null

  # Commands and timeclock entries
  local -r _cmd=("lib_finance::times" "export")
  local -r _count=$("${_cmd[@]}" | jq '. | length')
  local _id _start _end _tags

  # Cycle through total count of timewarrior entries
  for ((_i = 1; _i <= _count; _i++)); do

    # Parse timewarrior format
    local _array=()
    while read _line; do
      _array+=("$_line")
    done < <("${_cmd[@]}" | jq -Mer ".[${_i}-1] | .id,.start,.end")

    local _id="${_array[0]}"
    local _start="${_array[1]}"
    local _end="${_array[2]}"

    local _tags
    _tags="$("${_cmd[@]}" | jq -Mer ".[${_i}-1] | .tags.[]")"

    # Print parsed as timeclock format
    __format_time "i" "$_start"
    echo "$_tags" \
      | grep -E "(^profile=|^desc=|^comment=|^tag:)" \
      | gawk -F'=' -v _id="$_id" '{
          switch ($1)
            {
              case "profile":
                account=$2
                break
              case "desc":
                description=$2
                break
              case "comment":
                comment=$2
                break
              default:
                # All others are considered "real" tags
                tag=substr($0, 5)

                # Reconstruct to hledger tag format
                sub(/=/, ":", tag)
                tags[tag]=tag ", "

                break
            }
          }
          END {
            printf " " account "  " description " ; " comment ", id:" _id ", "
            for (t in tags)
              {
                i++
                printf t ", "
              }
            printf "\n"
          }'
    __format_time "o" "$_end"
    echo
  done
}

main "$@"

# vim: sw=2 sts=2 si ai et
