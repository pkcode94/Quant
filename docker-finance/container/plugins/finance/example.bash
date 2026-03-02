#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2024 Aaron Fiore (Founder, Evergreen Crypto LLC)
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
# Plugins
#
# The plugins path (this directory) is where you can place any executable.
# Any plugin found in this directory will be made available via the `plugins` command.
#
# Your plugin will have access to globals and functions provided by:
#
#   - finance.bash
#   - lib_finance.bash
#
# The following is an example of how to create your own plugin:
#

#
# "Libraries"
#

[ -z "$DOCKER_FINANCE_CONTAINER_REPO" ] && exit 1
source "${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/lib_finance.bash"

[[ -z "$global_parent_profile" || -z "$global_arg_delim_1" || -z "$global_child_profile" ]] && lib_utils::die_fatal
instance="${global_parent_profile}${global_arg_delim_1}${global_child_profile}"

# Initialize "constructor"
# NOTE: "constructor" only needed if calling library directly
lib_finance::finance "$instance" || lib_utils::die_fatal

#
# Implementation
#

function main()
{
  local -r _example="
This container's environment:

$(printenv | grep ^DOCKER_FINANCE | sort)

This plugin's caller profile:

    $instance

This plugin's path is:

    $0

This plugin's arguments:

    '${*}'

Showing total current BTC balance:

  $(lib_finance::hledger bal assets liabilities cur:BTC | tail -n1)

"
  lib_utils::print_custom "$_example"
}

main "$@"

# vim: sw=2 sts=2 si ai et
