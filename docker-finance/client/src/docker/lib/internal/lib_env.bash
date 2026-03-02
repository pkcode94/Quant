#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2021-2026 Aaron Fiore (Founder, Evergreen Crypto LLC)
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

# Utilities (a container library (but container is never exposed to client code))
source "${DOCKER_FINANCE_CLIENT_REPO}/container/src/finance/lib/internal/lib_utils.bash" || exit 1

#
# Implementation
#

if [ $UID -lt 1000 ]; then
  [ $UID -eq 0 ] \
    && lib_utils::die_fatal "Cannot run as root!" \
    || lib_utils::print_warning "Running as system user"
fi

# Dependencies
deps=("sed")
lib_utils::deps_check "${deps[@]}"

# IMPORTANT: keep umask for security
umask o-rwx

#
# "Constructor" for environment generation
#
#   1. Sets client-side environment with defaults or use existing environment
#   2. If configured, resets to alternative environment configuration after bootstrap
#
# NOTE: some bootstrapped defaults are ignored by environment file (as seen below)
#

function lib_env::env()
{
  [ -z "$DOCKER_FINANCE_CLIENT_REPO" ] && lib_utils::die_fatal

  # NOTE: global_* *MUST* be reset after sourcing new env file

  #
  # Generate `docker-finance` version
  #

  global_repo_manifest="${DOCKER_FINANCE_CLIENT_REPO}/client/docker-finance.yaml"
  declare -g global_repo_manifest

  global_client_version="$(grep '^version: ' $global_repo_manifest | sed -e 's/version: "//' -e 's/"//g')"
  # shellcheck disable=SC2034  # used during env gen
  declare -g global_client_version

  #
  # If empty environment:
  #
  #   1. Poke at possible (default) location of end-user environment file
  #     a. If found, point to new location and read/reset from there
  #
  #   2. If file not found, bootstrap with defaults
  #

  # Environment (end-user)
  global_conf_filename="${USER}@$(uname -n)"
  declare -g global_conf_filename

  # Environment (end-user) tag dir (not full path)
  [ -z "$global_tag" ] && lib_utils::die_fatal
  global_tag_dir="client/$(uname -s)-$(uname -m)/${global_platform}/${global_tag}"
  declare -g global_tag_dir

  # Environment location
  # NOTE: keep aligned with gen.bash
  local _env_dir
  _env_dir="$(realpath -s "${BASH_SOURCE[0]}" | rev | cut -d'/' -f7- | rev)/conf.d/${global_tag_dir}/env"
  local _env_file="${_env_dir}/${global_conf_filename}"

  # shellcheck source=/dev/null
  [ -f "$_env_file" ] && source "$_env_file"

  if [ -z "$DOCKER_FINANCE_CLIENT_CONF" ]; then
    # shellcheck source=/dev/null
    source "${DOCKER_FINANCE_CLIENT_REPO}/client/conf.d/client/env/gen.bash"
  fi

  [ -z "$DOCKER_FINANCE_CLIENT_CONF" ] \
    && lib_utils::die_fatal "Defaults not generated! (${global_repo_env_file})"

  lib_env::__set_client_globals

  #
  # Reset environment with user-provided (user-defined) existing file (if available)
  #

  _env_dir="${DOCKER_FINANCE_CLIENT_CONF}/${global_tag_dir}/env"
  _env_file="${_env_dir}/${global_conf_filename}"

  if [ -f "$_env_file" ]; then
    if [ -s "$_env_file" ]; then
      # Re-bootstrap with (new) environment
      lib_utils::print_debug "Environment found! Using '${_env_file}'"
      lib_env::__read "$_env_file"
      lib_env::__set_client_globals
    else
      lib_utils::print_warning \
        "Client environment '${_env_file}' is empty! Writing defaults"
      lib_env::__write "$_env_file"
    fi
  else
    # shellcheck disable=SC2154
    if [ "$global_command" != "gen" ]; then
      lib_utils::die_fatal \
        "Client environment not found! Run 'gen' command"
    fi

    lib_utils::print_info \
      "Client environment not found, writing defaults to '${_env_file}'"
    lib_env::__write "$_env_file"
  fi
}

#
# Set client globals from environment
#

