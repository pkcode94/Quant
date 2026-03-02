#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2021-2024,2026 Aaron Fiore (Founder, Evergreen Crypto LLC)
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

# Runtime environment handler
source "${DOCKER_FINANCE_CLIENT_REPO}/client/src/docker/lib/internal/lib_env.bash" || exit 1

# Utilities (a container library (but container is never exposed to client code))
source "${DOCKER_FINANCE_CLIENT_REPO}/container/src/finance/lib/internal/lib_utils.bash" || exit 1

#
# Implementation
#

function lib_gen::__parse_args()
{
  [ -z "$global_usage" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_1" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_2" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_3" ] && lib_utils::die_fatal

  # All available hledger-flow accounts
  local _accounts
  mapfile -t _accounts < <(find ${DOCKER_FINANCE_CLIENT_REPO}/container/src/hledger-flow/accounts -name template | sort | rev | cut -d/ -f2 | rev)
  declare -r _accounts

  local -r _usage="
\e[32mDescription:\e[0m

  Generate environment and configurations

\e[32mUsage:\e[0m

  $ $global_usage <<[all${global_arg_delim_2}<type1[{${global_arg_delim_2}type2${global_arg_delim_3}...}]>] [type${global_arg_delim_3}<type1[${global_arg_delim_3}type2...]>]> [<[profile${global_arg_delim_2}<profile>]> [config${global_arg_delim_2}<config1[${global_arg_delim_3}config2...]>] [account${global_arg_delim_2}<account1[${global_arg_delim_3}account2...]>]] [confirm${global_arg_delim_2}<{on|yes|true} | {off|no|false}>] [dev${global_arg_delim_2}<{on|yes|true} | {off|no|false}>]

\e[32mArguments:\e[0m

  All categories and data:

    all${global_arg_delim_2}<all | type>

    Note: 'type' is currently the same as 'all'

  Category type:

    type${global_arg_delim_2}<env | build | flow | superscript>

  Flow (only):

    Full profile (w/ subprofile):

      profile${global_arg_delim_2}<parent${global_arg_delim_1}child>

    Configuration type:

      config${global_arg_delim_2}<fetch | hledger | meta | subscript>

    Accounts:

      account${global_arg_delim_2}<$(echo "${_accounts[@]}" | sed 's: : | :g')>

    Enable developer profile w/ mockups:

      dev${global_arg_delim_2}<{on|yes|true} | {off|no|false}>  # (default disabled)

  Enable confirmations:

    confirm${global_arg_delim_2}<{on|yes|true} | {off|no|false}>  # (default enabled)


\e[32mExamples:\e[0m

  \e[37;2m#\e[0m
  \e[37;2m# All (client, container)\e[0m
  \e[37;2m#\e[0m

  \e[37;2m# Generate all client and container data\e[0m
  $ $global_usage all${global_arg_delim_2}all

  \e[37;2m# Generate all client and container data, skipping confirmations (using a default profile/subprofile name)\e[0m
  $ $global_usage all${global_arg_delim_2}all confirm${global_arg_delim_2}no

  \e[37;2m# Generate all client and container data for profile called 'parent/child', skipping confirmations\e[0m
  $ $global_usage all${global_arg_delim_2}all profile${global_arg_delim_2}parent${global_arg_delim_1}child confirm${global_arg_delim_2}off

  \e[37;2m#\e[0m
  \e[37;2m# Client (joint container)\e[0m
  \e[37;2m#\e[0m

  \e[37;2m# Generate only the Docker environment\e[0m
  $ $global_usage type${global_arg_delim_2}env

  \e[37;2m# Generate custom Dockerfile and joint client/container superscript\e[0m
  $ $global_usage type${global_arg_delim_2}build${global_arg_delim_3}superscript

  \e[37;2m# Generate all client related data\e[0m
  $ $global_usage type${global_arg_delim_2}env${global_arg_delim_3}build${global_arg_delim_3}superscript

  \e[37;2m#\e[0m
  \e[37;2m# Flow: Profile -> Configs / Accounts\e[0m
  \e[37;2m#\e[0m

  \e[37;2m# Generate all client related and all flow related for profile/subprofile called 'parent/child'\e[0m
  $ $global_usage type${global_arg_delim_2}env${global_arg_delim_3}build${global_arg_delim_3}superscript${global_arg_delim_3}flow profile${global_arg_delim_2}parent${global_arg_delim_1}child

  \e[37;2m# Same command as above but without confirmations and with developer mockups\e[0m
  $ $global_usage type${global_arg_delim_2}env${global_arg_delim_3}build${global_arg_delim_3}superscript${global_arg_delim_3}flow profile${global_arg_delim_2}parent${global_arg_delim_1}child confirm${global_arg_delim_2}false dev${global_arg_delim_2}true

  \e[37;2m# Generate only the given configurations for 'parent/child'\e[0m
  $ $global_usage type${global_arg_delim_2}flow profile${global_arg_delim_2}parent${global_arg_delim_1}child config${global_arg_delim_2}fetch${global_arg_delim_3}hledger${global_arg_delim_3}meta${global_arg_delim_3}subscript

  \e[37;2m# Generate only the given accounts for 'parent/child'\e[0m
  $ $global_usage type${global_arg_delim_2}flow profile${global_arg_delim_2}parent${global_arg_delim_1}child account${global_arg_delim_2}capital-one${global_arg_delim_3}chase${global_arg_delim_3}coinbase

  \e[37;2m# Generate the given configs and accounts for given 'parent/child'\e[0m
  $ $global_usage type${global_arg_delim_2}flow profile${global_arg_delim_2}parent${global_arg_delim_1}child config${global_arg_delim_2}meta${global_arg_delim_3}subscript account${global_arg_delim_2}ethereum-based${global_arg_delim_3}metamask

\e[32mNotes:\e[0m

  - The 'profile' argument is limited to 'type' flow
  - The 'config' and 'account' arguments require 'profile'
"

  #
  # Ensure supported arguments
  #

  [ $# -eq 0 ] && lib_utils::die_usage "$_usage"

  for _arg in "$@"; do
    [[ ! "$_arg" =~ ^all${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^type${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^profile${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^config[s]?${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^account[s]?${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^confirm${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^dev${global_arg_delim_2} ]] \
      && lib_utils::die_usage "$_usage"
  done

  #
  # Parse arguments before testing
  #

  # Parse key for value
  for _arg in "$@"; do

    local _key="${_arg%${global_arg_delim_2}*}"
    local _len="$((${#_key} + 1))"

    if [[ "$_key" =~ ^all$ ]]; then
      local _arg_all="${_arg:${_len}}"
      [ -z "$_arg_all" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^type$ ]]; then
      local _arg_type="${_arg:${_len}}"
      [ -z "$_arg_type" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^profile$ ]]; then
      local _arg_profile="${_arg:${_len}}"
      [ -z "$_arg_profile" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^config[s]?$ ]]; then
      local _arg_config="${_arg:${_len}}"
      [ -z "$_arg_config" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^account[s]?$ ]]; then
      local _arg_account="${_arg:${_len}}"
      [ -z "$_arg_account" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^confirm$ ]]; then
      local _arg_confirm="${_arg:${_len}}"
      [ -z "$_arg_confirm" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^dev$ ]]; then
      local _arg_dev="${_arg:${_len}}"
      [ -z "$_arg_dev" ] && lib_utils::die_usage "$_usage"
    fi

  done

  #
  # Test for valid ordering/functionality of argument values
  #

  [[ -z "$_arg_all" && -z "$_arg_type" ]] && lib_utils::die_usage "$_usage"

  # Arg: all
  if [ ! -z "$_arg_all" ]; then
    # If all= then no need for type= or config= or account=
    if [[ ! -z "$_arg_type" || ! -z "$_arg_config" || ! -z "$_arg_account" ]]; then
      lib_utils::die_usage "$_usage"
    fi
  fi

  # Arg: type
  # Note: check not needed

  # Arg: profile
  if [ ! -z "$_arg_profile" ]; then
    # Requires type to be set
    if [[ ! "${_arg_all[*]}" =~ all|type && -z "$_arg_type" ]]; then
      lib_utils::die_usage "$_usage"
    fi
  fi

  # Arg: config
  if [ ! -z "$_arg_config" ]; then
    # Requires profile to be set
    if [ -z "$_arg_profile" ]; then
      lib_utils::die_usage "$_usage"
    fi
  fi

  # Arg: account
  if [ ! -z "$_arg_account" ]; then
    # Requires profile to be set
    if [ -z "$_arg_profile" ]; then
      lib_utils::die_usage "$_usage"
    fi
  fi

  # Arg: confirm
  # Note: optional argument, check not needed

  # Arg: dev
  # Note: optional argument, check not needed

  #
  # Test argument values, set globals
  #

  IFS="$global_arg_delim_3"

  # Arg: all
  if [ ! -z "$_arg_all" ]; then
    read -ra _read <<<"$_arg_all"
    for _arg in "${_read[@]}"; do
      # Supported values
      [[ ! "$_arg" =~ ^all$|^type[s]?$ ]] \
        && lib_utils::die_usage "$_usage"

      # If all=all then no need for all={type}
      [[ "$_arg" == "all" && "${#_read[@]}" -gt 1 ]] \
        && lib_utils::die_usage "$_usage"
    done
    # TODO: currently, 'all' will be equivalent 'type'
    declare -gr global_arg_all=("${_read[@]}")
  fi

  # Arg: type
  if [ ! -z "$_arg_type" ]; then
    read -ra _read <<<"$_arg_type"
    for _arg in "${_read[@]}"; do
      if [[ ! "$_arg" =~ ^env$|^build$|^flow$|^superscript$ ]]; then
        lib_utils::die_usage "$_usage"
      fi
    done
    declare -gr global_arg_type=("${_read[@]}")
  fi

  # Arg: profile
  if [ ! -z "$_arg_profile" ]; then
    # Requires flow type
    if [[ ! "${global_arg_all[*]}" =~ all|type && ! "${global_arg_type[*]}" =~ flow ]]; then
      lib_utils::die_usage "$_usage"
    fi
    if [[ ! "$_arg_profile" =~ $global_arg_delim_1 ]]; then
      lib_utils::die_usage "$_usage"
    fi
    local -r _parent="${_arg_profile%${global_arg_delim_1}*}"
    local -r _child="${_arg_profile##*${global_arg_delim_1}}"
    [[ -z "$_parent" || -z "$_child" ]] && lib_utils::die_usage "$_usage"
    declare -gr global_arg_parent_profile="$_parent"
    declare -gr global_arg_child_profile="$_child"
  fi

  # Arg: config
  if [ ! -z "$_arg_config" ]; then
    # Requires profile type
    if [ -z "$global_arg_parent_profile" ]; then
      lib_utils::die_usage "$_usage"
    fi
    read -ra _read <<<"$_arg_config"
    for _arg in "${_read[@]}"; do
      if [[ ! "$_arg" =~ ^fetch$|^hledger$|^meta$|^subscript$ ]]; then
        lib_utils::die_usage "$_usage"
      fi
    done
    declare -gr global_arg_config=("${_read[@]}")
  fi

  # Arg: account
  if [ ! -z "$_arg_account" ]; then
    # Requires profile type
    if [ -z "$global_arg_parent_profile" ]; then
      lib_utils::die_usage "$_usage"
    fi
    read -ra _read <<<"$_arg_account"
    # Test if given account is supported
    for _arg in "${_read[@]}"; do
      _found=false
      for _account in "${_accounts[@]}"; do
        [[ "$_arg" == "$_account" ]] && _found=true
      done
      [ $_found == true ] && continue || lib_utils::die_usage "$_usage"
    done
    declare -gr global_arg_account=("${_read[@]}")
  fi

  # Arg: confirm
  if [ ! -z "$_arg_confirm" ]; then
    if [[ ! "$_arg_confirm" =~ (^on$|^yes$|^true$)|(^off$|^no$|^false$) ]]; then
      lib_utils::die_usage "$_usage"
    fi
    if [[ "$_arg_confirm" =~ ^off$|^no$|^false$ ]]; then
      declare -gr global_arg_confirm=""
    else
      declare -gr global_arg_confirm="true"
    fi
  else
    declare -gr global_arg_confirm="true"
  fi

  # Arg: dev
  if [ ! -z "$_arg_dev" ]; then
    if [[ ! "$_arg_dev" =~ (^on$|^yes$|^true$)|(^off$|^no$|^false$) ]]; then
      lib_utils::die_usage "$_usage"
    fi
    if [[ "$_arg_dev" =~ ^on$|^yes$|^true$ ]]; then
      declare -gr global_arg_dev="true"
    else
      declare -gr global_arg_dev=""
    fi
  else
    declare -gr global_arg_dev=""
  fi
}

#
# Generate new configurations or layout
#

function lib_gen::gen()
{
  lib_gen::__parse_args "$@"

  lib_utils::print_custom "\n"
  lib_utils::print_info "Generating client/container environment"

  lib_gen::__gen_client

  if [ $? -eq 0 ]; then
    [ -z "$global_platform" ] && lib_utils::die_fatal
    if [ "$global_platform" != "dev-tools" ]; then
      lib_gen::__gen_container
    fi
  fi

  if [ $? -eq 0 ]; then
    lib_utils::print_custom "\n"
    lib_utils::print_info "Congratulations, environment sucessfully generated!"
    lib_utils::print_custom "\n"
  else
    lib_utils::print_custom "\n"
    lib_utils::die_fatal "Environment not fully generated! Try again"
    lib_utils::print_custom "\n"
  fi
}

function lib_gen::__gen_client()
{
  [ -z "$global_suffix" ] && lib_utils::die_fatal

  lib_utils::print_debug "Generating client"

  #
  # Client-side environment file
  #

  if [[ -z "${global_arg_type[*]}" || "${global_arg_type[*]}" =~ env ]]; then

    [ -z "$global_env_file" ] && lib_utils::die_fatal
    [ -z "$global_repo_env_file" ] && lib_utils::die_fatal

    # Backup existing file
    if [ -f "$global_env_file" ]; then
      lib_utils::print_custom "    \e[32m│\e[0m\n"
      lib_utils::print_custom "    \e[32m├─\e[34;1m Client environment file found, backup then generate new one? [Y/n] \e[0m"

      [ -z "$global_arg_confirm" ] && lib_utils::print_custom "\n" || read -p "" _read
      local _confirm="${_read:-y}"

      if [[ "$_confirm" == [yY] || -z "$global_arg_confirm" ]]; then
        cp -a "$global_env_file" "${global_env_file}_${global_suffix}" \
          || lib_utils::die_fatal

        # Get/Set with repository defaults
        lib_utils::print_debug "Reading $global_repo_env_file"
        lib_env::__read "$global_repo_env_file"

        lib_utils::print_debug "Writing $global_env_file"
        lib_env::__write "$global_env_file"
      fi
    fi

    lib_utils::print_custom "    \e[32m│  └─\e[34m Edit file now? [Y/n] \e[0m"
    lib_gen::__gen_edit "$global_env_file"

    # Get/Set new (edited) environment variables
    lib_env::__read "$global_env_file"
    lib_env::__set_client_globals

  fi

  #
  # Custom (optional) Dockerfile
  #

  if [[ -z "${global_arg_type[*]}" || "${global_arg_type[*]}" =~ build ]]; then

    [ -z "$global_custom_dockerfile" ] && lib_utils::die_fatal
    [ -z "$global_repo_custom_dockerfile" ] && lib_utils::die_fatal

    # Backup existing custom Dockerfile
    if [ -f "$global_custom_dockerfile" ]; then
      lib_utils::print_custom "    \e[32m│\e[0m\n"
      lib_utils::print_custom "    \e[32m├─\e[34;1m Custom (optional) Dockerfile found, backup then generate new one? [Y/n] \e[0m"

      [ -z "$global_arg_confirm" ] && lib_utils::print_custom "\n" || read -p "" _read
      local _confirm="${_read:-y}"
      if [[ "$_confirm" == [yY] || -z "$global_arg_confirm" ]]; then
        cp -a "$global_custom_dockerfile" "${global_custom_dockerfile}_${global_suffix}" || lib_utils::die_fatal
      fi
    else
      lib_utils::print_custom "    \e[32m│\e[0m\n"
      lib_utils::print_custom "    \e[32m├─\e[34;1m Generating new custom (optional) Dockerfile\e[0m\n"

      lib_utils::print_debug "$global_repo_custom_dockerfile"
      lib_utils::print_debug "$global_custom_dockerfile"
    fi

    # Filter to de-clutter output file (license cleanup)
    local -r _filter="1,17d"

    sed \
      -e "$_filter" \
      -e "s:@DOCKER_FINANCE_VERSION@:${global_client_version}:g" \
      "$global_repo_custom_dockerfile" >"$global_custom_dockerfile" || lib_utils::die_fatal

    lib_utils::print_custom "    \e[32m│  └─\e[34m Edit file now? [Y/n] \e[0m"
    lib_gen::__gen_edit "$global_custom_dockerfile"
  fi

  #
  # Generate client-side `plugins` layout (custom)
  #

  lib_gen::__gen_plugins

  #
  # Generate shared path layout
  #

  # TODO: consider supporting for dev-tools
  [ -z "$global_platform" ] && lib_utils::die_fatal
  if [ "$global_platform" != "dev-tools" ]; then
    lib_gen::__gen_shared
  fi
}

# Custom plugins layout to drop-in custom plugins
# NOTE: underlying impl expects this layout
function lib_gen::__gen_plugins()
{
  lib_utils::print_debug "Generating custom plugins layout"

  [ -z "$DOCKER_FINANCE_CLIENT_PLUGINS" ] && lib_utils::die_fatal
  if [ ! -d "$DOCKER_FINANCE_CLIENT_PLUGINS" ]; then
    mkdir -p "$DOCKER_FINANCE_CLIENT_PLUGINS" || lib_utils::die_fatal
  fi

  local -r _client="${DOCKER_FINANCE_CLIENT_PLUGINS}/client"
  if [ ! -d "${_client}/docker" ]; then
    mkdir -p "${_client}/docker" || lib_utils::die_fatal
  fi

  local -r _container="${DOCKER_FINANCE_CLIENT_PLUGINS}/container"
  if [ ! -d "${_container}/finance" ]; then
    mkdir -p "${_container}/finance" || lib_utils::die_fatal
  fi
  if [ ! -d "${_container}/root" ]; then
    mkdir -p "${_container}/root" || lib_utils::die_fatal
  fi
}

# Shared path layout to drop-in shared files
function lib_gen::__gen_shared()
{
  lib_utils::print_debug "Generating shared path"
  [ -z "$DOCKER_FINANCE_CLIENT_SHARED" ] && lib_utils::die_fatal
  if [ ! -d "$DOCKER_FINANCE_CLIENT_SHARED" ]; then
    mkdir -p "$DOCKER_FINANCE_CLIENT_SHARED" || lib_utils::die_fatal
  fi
}

function lib_gen::__gen_container()
{
  lib_utils::print_debug "Generating container"

  #
  # Generate joint client/container superscript
  #

  if [[ -z "${global_arg_type[*]}" || "${global_arg_type[*]}" =~ superscript ]]; then

    lib_utils::print_custom "    \e[32m│\e[0m\n"
    lib_utils::print_custom "    \e[32m├─\e[34;1m Generate joint client/container superscript? [Y/n] \e[0m"
    [ -z "$global_arg_confirm" ] && lib_utils::print_custom "\n" || read -p "" _read
    _confirm="${_read:-y}"
    if [[ "$_confirm" == [yY] || -z "$global_arg_confirm" ]]; then
      lib_gen::__gen_superscript
    fi

  fi

  #
  # Generate flow
  #

  if [[ "${global_arg_all[*]}" =~ all|type || "${global_arg_type[*]}" =~ flow ]]; then

    lib_utils::print_custom "    \e[32m│\e[0m\n"
    lib_utils::print_custom "    \e[32m├─\e[34;1m Generate container finance flow (layout and profiles)? [Y/n] \e[0m"
    # Prompt if type not given
    if [[ ! "${global_arg_all[*]}" =~ all|type || -z "${global_arg_type[*]}" ]]; then
      if [ -z "$global_arg_confirm" ]; then
        lib_utils::print_custom "\n"
      else
        read -p "" _read
        local _confirm="${_read:-y}"
        if [[ "$_confirm" != [yY] ]]; then
          lib_utils::print_custom "    \e[32m│\e[0m\n"
          return 0
        fi
      fi
    fi

    [ -z "$DOCKER_FINANCE_CLIENT_FLOW" ] && lib_utils::die_fatal
    if [ ! -d "$DOCKER_FINANCE_CLIENT_FLOW" ]; then
      mkdir -p "$DOCKER_FINANCE_CLIENT_FLOW" || lib_utils::die_fatal
    fi

    lib_gen::__gen_times
    lib_gen::__gen_profile

  fi
}

#
# Generate joint client/container superscript
#

function lib_gen::__gen_superscript()
{
  [ -z "$global_superscript" ] && lib_utils::die_fatal

  if [ -f "$global_superscript" ]; then
    lib_utils::print_custom "    \e[32m│  └─\e[34m Backup existing file and generate a new one? [N/y] \e[0m"

    [ -z "$global_arg_confirm" ] && lib_utils::print_custom "\n" || read -p "" _read
    local _confirm="${_read:-n}"

    if [[ "$_confirm" == [yY] || -z "$global_arg_confirm" ]]; then
      # Backup
      local -r _backup=("cp" "-a" "$global_superscript" "${global_superscript}_${global_suffix}")
      lib_utils::print_debug "${_backup[@]}"
      "${_backup[@]}" || lib_utils::die_fatal
      # Write
      lib_gen::__gen_superscript_write
    fi
    local _print_custom="    \e[32m│     └─\e[34m Edit file now? [Y/n] \e[0m"
  else
    # Generate new default file
    lib_gen::__gen_superscript_write
    local _print_custom="    \e[32m│  │  └─\e[34m Edit file now? [Y/n] \e[0m"
  fi

  lib_utils::print_custom "$_print_custom"
  lib_gen::__gen_edit "$global_superscript"
}

function lib_gen::__gen_superscript_write()
{
  [ -z "$global_repo_conf_dir" ] && lib_utils::die_fatal
  lib_utils::print_debug "global_repo_conf_dir=${global_repo_conf_dir}"

  [ -z "$global_client_version" ] && lib_utils::die_fatal
  lib_utils::print_debug "global_client_version=${global_client_version}"

  [ -z "$global_superscript" ] && lib_utils::die_fatal
  lib_utils::print_debug "global_superscript=${global_superscript}"

  # Filter to de-clutter output file (license cleanup)
  local -r _filter="3,19d"

  sed \
    -e "$_filter" \
    -e "s:@DOCKER_FINANCE_VERSION@:${global_client_version}:g" \
    "${global_repo_conf_dir}/container/shell/superscript.bash.in" >"$global_superscript"
}

#
# Generate flow: times
#
#  - Provides a place for:
#    * timew database
#    * Anything time-related (among all profiles)
#

function lib_gen::__gen_times()
{
  lib_utils::print_debug "Generating times"

  local -r _times="${DOCKER_FINANCE_CLIENT_FLOW}/times"
  if [ ! -d "$_times" ]; then
    mkdir -p "$_times" || lib_utils::die_fatal
  fi

  # NOTE: timew database will be created upon first call
}

#
# Generate flow: profile/subprofile
#

function lib_gen::__gen_profile()
{
  lib_utils::print_debug "Generating profiles"

  local -r _profiles="${DOCKER_FINANCE_CLIENT_FLOW}/profiles"
  if [ ! -d "$_profiles" ]; then
    mkdir -p "$_profiles" || lib_utils::die_fatal
  fi

  #
  # Profile: construct full path to subprofile
  #

  [ -z "$global_user" ] && lib_utils::die_fatal

  # Profile
  lib_utils::print_custom "    \e[32m│  │  │\e[0m\n"
  lib_utils::print_custom "    \e[32m│  │  ├─\e[34m Enter profile name (e.g., family in 'family/alice'): \e[0m"
  if [ ! -z "$global_arg_parent_profile" ]; then
    lib_utils::print_custom "\n"
    _read="$global_arg_parent_profile"
  else
    if [ -z "$global_arg_confirm" ]; then
      lib_utils::print_custom "\n"
      _read=""
    else
      read -p "" _read
    fi
  fi
  local -r _profile="${_read:-default}"
  lib_utils::print_custom "    \e[32m│  │  │  └─\e[34m Using profile:\e[0m ${_profile}\n"

  # Subprofile
  lib_utils::print_custom "    \e[32m│  │  │\e[0m\n"
  lib_utils::print_custom "    \e[32m│  │  ├─\e[34m Enter subprofile name (e.g., alice in 'family/alice'): \e[0m"
  if [ ! -z "$global_arg_child_profile" ]; then
    lib_utils::print_custom "\n"
    _read="$global_arg_child_profile"
  else
    if [ -z "$global_arg_confirm" ]; then
      lib_utils::print_custom "\n"
      _read=""
    else
      read -p "" _read
    fi
  fi
  local -r _subprofile="${_read:-${global_user}}"
  lib_utils::print_custom "    \e[32m│  │  │  └─\e[34m Using subprofile:\e[0m ${_subprofile}\n"

  # Check if full profile exists
  if [ -d "${_profiles}/${_profile}/${_subprofile}" ]; then
    lib_utils::print_custom "    \e[32m│  │  │     └─\e[34m Subprofile exists! Continue with backup and generation? [Y/n] \e[0m"

    if [ -z "$global_arg_confirm" ]; then
      lib_utils::print_custom "\n"
      _read=""
    else
      read -p "" _read
    fi
    _confirm="${_read:-y}"
    if [[ "$_confirm" != [yY] ]]; then
      lib_utils::print_custom "    \e[32m│\e[0m\n"
      lib_utils::print_normal ""
      return 0
    fi
  fi

  #
  # Profile: generate profile-specific
  #

  local -r _gen_path="${_profiles}/${_profile}/${_subprofile}"
  lib_utils::print_debug "_gen_path=${_gen_path}"

  local -r _gen_conf_path="${_gen_path}/conf.d"
  lib_utils::print_debug "_gen_conf_path=${_gen_conf_path}"

  lib_gen::__gen_subprofile_flow
}

#
# Subprofile: generate flow
#

function lib_gen::__gen_subprofile_flow()
{
  [[ -z "$_gen_path" || -z "$_gen_conf_path" ]] && lib_utils::die_fatal

  if lib_gen::__gen_subprofile_flow_args_config "subscript"; then
    lib_utils::print_custom "    \e[32m│  │  │\e[0m\n"
    lib_utils::print_custom "    \e[32m│  │  ├─\e[34;1m Generate subprofile's subscript file? [Y/n] \e[0m"
    [ -z "$global_arg_confirm" ] && lib_utils::print_custom "\n" || read -p "" _read
    _confirm="${_read:-y}"
    if [[ "$_confirm" == [yY] || -z "$global_arg_confirm" ]]; then
      # Subprofile's shell script
      lib_gen::__gen_subprofile_flow_subscript
      # Append subprofile source to superscript
      lib_gen::__gen_subprofile_flow_superscript
    fi
  fi

  if lib_gen::__gen_subprofile_flow_args_config "fetch"; then
    # Prompt for default generation
    lib_utils::print_custom "    \e[32m│  │  │\e[0m\n"
    lib_utils::print_custom "    \e[32m│  │  ├─\e[34;1m Generate subprofile's fetch configuration file? [Y/n] \e[0m"
    [ -z "$global_arg_confirm" ] && lib_utils::print_custom "\n" || read -p "" _read
    _confirm="${_read:-y}"
    [[ "$_confirm" == [yY] || -z "$global_arg_confirm" ]] && lib_gen::__gen_subprofile_flow_fetch
  fi

  if lib_gen::__gen_subprofile_flow_args_config "meta"; then
    lib_utils::print_custom "    \e[32m│  │  │\e[0m\n"
    lib_utils::print_custom "    \e[32m│  │  ├─\e[34;1m Generate subprofile's financial metadata file? [Y/n] \e[0m"
    [ -z "$global_arg_confirm" ] && lib_utils::print_custom "\n" || read -p "" _read
    _confirm="${_read:-y}"
    [[ "$_confirm" == [yY] || -z "$global_arg_confirm" ]] && lib_gen::__gen_subprofile_flow_meta
  fi

  if lib_gen::__gen_subprofile_flow_args_config "hledger"; then
    lib_utils::print_custom "    \e[32m│  │  │\e[0m\n"
    lib_utils::print_custom "    \e[32m│  │  ├─\e[34;1m Generate subprofile's hledger configuration file? [Y/n] \e[0m"
    [ -z "$global_arg_confirm" ] && lib_utils::print_custom "\n" || read -p "" _read
    _confirm="${_read:-y}"
    [[ "$_confirm" == [yY] || -z "$global_arg_confirm" ]] && lib_gen::__gen_subprofile_flow_hledger
  fi

  if lib_gen::__gen_subprofile_flow_args_account; then
    lib_utils::print_custom "    \e[32m│  │  │\e[0m\n"
    lib_utils::print_custom "    \e[32m│  │  ├─\e[34;1m Generate subprofile's hledger-flow accounts? [Y/n] \e[0m"
    if [ ! -z "${global_arg_account[*]}" ]; then
      lib_utils::print_custom "\n"
      lib_gen::__gen_subprofile_flow_accounts
    else
      [ -z "$global_arg_confirm" ] && lib_utils::print_custom "\n" || read -p "" _read
      _confirm="${_read:-y}"
      [[ "$_confirm" == [yY] || -z "$global_arg_confirm" ]] && lib_gen::__gen_subprofile_flow_accounts
    fi
  fi

  lib_utils::print_custom "    \e[32m│\e[0m\n"

  # Placeholder connector between hledger-flow code and end-user's hledger-flow profiles
  # NOTE: entrypoint will (MUST) update this on every container run (yes, looks different than entrypoint)
  lib_utils::print_custom "    \e[32m├─\e[34;1m Connecting flow sources \e[0m\n"

  [ -z "$DOCKER_FINANCE_CONTAINER_REPO" ] && lib_utils::die_fatal
  ln -f -s "${DOCKER_FINANCE_CONTAINER_REPO}/src/hledger-flow" "${DOCKER_FINANCE_CLIENT_FLOW}/src"

  lib_utils::print_custom "    \e[32m│\e[0m\n"
}

function lib_gen::__gen_subprofile_flow_args_config()
{
  [ -z "$1" ] && lib_utils::die_fatal
  if [[ -z "${global_arg_type[*]}" || ("${global_arg_type[*]}" =~ flow && -z "$global_arg_parent_profile") || ("${global_arg_type[*]}" =~ flow && ! -z "$global_arg_parent_profile" && -z "${global_arg_account[*]}" && -z "${global_arg_config[*]}") || "${global_arg_config[*]}" =~ $1 ]]; then
    return 0
  fi
  return 1
}

function lib_gen::__gen_subprofile_flow_args_account()
{
  if [[ -z "${global_arg_type[*]}" || ("${global_arg_type[*]}" =~ flow && -z "$global_arg_parent_profile") || ("${global_arg_type[*]}" =~ flow && ! -z "$global_arg_parent_profile" && -z "${global_arg_account[*]}" && -z "${global_arg_config[*]}") || ("${global_arg_type[*]}" =~ flow && ! -z "$global_arg_parent_profile" && ! -z "${global_arg_account[*]}") ]]; then
    return 0
  fi
  return 1
}

#
# Subprofile: flow: generate subprofile subscript
#

function lib_gen::__gen_subprofile_flow_subscript()
{
  [[ -z "$_gen_path" || -z "$_gen_conf_path" ]] && lib_utils::die_fatal

  local _dir="${_gen_conf_path}/shell"
  [ ! -d "$_dir" ] && mkdir -p "$_dir"

  local _file="${_dir}/subscript.bash"
  if [ -f "$_file" ]; then
    lib_utils::print_custom "    \e[32m│  │  │  └─\e[34m File found, backup then generate new one? [Y/n] \e[0m"
    [ -z "$global_arg_confirm" ] && lib_utils::print_custom "\n" || read -p "" _read
    _confirm="${_read:-y}"

    if [[ "$_confirm" == [yY] || -z "$global_arg_confirm" ]]; then
      cp -a "$_file" "${_file}_${global_suffix}" || lib_utils::die_fatal

      lib_gen::__gen_subprofile_flow_subscript_write
      local _print_custom="    \e[32m│  │  │     └─\e[34m Edit file now? [Y/n] \e[0m"
    fi
  else
    lib_gen::__gen_subprofile_flow_subscript_write
    local _print_custom="    \e[32m│  │  │  └─\e[34m Edit file now? [Y/n] \e[0m"
  fi

  lib_utils::print_custom "$_print_custom"
  lib_gen::__gen_edit "$_file"
}

function lib_gen::__gen_subprofile_flow_subscript_write()
{
  [[ -z "$DOCKER_FINANCE_CONTAINER_REPO" || -z "$DOCKER_FINANCE_CONTAINER_CMD" ]] && lib_utils::die_fatal

  [[ -z "$_profile" || -z "$_subprofile" || -z "$_file" ]] && lib_utils::die_fatal

  [ -z "$global_client_version" ] && lib_utils::die_fatal
  [ -z "$global_repo_conf_dir" ] && lib_utils::die_fatal

  # Filter to de-clutter output file (license cleanup)
  local -r _filter="3,19d"

  sed \
    -e "$_filter" \
    -e "s:@DOCKER_FINANCE_CONTAINER_CMD@:${DOCKER_FINANCE_CONTAINER_CMD}:g" \
    -e "s:@DOCKER_FINANCE_CONTAINER_REPO@:${DOCKER_FINANCE_CONTAINER_REPO}:g" \
    -e "s:@DOCKER_FINANCE_VERSION@:${global_client_version}:g" \
    -e "s:@DOCKER_FINANCE_PROFILE@:${_profile}:g" \
    -e "s:@DOCKER_FINANCE_SUBPROFILE@:${_subprofile}:g" \
    "${global_repo_conf_dir}/container/shell/subscript.bash.in" >"$_file"
}

#
# Subprofile: flow: append subscript to superscript
#

function lib_gen::__gen_subprofile_flow_superscript()
{
  [[ -z "$_profile" || -z "$_subprofile" ]] && lib_utils::die_fatal

  [ -z "$global_superscript" ] && lib_utils::die_fatal
  [ ! -f "$global_superscript" ] && lib_utils::die_fatal "Superscript does not exist!"

  # Append subprofile source to superscript
  local -r _source="source \"\${DOCKER_FINANCE_CONTAINER_FLOW}/profiles/${_profile}/${_subprofile}/conf.d/shell/subscript.bash\""
  lib_utils::print_custom "    \e[32m│  │  │  └─\e[34m Appending subprofile to superscript\e[0m\n"

  # If source subprofile does not exist, append
  grep "$_source" "$global_superscript" >&/dev/null \
    || sed -i "$(wc -l <$global_superscript)i\\$_source\\" "$global_superscript"

  lib_utils::print_custom "    \e[32m│  │  │     └─\e[34m Edit superscript now? [Y/n] \e[0m"
  lib_gen::__gen_edit "$global_superscript"
}

#
# Subprofile: flow: generate fetch config
#

function lib_gen::__gen_subprofile_flow_fetch()
{
  [[ -z "$_gen_path" || -z "$_gen_conf_path" ]] && lib_utils::die_fatal

  local -r _dir="${_gen_conf_path}/fetch"
  [ ! -d "$_dir" ] && mkdir -p "$_dir"

  local -r _file="${_dir}/fetch.yaml"
  if [ -f "$_file" ]; then
    lib_utils::print_custom "    \e[32m│  │  │  └─\e[34m File found, backup then generate new one? [Y/n] \e[0m"
    [ -z "$global_arg_confirm" ] && lib_utils::print_custom "\n" || read -p "" _read
    _confirm="${_read:-y}"

    if [[ "$_confirm" == [yY] || -z "$global_arg_confirm" ]]; then
      cp -a "$_file" "${_file}_${global_suffix}" || lib_utils::die_fatal

      lib_gen::__gen_subprofile_flow_fetch_write
      local _print_custom="    \e[32m│  │  │     └─\e[34m Edit file now? [Y/n] \e[0m"
    fi
  else
    lib_gen::__gen_subprofile_flow_fetch_write
    local _print_custom="    \e[32m│  │  │  └─\e[34m Edit file now? [Y/n] \e[0m"
  fi

  lib_utils::print_custom "$_print_custom"
  lib_gen::__gen_edit "$_file"
}

function lib_gen::__gen_subprofile_flow_fetch_write()
{
  [[ -z "$_profile" || -z "$_subprofile" || -z "$_file" ]] && lib_utils::die_fatal

  [ -z "$global_client_version" ] && lib_utils::die_fatal
  [ -z "$global_repo_conf_dir" ] && lib_utils::die_fatal

  # Filter to de-clutter output file (license cleanup)
  local -r _filter="1,17d"

  sed \
    -e "$_filter" \
    -e "s:@DOCKER_FINANCE_VERSION@:${global_client_version}:g" \
    -e "s:@DOCKER_FINANCE_PROFILE@:${_profile}:g" \
    -e "s:@DOCKER_FINANCE_SUBPROFILE@:${_subprofile}:g" \
    "${global_repo_conf_dir}/container/fetch/fetch.yaml.in" >"$_file"
}

#
# Subprofile: flow: generate financial metadata
#

function lib_gen::__gen_subprofile_flow_meta()
{
  [[ -z "$_gen_path" || -z "$_gen_conf_path" ]] && lib_utils::die_fatal

  local _dir="${_gen_conf_path}/meta"
  [ ! -d "$_dir" ] && mkdir -p "$_dir"

  local _file="${_dir}/meta.csv"
  if [ -f "$_file" ]; then

    lib_utils::print_custom "    \e[32m│  │  │  └─\e[34m File found, backup then generate new one? [Y/n] \e[0m"
    [ -z "$global_arg_confirm" ] && lib_utils::print_custom "\n" || read -p "" _read
    _confirm="${_read:-y}"

    if [[ "$_confirm" == [yY] || -z "$global_arg_confirm" ]]; then
      cp -a "$_file" "${_file}_${global_suffix}" || lib_utils::die_fatal

      lib_gen::__gen_subprofile_flow_meta_write
      local _print_custom="    \e[32m│  │  │     └─\e[34m Edit file now? [Y/n] \e[0m"
    fi
  else
    lib_gen::__gen_subprofile_flow_meta_write
    local _print_custom="    \e[32m│  │  │  └─\e[34m Edit file now? [Y/n] \e[0m"
  fi

  lib_utils::print_custom "$_print_custom"
  lib_gen::__gen_edit "$_file"
}

function lib_gen::__gen_subprofile_flow_meta_write()
{
  [ -z "$_file" ] && lib_utils::die_fatal

  [ -z "$global_client_version" ] && lib_utils::die_fatal
  [ -z "$global_repo_conf_dir" ] && lib_utils::die_fatal

  # Deletes default comments or else ROOT meta sample won't work out-of-the-box
  # TODO: possible to keep filtered comments using upstream options?
  sed \
    -e "/\/\/\\!/d" \
    -e "s:@DOCKER_FINANCE_VERSION@:${global_client_version}:g" \
    "${global_repo_conf_dir}/container/meta/meta.csv.in" >"$_file"
}

#
# Subprofile: flow: generate hledger configuration file
#

function lib_gen::__gen_subprofile_flow_hledger()
{
  [[ -z "$_gen_path" || -z "$_gen_conf_path" ]] && lib_utils::die_fatal

  local _dir="${_gen_conf_path}/hledger"
  [ ! -d "$_dir" ] && mkdir -p "$_dir"

  local _file="${_dir}/hledger.conf"
  if [ -f "$_file" ]; then
    lib_utils::print_custom "    \e[32m│  │  │  └─\e[34m File found, backup then generate new one? [Y/n] \e[0m"
    [ -z "$global_arg_confirm" ] && lib_utils::print_custom "\n" || read -p "" _read
    _confirm="${_read:-y}"

    if [[ "$_confirm" == [yY] || -z "$global_arg_confirm" ]]; then
      cp -a "$_file" "${_file}_${global_suffix}" || lib_utils::die_fatal

      lib_gen::__gen_subprofile_flow_hledger_write
      local _print_custom="    \e[32m│  │  │     └─\e[34m Edit file now? [Y/n] \e[0m"
    fi
  else
    lib_gen::__gen_subprofile_flow_hledger_write
    local _print_custom="    \e[32m│  │  │  └─\e[34m Edit file now? [Y/n] \e[0m"
  fi

  lib_utils::print_custom "$_print_custom"
  lib_gen::__gen_edit "$_file"
}

function lib_gen::__gen_subprofile_flow_hledger_write()
{
  [ -z "$_file" ] && lib_utils::die_fatal

  [ -z "$global_client_version" ] && lib_utils::die_fatal
  lib_utils::print_debug "global_client_version=${global_client_version}"

  [ -z "$global_repo_conf_dir" ] && lib_utils::die_fatal
  lib_utils::print_debug "global_repo_conf_dir=${global_repo_conf_dir}"

  # Filter to de-clutter output file (license cleanup)
  local -r _filter="1,17d"

  sed \
    -e "$_filter" \
    -e "s:@DOCKER_FINANCE_VERSION@:${global_client_version}:g" \
    "${global_repo_conf_dir}/container/hledger/hledger.conf.in" >"$_file"
}

#
# Subprofile: flow: generate accounts
#

function lib_gen::__gen_subprofile_flow_accounts()
{
  [[ -z "$_gen_path" || -z "$_gen_conf_path" || -z "$_subprofile" ]] && lib_utils::die_fatal

  lib_utils::print_debug "Generating accounts"

  local _subprofile_path="${_gen_path}/import/${_subprofile}"
  lib_utils::print_debug "_subprofile_path=${_subprofile_path}"

  if [ -d "$_subprofile_path" ]; then
    lib_utils::print_custom "    \e[32m│  │  │  ├─\e[34m Subprofile import path exists! Continue with account generation (files will be backed up)? [Y/n] \e[0m"
    [ -z "$global_arg_confirm" ] && lib_utils::print_custom "\n" || read -p "" _read
    _confirm="${_read:-y}"
    [[ "$_confirm" == [yY] || -z "$global_arg_confirm" ]] || return 0
  else
    mkdir -p "$_subprofile_path"
  fi

  # All available hledger-flow account paths
  local _templates_paths
  mapfile -t _templates_paths < <(find "${DOCKER_FINANCE_CLIENT_REPO}"/container/src/hledger-flow/accounts -name template | sort)
  declare -r _templates_paths

  # All available hledger-flow account names
  local _accounts
  if [ -z "${global_arg_account[*]}" ]; then
    for _template in "${_templates_paths[@]}"; do
      _accounts+=("$(echo "$_template" | rev | cut -d/ -f2 | rev)")
    done
  else
    declare -r _accounts=("${global_arg_account[@]}")
  fi

  # Cycle through all available docker-finance templates and populate subprofile
  for _account in "${_accounts[@]}"; do
    for _template_path in "${_templates_paths[@]}"; do
      if [[ "$_template_path" =~ \/$_account\/ ]]; then
        lib_utils::print_debug "_template_path=${_template_path}"
        lib_utils::print_custom "    \e[32m│  │  │  │  │\e[0m\n"
        lib_utils::print_custom "    \e[32m│  │  │  │  ├─\e[34m\e[1m Generate $_account ? [Y/n] \e[0m"
        [ -z "$global_arg_confirm" ] && lib_utils::print_custom "\n" || read -p "" _read
        _confirm="${_read:-y}"
        [[ "$_confirm" == [yY] || -z "$global_arg_confirm" ]] \
          && lib_gen::__gen_subprofile_flow_accounts_populate "$_subprofile_path" "$_template_path"
      fi
    done
  done
}

function lib_gen::__gen_subprofile_flow_accounts_populate()
{
  [[ -z "$1" || -z "$2" ]] && lib_utils::die_fatal
  local _subprofile_path="$1"
  local _template_path="$2"

  # Continue if hledger-flow account exists
  local _continue
  _continue=true

  local _account_path
  _account_path="${_subprofile_path}/$(echo $_template_path | rev | cut -d/ -f2 | rev)"
  lib_utils::print_debug "_account_path=${_account_path}"

  # TODO: doesn't check for blockchain explorer shared rules/bash
  if [ -d "$_account_path" ]; then
    lib_utils::print_custom "    \e[32m│  │  │  │  │  └─\e[34m Account exists! Continue with backup and generation? [Y/n] \e[0m"
    [ -z "$global_arg_confirm" ] && lib_utils::print_custom "\n" || read -p "" _read
    _confirm="${_read:-y}"
    [[ "$_confirm" == [yY] || -z "$global_arg_confirm" ]] || _continue=false
  fi

  # Populate with template
  if [[ $_continue == true ]]; then

    # Populates account and blockchain explorer's shared rules/preprocess
    #   (at top-level, along with all the account's shared rules/preprocess)
    lib_utils::print_debug "Copying '${_template_path}/*' to '${_subprofile_path}/'"
    cp -a -R --backup --suffix="_${global_suffix}" \
      "${_template_path}"/* "$_subprofile_path/" || lib_utils::die_fatal

    # If not blockchain explorers (since those exist as subaccounts)
    if [[ ! "$_template_path" =~ "blockchain-explorers" ]]; then

      # Add year to 1-in
      while read _in_path; do
        local _year_path
        _year_path="${_in_path}/$(date +%Y)"
        lib_utils::print_debug "Making year path '${_year_path}'"
        mkdir -p "$_year_path"

        # Appropriate test data if testing
        local _mockup="${_in_path}/mockup"
        lib_utils::print_debug "Getting mockup '${_mockup}'"

        if [ ! -z "$global_arg_dev" ]; then
          lib_utils::print_debug "Copying mockup to '${_in_path}'"
          cp -a -R --backup --suffix="_${global_suffix}" "${_mockup}"/* "${_in_path}/" || lib_utils::die_fatal
        fi

        # Always remove original test data
        rm -fr "$_mockup"

      done < <(find "$_account_path" -name 1-in)
    fi

  fi
}

function lib_gen::__gen_edit()
{
  local _file="$1"
  [[ -z "$_file" || ! -f "$_file" ]] && lib_utils::die_fatal

  if [ -z "$global_arg_confirm" ]; then
    lib_utils::print_custom "\n"
    _read="n"
  else
    read -p "" _read
  fi
  local _confirm="${_read:-y}"

  [ -z "$EDITOR" ] && lib_utils::die_fatal
  [[ "$_confirm" == [yY] ]] && $EDITOR "$_file" || return 0
}

# vim: sw=2 sts=2 si ai et
