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
# Implementation
#

[ -z "$DOCKER_FINANCE_CONTAINER_REPO" ] && exit 1
source "${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/lib_finance.bash" || exit 1

#
# Execute
#

function main()
{
  [ -z "$global_basename" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_1" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_2" ] && lib_utils::die_fatal

  local _usage="
\e[32mDescription:\e[0m

  The 'finance' in 'docker-finance'

\e[32mUsage:\e[0m

  $ $global_basename <profile>${global_arg_delim_1}<subprofile> <command> [args]

\e[32mProfile:\e[0m

  <profile>     \e[34;3mParent (category) profile (e.g., personal or business)\e[0m
  <subprofile>  \e[34;3mChild (username) profile (e.g., alice or evergreencrypto)\e[0m

\e[32mCommand:\e[0m

  all           \e[34;3mFetches, imports, generates taxes and reports w/ [args] year\e[0m
  edit          \e[34;3mEdit container configurations, metadata and journals\e[0m
  fetch         \e[34;3mFetch prices, remote accounts and blockchain data\e[0m
  import        \e[34;3mImport CSV data since given year (default: current year)\e[0m
  hledger       \e[34;3mRun hledger commands\e[0m
  hledger-ui    \e[34;3mStart hledger terminal UI\e[0m
  hledger-vui   \e[34;3mStart visidata terminal UI\e[0m
  hledger-web   \e[34;3mStart hledger web-based UI\e[0m
  meta          \e[34;3mSearch and view financial metadata\e[0m
  plugins       \e[34;3mExecute a categorical shell plugin (non-\`root\`)\e[0m
  reports       \e[34;3mGenerate balance sheet, income statement, etc.\e[0m
  root          \e[34;3mRun ROOT.cern instance for docker-finance analysis\e[0m
  taxes         \e[34;3mGenerate tax reports\e[0m
  times         \e[34;3mAccount for time\e[0m

\e[32mOptional:\e[0m

  [args]        \e[34;3mOptional arguments to command (when applicable)\e[0m

\e[32mExamples:\e[0m

  \e[37;2m# See help usage for the 'edit' command (e.g., to edit 'fetch' configuration)\e[0m
  $ $global_basename household${global_arg_delim_1}spouse edit help

  \e[37;2m# Fetch all of Alice's prices (as seen in her 'fetch' configuration)\e[0m
  $ $global_basename family${global_arg_delim_1}alice fetch all${global_arg_delim_2}price

  \e[37;2m# Import all of Bob's accounts for the years 2022 to now\e[0m
  $ $global_basename family${global_arg_delim_1}bob import year${global_arg_delim_2}2022

  \e[37;2m# Show Carol's total USD value of Bitcoin across all asset accounts\e[0m
  $ $global_basename friends${global_arg_delim_1}carol hledger bal assets cur:BTC --value=now

  \e[37;2m# Generate quarterly reports for your client 'ABC Inc.' in the year 2023\e[0m
  $ $global_basename clients${global_arg_delim_1}abc_inc reports all${global_arg_delim_2}type interval=quarterly year=2023
"

  #
  # "Constructor" (initializes finance environment)
  #

  lib_finance::finance "$@" || lib_utils::die_usage "$_usage"

  # shellcheck disable=SC2154
  [ "$DOCKER_FINANCE_DEBUG" == 2 ] && set -xv

  #
  # Facade commands
  #

  case "$2" in
    all)
      lib_finance::all "${@:3}"
      ;;
    edit)
      lib_finance::edit "${@:3}"
      ;;
    fetch)
      lib_finance::fetch "${@:3}"
      ;;
    import)
      lib_finance::import "${@:3}"
      ;;
    hledger)
      lib_finance::hledger "${@:3}"
      ;;
    hledger-ui)
      lib_finance::hledger-ui "${@:3}"
      ;;
    hledger-vui)
      lib_finance::hledger-vui "${@:3}"
      ;;
    hledger-web)
      lib_finance::hledger-web "${@:3}"
      ;;
    meta)
      lib_finance::meta "${@:3}"
      ;;
    plugins)
      lib_finance::plugins "${@:3}"
      ;;
    reports)
      lib_finance::reports "${@:3}"
      ;;
    root)
      lib_finance::root "${@:3}"
      ;;
    taxes)
      lib_finance::taxes "${@:3}"
      ;;
    time | times)
      lib_finance::times "${@:3}"
      ;;
    *)
      lib_utils::die_usage "$_usage"
      ;;
  esac
}

main "$@"

# vim: sw=2 sts=2 si ai et
