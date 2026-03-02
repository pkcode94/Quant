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

[ -z "$DOCKER_FINANCE_CLIENT_REPO" ] && exit 1

#
# "Libraries"
#

# Docker impl
source "${DOCKER_FINANCE_CLIENT_REPO}/client/src/docker/lib/internal/lib_docker.bash" || exit 1

# Runtime environment handler
source "${DOCKER_FINANCE_CLIENT_REPO}/client/src/docker/lib/internal/lib_env.bash" || exit 1

# Environment layout generator
source "${DOCKER_FINANCE_CLIENT_REPO}/client/src/docker/lib/internal/lib_gen.bash" || exit 1

# Plugins support
source "${DOCKER_FINANCE_CLIENT_REPO}/client/src/docker/lib/internal/lib_plugins.bash" || exit 1

# Development tools
source "${DOCKER_FINANCE_CLIENT_REPO}/client/src/docker/lib/internal/dev-tools/lib_doxygen.bash" || exit 1
source "${DOCKER_FINANCE_CLIENT_REPO}/client/src/docker/lib/internal/dev-tools/lib_license.bash" || exit 1
source "${DOCKER_FINANCE_CLIENT_REPO}/client/src/docker/lib/internal/dev-tools/lib_linter.bash" || exit 1

# Utilities (a container library (but container is never exposed to client code))
source "${DOCKER_FINANCE_CLIENT_REPO}/container/src/finance/lib/internal/lib_utils.bash" || exit 1

#
# Implementation
#

# Top-level caller
global_basename="dfi"
declare -rx global_basename

# Globals argument delimiters
# NOTE: argument parsing will use this unified format
[ -z "$global_arg_delim_1" ] && declare -rx global_arg_delim_1="/"
[ -z "$global_arg_delim_2" ] && declare -rx global_arg_delim_2="="
[ -z "$global_arg_delim_3" ] && declare -rx global_arg_delim_3=","

# "Constructor" for client environment
function lib_docker::docker()
{
  # Instance format:
  #
  #   docker-finance/platform/user:tag [command]
  #
  # Where user supplies:
  #
  #   platform/user:tag [command]
  #
  # Where [command] is optional if only instantiating library

  [ -z "$1" ] && return 2
  [[ ! "$1" =~ $global_arg_delim_1 ]] && return 2
  [ -z "$2" ] && lib_utils::print_debug "command not given, assuming unused"

  # Parse image
  IFS="/" read -ra _image <<<"$1"
  declare -gx global_platform="${_image[0]}"
  local -r _user="${_image[1]}"
  [ -z "$_user" ] && lib_utils::die_fatal "User not given for '${_image[*]}'"
  lib_utils::print_debug "global_platform=${global_platform}"

  if [[ ! "$global_platform" =~ ^archlinux$|^ubuntu$|^dev-tools$ ]]; then
    lib_utils::die_fatal "Unsupported platform '${global_platform}'"
  fi

  # Parse tag
  IFS=":" read -ra _tag <<<"$_user"
  declare -gx global_user="${_tag[0]}"
  declare -gx global_tag="${_tag[1]}"
  [ -z "$global_tag" ] && global_tag="latest" # TODO: needs to make sense, actually have version-related functionality with client conf
  lib_utils::print_debug "global_user=${global_user}"
  lib_utils::print_debug "global_tag=${global_tag}"

  # Set instance name
  declare -gxr global_image="docker-finance/${global_platform}/${global_user}"
  declare -gxr global_container="docker-finance_${global_platform}_${global_user}_${global_tag}"
  declare -gxr global_network="docker-finance"
  lib_utils::print_debug "global_image=${global_image}"
  lib_utils::print_debug "global_container=${global_container}"
  lib_utils::print_debug "global_network=${global_network}"

  # Instance command
  declare -gxr global_command="$2"
  lib_utils::print_debug "global_command=${global_command}"

  # Convenience usage
  declare -gxr global_usage="$global_basename ${global_platform}${global_arg_delim_1}${global_user}:${global_tag} $global_command"
  lib_utils::print_debug "global_usage=${global_usage}"

  # Setup remaining environment globals
  lib_env::env || return $?

  # Remaining "constructor" implementation
  lib_docker::__docker || return $?

  return 0
}

#
# Generate client/container environment
#

function lib_docker::gen()
{
  lib_gen::gen "$@"
  lib_utils::catch $?
}

#
# Build docker-finance image
#

function lib_docker::build()
{
  lib_docker::__build "$@"
  lib_utils::catch $?
}

#
# Update docker-finance image
#

function lib_docker::update()
{
  lib_docker::__update "$@"
  lib_utils::catch $?
}

#
# Bring up docker-finance services
#

function lib_docker::up()
{
  lib_docker::__up "$@"
  lib_utils::catch $?
}

#
# Bring down docker-finance services
#

function lib_docker::down()
{
  lib_docker::__down "$@"
  lib_utils::catch $?
}

#
# Start docker-finance services
#

function lib_docker::start()
{
  lib_docker::__start "$@"
  lib_utils::catch $?
}

#
# Stop docker-finance services
#

function lib_docker::stop()
{
  lib_docker::__stop "$@"
  lib_utils::catch $?
}

#
# Remove docker-finance image
#

function lib_docker::rm()
{
  lib_docker::__rm "$@"
  lib_utils::catch $?
}

#
# Spawn a docker-finance container with given command
#

function lib_docker::run()
{
  lib_docker::__run "$@"
  lib_utils::catch $?
}

#
# Open shell into docker-finance container
#

function lib_docker::shell()
{
  lib_docker::__shell "$@"
  lib_utils::catch $?
}

#
# Execute command within a running docker-finance container
#

function lib_docker::exec()
{
  lib_docker::__exec "$@"
  lib_utils::catch $?
}

#
# Edit client configuration of given docker-finance instance
#

function lib_docker::edit()
{
  lib_docker::__edit "$@"
  lib_utils::catch $?
}

#
# Backup docker-finance image
#

function lib_docker::backup()
{
  lib_docker::__backup
  lib_utils::catch $?
}

#
# Generate docker-finance license for source file
#

function lib_docker::license()
{
  [[ "$global_platform" != "dev-tools" ]] \
    && lib_utils::die_fatal "Invalid platform, use 'dev-tools'"

  lib_license::license "$@"
  lib_utils::catch $?
}

#
# Lint docker-finance source files
#

function lib_docker::linter()
{
  [[ "$global_platform" != "dev-tools" ]] \
    && lib_utils::die_fatal "Invalid platform, use 'dev-tools'"

  lib_linter::linter "$@"
  lib_utils::catch $?
}

#
# Generate Doxygen for docker-finance source files
#

function lib_docker::doxygen()
{
  [[ "$global_platform" != "dev-tools" ]] \
    && lib_utils::die_fatal "Invalid platform, use 'dev-tools'"

  lib_doxygen::doxygen "$@"
  lib_utils::catch $?
}

#
# Prints `docker-finance` version (and dependencies' version)
#

function lib_docker::version()
{
  lib_docker::__version "$@"
  lib_utils::catch $?
}

#
# Executes given client plugin
#

function lib_docker::plugins()
{
  lib_plugins::__plugins "$@"
  lib_utils::catch $?
}

# vim: sw=2 sts=2 si ai et
