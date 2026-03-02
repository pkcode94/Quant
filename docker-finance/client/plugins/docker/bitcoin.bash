#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2025-2026 Aaron Fiore (Founder, Evergreen Crypto LLC)
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
# NOTES:
#
#   This plugin provides a rudimentary clone/pull/build process that
#   adheres to the expectations from the container-side bitcoin plugin.
#
#   The only requirement for this plugin is that build dependencies
#   are installed on your existing/running image (which is assumed to
#   have also been built with the `root` module enabled):
#
#     1. Uncomment plugin's image build requirements:
#
#          $ dfi <platform/user:tag> edit type=build
#
#     2. Build/re-build your image with `root` module:
#
#          $ dfi <platform/user:tag> build type=default
#
#   If you do bitcoin development on your client (host) and prefer to
#   use your own build process, you can skip the above requirements and,
#   instead, ensure the following:
#
#     1. Your bitcoin repository is dropped into your client-side shared
#        directory:
#
#          $DOCKER_FINANCE_CLIENT_SHARED
#
#        The expected path by the container-side plugin will be in your
#        container-side shared directory:
#
#          "${DOCKER_FINANCE_CONTAINER_SHARED}"/bitcoin
#
#        To see or edit your shared client/container path prior to
#        running this (or the container's) plugin, run the following:
#
#          $ dfi <platform/user:tag> edit type=env
#
#     2. Bitcoin's libraries are built as a shared (when possible) and
#        should continue to remain within the repo's build directory,
#        prior to running the container-side `root` bitcoin plugin.
#

#
# "Libraries"
#

[ -z "$DOCKER_FINANCE_CLIENT_REPO" ] && exit 1
source "${DOCKER_FINANCE_CLIENT_REPO}/client/src/docker/lib/lib_docker.bash" || exit 1

[[ -z "$global_platform" || -z "$global_arg_delim_1" || -z "$global_user" || -z "$global_tag" ]] && lib_utils::die_fatal
instance="${global_platform}${global_arg_delim_1}${global_user}:${global_tag}"

# Initialize "constructor"
# NOTE: "constructor" only needed if calling library directly
lib_docker::docker "$instance" || lib_utils::die_fatal

#
# Implementation
#

# WARNING: *MUST* be synced with image (assuming Arch Linux finance image)
declare -r plugin_bitcoin_deps=("boost" "capnproto" "cmake" "db")

# WARNING: *MUST* be synced with container's plugin implementation
[ -z "$DOCKER_FINANCE_CONTAINER_SHARED" ] && lib_utils::die_fatal
declare -r plugin_bitcoin_path="${DOCKER_FINANCE_CONTAINER_SHARED}/bitcoin"