function lib_env::__set_client_globals()
{
  lib_utils::print_debug "Setting (or resetting) client globals"

  #
  # Repository env
  #

  [ -z "$DOCKER_FINANCE_CLIENT_REPO" ] && lib_utils::die_fatal

  # Generate `docker-finance` version
  global_repo_manifest="${DOCKER_FINANCE_CLIENT_REPO}/client/docker-finance.yaml"
  declare -g global_repo_manifest
  lib_utils::print_debug "global_repo_manifest=${global_repo_manifest}"

  global_client_version="$(grep '^version: ' $global_repo_manifest | sed -e 's/version: "//' -e 's/"//g')"
  # shellcheck disable=SC2034  # used during env gen
  declare -g global_client_version
  lib_utils::print_debug "global_client_version=${global_client_version}"

  # This does not reset when reading env; export again
  export DOCKER_FINANCE_VERSION="$global_client_version"
  lib_utils::print_debug "DOCKER_FINANCE_VERSION=${DOCKER_FINANCE_VERSION}"

  # Repository-provided (not user-defined) default environment
  global_repo_conf_dir="${DOCKER_FINANCE_CLIENT_REPO}/client/conf.d"
  declare -g global_repo_conf_dir
  lib_utils::print_debug "global_repo_conf_dir=${global_repo_conf_dir}"

  # Environment (repo)
  declare -g global_repo_env_file="${global_repo_conf_dir}/client/env/gen.bash"
  [ ! -f "$global_repo_env_file" ] \
    && lib_utils::die_fatal "Missing environment defaults! ($global_repo_env_file)"
  lib_utils::print_debug "global_repo_env_file=${global_repo_env_file}"

  #
  # Repository env (Dockerfiles)
  #

  # Set image type
  [ -z "$global_platform" ] && lib_utils::die_fatal
  case "$global_platform" in
    archlinux | ubuntu)
      global_platform_image="finance"
      ;;
    dev-tools)
      global_platform_image="dev-tools"
      ;;
    *)
      lib_utils::die_fatal "unsupported platform"
      ;;
  esac
  declare -g global_platform_image
  lib_utils::print_debug "global_platform_image=${global_platform_image}"

  # Base location of Docker-related files (.in and final out files)
  global_repo_dockerfiles="${DOCKER_FINANCE_CLIENT_REPO}/client/Dockerfiles/local/${global_platform_image}"
  # shellcheck disable=SC2034  # used in lib_docker
  declare -g global_repo_dockerfiles
  lib_utils::print_debug "global_repo_dockerfiles=${global_repo_dockerfiles}"

  # Base custom end-user .in Dockerfile (to be appended to final Dockerfile after installation)
  declare -g global_repo_custom_dockerfile="${global_repo_conf_dir}/client/Dockerfiles/${global_platform_image}/Dockerfile.${global_platform}.in"
  [ ! -f "$global_repo_custom_dockerfile" ] \
    && lib_utils::die_fatal "Missing default custom Dockerfile '${global_repo_custom_dockerfile}'"
  lib_utils::print_debug "global_repo_custom_dockerfile=${global_repo_custom_dockerfile}"

  #
  # Client-side env
  #

  [ -z "$global_tag_dir" ] && lib_utils::die_fatal

  # Environment (end-user) format
  [ -z "$DOCKER_FINANCE_USER" ] && lib_utils::die_fatal
  global_conf_filename="${DOCKER_FINANCE_USER}@$(uname -n)"
  declare -g global_conf_filename
  lib_utils::print_debug "global_conf_filename=${global_conf_filename}"

  # Environment file (if available)
  local _client_env_dir="${DOCKER_FINANCE_CLIENT_CONF}/${global_tag_dir}/env"
  [ ! -d "$_client_env_dir" ] && mkdir -p "$_client_env_dir"
  global_env_file="${_client_env_dir}/${global_conf_filename}"
  lib_utils::print_debug "global_env_file=${global_env_file}"

  # Custom Dockerfile (if available)
  local _client_dockerfile_dir="${DOCKER_FINANCE_CLIENT_CONF}/${global_tag_dir}/Dockerfiles"
  [ ! -d "$_client_dockerfile_dir" ] && mkdir -p "$_client_dockerfile_dir"
  global_custom_dockerfile="${_client_dockerfile_dir}/${global_conf_filename}"
  lib_utils::print_debug "global_custom_dockerfile=${global_custom_dockerfile}"

  # NOTE:
  #
  #  Client env tag format is avoided because:
  #
  #  - We copy over static bash_aliases in Dockerfile,
  #    and superscript must be referenced by that static path
  #
  #  - The needed dynamicness appears to not be satisfied via docker-compose

  local _client_shell_dir="${DOCKER_FINANCE_CLIENT_CONF}/container/shell"
  [ ! -d "$_client_shell_dir" ] && mkdir -p "$_client_shell_dir"
  global_superscript="${_client_shell_dir}/superscript.bash"
  lib_utils::print_debug "global_superscript=${global_superscript}"

  # Client view of client portion of repository
  global_repo_client="${DOCKER_FINANCE_CLIENT_REPO}/client"
  [ ! -d "$global_repo_client" ] && lib_utils::die_fatal "Repository '${global_repo_client}' not found!"
  lib_utils::print_debug "global_repo_client=${global_repo_client}"

  # Backup-file extension
  # TODO: make configurable
  global_suffix="$(date +%Y-%m-%d_%H:%M:%S)"
  lib_utils::print_debug "global_suffix=${global_suffix}"
}

#
# Get/Set client-side environment with given file
#

function lib_env::__read()
{
  local _file="$1"

  # Get environment
  local _env=()
  while read _line; do
    _env+=("$_line")
  done < <(printenv | grep -Eo "^DOCKER_FINANCE[^=]+")

  # Reset environment
  for _line in "${_env[@]}"; do
    lib_utils::print_debug "Unsetting $_line"
    unset "$_line"
  done

  # Set, if a script that generates env
  if [[ "$(head -n1 $_file)" =~ (bin|env|sh|bash) ]]; then
    # shellcheck source=/dev/null
    source "$global_repo_env_file"
    return $?
  fi

  # Set, if env file format (docker / bash)
  while read _line; do
    # Ignore comments
    if [[ ! "$_line" =~ ^# ]]; then
      # Don't allow manipulating version via file
      if [[ "$_line" =~ ^DOCKER_FINANCE_VERSION ]]; then
        continue
      fi
      # Export valid line
      export "${_line?}" # SC2163
      lib_utils::print_debug "$_line"
    fi
  done <"$_file"
}

#
# Write client-side environment to given file
#

function lib_env::__write()
{
  lib_utils::print_debug "Writing environment"

  unset DOCKER_FINANCE_VERSION # Must be generated internally
  printenv | grep -E "DOCKER_FINANCE" | sort >"$1"
}

# vim: sw=2 sts=2 si ai et