function bitcoin::__parse_args()
{
  [ -z "$global_usage" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_1" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_2" ] && lib_utils::die_fatal

  local -r _default_arg_remote="https://github.com/bitcoin/bitcoin"
  local -r _default_arg_branch="master"
  local -r _default_arg_cmake="-DBUILD_BITCOIN_BIN=OFF -DBUILD_CLI=OFF -DBUILD_DAEMON=OFF -DBUILD_TESTS=OFF -DBUILD_UTIL=OFF -DBUILD_WALLET_TOOL=OFF -DINSTALL_MAN=OFF -DSECP256K1_BUILD_TESTS=OFF -DSECP256K1_BUILD_EXHAUSTIVE_TESTS=OFF -DBUILD_KERNEL_LIB=ON -DBUILD_SHARED_LIBS=ON"

  # Re-seat global usage
  local _global_usage
  _global_usage="$global_usage plugins repo${global_arg_delim_1}$(basename $0)"
  declare -r _global_usage

  local -r _usage="
\e[32mDescription:\e[0m

  Build bitcoin libraries for \`root\` container-side plugin.

\e[32mUsage:\e[0m

  $ $_global_usage <get | build | clean> [remote${global_arg_delim_2}<remote>] [branch${global_arg_delim_2}<branch>] [cmake${global_arg_delim_2}<cmake>]

\e[32mArguments:\e[0m

  Git remote to get; default:

    remote${global_arg_delim_2}\"$_default_arg_remote\"

  Git branch to build from; default:

    branch${global_arg_delim_2}\"$_default_arg_branch\"

  CMake build options; default:

    cmake${global_arg_delim_2}\"$_default_arg_cmake\"

\e[32mExamples:\e[0m

  \e[37;2m# Get from default\e[0m
  $ $_global_usage get

  \e[37;2m# Get from given remote 'https://my.custom.remote/bitcoin'\e[0m
  $ $_global_usage get remote${global_arg_delim_2}\"https://my.custom.remote/bitcoin\"

  \e[37;2m# Build default branch\e[0m
  $ $_global_usage build

  \e[37;2m# Checkout and build given branch\e[0m
  $ $_global_usage build branch=\"my_dev\"

  \e[37;2m# Checkout and build given commit\e[0m
  $ $_global_usage build branch=\"b30262dcaa28c40a0a5072847b7194b3db203160\"

  \e[37;2m# Build given branch with custom CMake options\e[0m
  $ $_global_usage build branch=\"my_dev\" cmake${global_arg_delim_2}\"-DBUILD_BITCOIN_BIN=ON -DBUILD_CLI=ON -DBUILD_DAEMON=ON -DBUILD_TESTS=ON -DBUILD_UTIL=ON -DBUILD_WALLET_TOOL=ON -DINSTALL_MAN=ON -DSECP256K1_BUILD_TESTS=ON -DSECP256K1_BUILD_EXHAUSTIVE_TESTS=ON -DBUILD_KERNEL_LIB=ON -DBUILD_SHARED_LIBS=ON\"

  \e[37;2m# Clean entire build directory\e[0m
  $ $_global_usage clean
"

  [ $# -eq 0 ] && lib_utils::die_usage "$_usage"

  [[ ! "$1" =~ ^get$|^build$|^clean$ ]] \
    && lib_utils::die_usage "$_usage" \
    || declare -gr plugin_arg_cmd="$1"

  for _arg in "${@:2}"; do
    [[ ! "$_arg" =~ ^remote${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^branch${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^cmake${global_arg_delim_2} ]] \
      && lib_utils::die_usage "$_usage"
  done

  for _arg in "${@:2}"; do
    local _key="${_arg%${global_arg_delim_2}*}"
    local _len="$((${#_key} + 1))"

    if [[ "$_key" =~ ^remote$ ]]; then
      local _arg_remote="${_arg:${_len}}"
      [ -z "$_arg_remote" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^branch$ ]]; then
      local _arg_branch="${_arg:${_len}}"
      [ -z "$_arg_branch" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^cmake$ ]]; then
      local _arg_cmake="${_arg:${_len}}"
      [ -z "$_arg_cmake" ] && lib_utils::die_usage "$_usage"
    fi
  done

  # Arg: get
  if [[ "$plugin_arg_cmd" =~ ^get$ ]]; then
    if [[ ! -z "$_arg_branch" || ! -z "$_arg_cmake" ]]; then
      lib_utils::die_usage "$_usage"
    fi

    # Arg: remote
    if [ -z "$_arg_remote" ]; then
      _arg_remote="$_default_arg_remote"
    fi
    declare -gr plugin_arg_remote="$_arg_remote"
    lib_utils::print_info "Using remote: ${_arg_remote}"
  fi

  # Arg: build
  if [[ "$plugin_arg_cmd" =~ ^build$ ]]; then
    if [[ ! -z "$_arg_remote" ]]; then
      lib_utils::die_usage "$_usage"
    fi

    # Arg: branch
    if [ -z "$_arg_branch" ]; then
      _arg_branch="$_default_arg_branch"
    fi
    declare -gr plugin_arg_branch="$_arg_branch"
    lib_utils::print_info "Using branch: ${plugin_arg_branch}"

    # Arg: cmake
    if [ -z "$_arg_cmake" ]; then
      _arg_cmake="$_default_arg_cmake"
    fi
    declare -gr plugin_arg_cmake="$_arg_cmake"
    lib_utils::print_info "Using CMake options: ${plugin_arg_cmake}"
  fi

  # Arg: clean
  if [[ "$plugin_arg_cmd" =~ ^clean$ ]]; then
    if [[ ! -z "$_arg_remote" || ! -z "$_arg_branch" || ! -z "$_arg_cmake" ]]; then
      lib_utils::die_usage "$_usage"
    fi
  fi
}

function bitcoin::__getter()
{
  [ -z "$plugin_arg_remote" ] && lib_utils::die_fatal
  [ -z "$plugin_bitcoin_path" ] && lib_utils::die_fatal

  lib_docker::exec "if [ ! -d $plugin_bitcoin_path ]; then git clone $plugin_arg_remote $plugin_bitcoin_path ; fi" \
    || lib_utils::die_fatal "Could not clone '${plugin_arg_remote}' to '${plugin_bitcoin_path}'"

  lib_docker::exec "pushd $plugin_bitcoin_path 1>/dev/null && git pull $plugin_arg_remote --tags && popd 1>/dev/null" \
    || lib_utils::die_fatal "Could not pull '${plugin_arg_remote}' to '${plugin_bitcoin_path}'"
}

function bitcoin::__builder()
{
  #
  # Ensure dependencies
  #

  local -r _no_deps="Build dependencies not found!

  Did you uncomment them with:

\t\$ dfi $instance edit type=build

  and then re-build your image with:

\t\$ dfi $instance build type=<type>"

  # NOTE: *MUST* be synced with end-user Dockerfile (`edit type=build`)
  # TODO: not ideal, would prefer runtime dependency pulling but that has drawbacks as well
  [ -z "${plugin_bitcoin_deps[*]}" ] && lib_utils::die_fatal
  lib_docker::exec "pacman -Q ${plugin_bitcoin_deps[*]}" | grep "^error: package" \
    && lib_utils::die_fatal "$_no_deps"

  #
  # Execute build
  #

  [ -z "$plugin_arg_cmake" ] && lib_utils::die_fatal
  [ -z "$plugin_bitcoin_path" ] && lib_utils::die_fatal

  lib_docker::exec "pushd $plugin_bitcoin_path 1>/dev/null && git checkout $plugin_arg_branch && popd 1>/dev/null" \
    || lib_utils::die_fatal "Could not checkout '${plugin_arg_branch}'"

  lib_docker::exec "cmake $plugin_arg_cmake -B ${plugin_bitcoin_path}/build $plugin_bitcoin_path" \
    || lib_utils::die_fatal "Could not prepare build with given options"

  lib_docker::exec "cmake --build ${plugin_bitcoin_path}/build -j$(nproc)" \
    || lib_utils::die_fatal "Could not build"

  lib_utils::print_info "$(lib_docker::exec "find ${plugin_bitcoin_path} -name libbitcoinkernel.so")"
}

function bitcoin::__cleaner()
{
  [ -z "$plugin_bitcoin_path" ] && lib_utils::die_fatal
  local -r _build="${plugin_bitcoin_path}/build"

  lib_utils::print_info "Removing '${_build}'"
  lib_docker::exec "rm -fr $_build" || lib_utils::die_fatal "Could not delete= '${_build}'"
}

#
# Facade
#

function bitcoin::get()
{
  lib_utils::print_info "Getting repository"
  bitcoin::__getter
}

function bitcoin::build()
{
  lib_utils::print_info "Building repository"
  bitcoin::__builder
}

function bitcoin::clean()
{
  lib_utils::print_info "Cleaning repository"
  bitcoin::__cleaner
}

function main()
{
  bitcoin::__parse_args "$@"

  # Ensure running instance
  local -r _no_instance="Did you start a running instance?

  Run the following in a separate shell:

\t$ dfi $instance up"

  lib_docker::exec "" || lib_utils::die_fatal "$_no_instance"

  [ -z "$plugin_arg_cmd" ] && lib_utils::die_fatal
  case "$plugin_arg_cmd" in
    get)
      bitcoin::get
      ;;
    build)
      bitcoin::build
      ;;
    clean)
      bitcoin::clean
      ;;
    *)
      lib_utils::die_fatal "Not implemented"
      ;;
  esac
}

main "$@"

# vim: sw=2 sts=2 si ai et
