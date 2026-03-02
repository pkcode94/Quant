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

if [ -z "$DOCKER_FINANCE_CLIENT_REPO" ]; then
  declare -gx DOCKER_FINANCE_CLIENT_REPO
  DOCKER_FINANCE_CLIENT_REPO="$(dirname "$(realpath -s $0)" | rev | cut -d'/' -f2- | rev)"
fi

source "${DOCKER_FINANCE_CLIENT_REPO}/client/src/docker/lib/lib_docker.bash" || exit 1

#
# Execute
#

function main()
{
  [ -z "$global_basename" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_1" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_2" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_3" ] && lib_utils::die_fatal

  local _usage="
\e[32mDescription:\e[0m

  The 'docker' in 'docker-finance'

\e[32mUsage:\e[0m

  $ $global_basename <instance> <command> [args]

\e[32mInstance:\e[0m

  <platform/user:tag>

    platform    \e[34;3mBase image platform (archlinux | ubuntu | dev-tools)\e[0m
    user        \e[34;3mUser to bind-mount with (default: ${USER})\e[0m
    tag         \e[34;3mImage tag type to use (default: latest)\e[0m

\e[32mCommand:\e[0m

  Environment (all platforms):

    gen         \e[34;3mGenerate client/container environment\e[0m
    edit        \e[34;3mEdit existing client-side configurations\e[0m

  All platforms:

    build       \e[34;3mBuild a new docker-finance image\e[0m
    update      \e[34;3mUpdate an existing docker-finance image\e[0m
    backup      \e[34;3mBackup existing docker-finance image\e[0m
    rm          \e[34;3mRemove docker-finance image and network\e[0m

    up          \e[34;3mBring up docker-finance services, open shell\e[0m
    down        \e[34;3mBring down docker-finance services, remove network\e[0m
    start       \e[34;3mStart existing docker-finance container\e[0m
    stop        \e[34;3mStop running docker-finance container\e[0m

    run         \e[34;3mSpawn docker-finance container with given command (container removed on exit)\e[0m
    shell       \e[34;3mOpen shell into running docker-finance container\e[0m
    exec        \e[34;3mExecute command within running docker-finance container\e[0m

    plugins     \e[34;3mExecute a client-side categorical docker-finance plugin ('repo' or 'custom')\e[0m
    version     \e[34;3mPrint current version of docker-finance and its dependencies\e[0m

  Dev-tools platform:

    license     \e[34;3mAdd a license to a docker-finance file\e[0m
    linter      \e[34;3mLint docker-finance source files\e[0m
    doxygen     \e[34;3mGenerate Doxygen for docker-finance source files\e[0m

\e[32mOptional:\e[0m

  [args]        \e[34;3mCommand argument(s)\e[0m

\e[32mExamples:\e[0m

  \e[37;2m#\e[0m
  \e[37;2m# Finance platform\e[0m
  \e[37;2m#\e[0m

  \e[37;2m# Generate default environment and build default image\e[0m
  $ $global_basename archlinux${global_arg_delim_1}${USER}:default gen \\
    && $global_basename archlinux${global_arg_delim_1}${USER}:default build type${global_arg_delim_2}default

  \e[37;2m# Bring up container, open shell (type 'exit' to leave)\e[0m
  $ $global_basename archlinux${global_arg_delim_1}${USER}:default up

  \e[37;2m# In another shell (or after you exit), open a container root shell\e[0m
  $ $global_basename archlinux${global_arg_delim_1}${USER}:default shell user${global_arg_delim_2}root

  \e[37;2m# In another shell (or after you exit), edit client/container variables\e[0m
  $ $global_basename archlinux${global_arg_delim_1}${USER}:default edit type${global_arg_delim_2}env

  \e[37;2m# Spawn a container with given command (removed after command finishes)\e[0m
  \e[37;2m# NOTE: incredibly useful when used with your host's crontab\e[0m
  $ $global_basename archlinux${global_arg_delim_1}${USER}:default run 'dfi family/alice fetch all${global_arg_delim_2}price'

  \e[37;2m# Bring down running container (stop and remove container & network)\e[0m
  $ $global_basename archlinux${global_arg_delim_1}${USER}:default down

  \e[37;2m# Backup image, delete old image, build new image\e[0m
  $ $global_basename archlinux${global_arg_delim_1}${USER}:default backup \\
    && $global_basename archlinux${global_arg_delim_1}${USER}:default rm \\
    && $global_basename archlinux${global_arg_delim_1}${USER}:default build

  \e[37;2m# Print current version of 'docker-finance' and client/container ('finance') dependencies\e[0m
  $ $global_basename archlinux${global_arg_delim_1}${USER}:default version type${global_arg_delim_2}all

  \e[37;2m# Print Tor plugin's usage help\e[0m
  $ $global_basename archlinux${global_arg_delim_1}${USER}:default plugins repo${global_arg_delim_1}tor.bash help

  \e[37;2m#\e[0m
  \e[37;2m# Dev-tools platform\e[0m
  \e[37;2m#\e[0m

  \e[37;2m# Generate default environment and build default image\e[0m
  $ $global_basename dev-tools${global_arg_delim_1}${USER}:default gen \\
    && $global_basename dev-tools${global_arg_delim_1}${USER}:default build type${global_arg_delim_2}default

  \e[37;2m# Spawn a container with given command (removed after command finishes)\e[0m
  $ $global_basename dev-tools${global_arg_delim_1}${USER}:default run 'shellcheck --version'

  \e[37;2m# Lint entire docker-finance source\e[0m
  $ $global_basename dev-tools${global_arg_delim_1}${USER}:default linter type${global_arg_delim_2}bash${global_arg_delim_3}php${global_arg_delim_3}c++

  \e[37;2m# Generate Doxygen for docker-finance source\e[0m
  $ $global_basename dev-tools${global_arg_delim_1}${USER}:default doxygen gen

  \e[37;2m# Print current version of 'docker-finance' and client/container ('dev-tools') dependencies\e[0m
  $ $global_basename dev-tools${global_arg_delim_1}${USER}:default version type${global_arg_delim_2}all

\e[32mTips:\e[0m

  - Save your fingers! Use tab completion for all images and commands (see README)
  - Setup aliases for frequently used images (useful for single-user systems)
    * Depending on your alias design, tab completion may not be available
"

  #
  # "Constructor" (initilizes client environment)
  #

  lib_docker::docker "$@" || lib_utils::die_usage "$_usage"

  # shellcheck disable=SC2154
  [ "$DOCKER_FINANCE_DEBUG" == 2 ] && set -xv

  #
  # Command facade
  #

  # shellcheck disable=SC2154
  case "$global_command" in
    gen)
      lib_docker::gen "${@:3}"
      ;;
    edit)
      lib_docker::edit "${@:3}"
      ;;
    build)
      lib_docker::build "${@:3}"
      ;;
    update)
      lib_docker::update "${@:3}"
      ;;
    up)
      lib_docker::up "${@:3}"
      ;;
    down)
      lib_docker::down "${@:3}"
      ;;
    start)
      lib_docker::start "${@:3}"
      ;;
    stop)
      lib_docker::stop "${@:3}"
      ;;
    rm)
      lib_docker::rm "${@:3}"
      ;;
    run)
      lib_docker::run "${@:3}"
      ;;
    shell)
      lib_docker::shell "${@:3}"
      ;;
    exec)
      lib_docker::exec "${@:3}"
      ;;
    backup)
      lib_docker::backup "${@:3}"
      ;;
    license)
      lib_docker::license "${@:3}"
      ;;
    linter)
      lib_docker::linter "${@:3}"
      ;;
    doxygen)
      lib_docker::doxygen "${@:3}"
      ;;
    version)
      lib_docker::version "${@:3}"
      ;;
    plugins)
      lib_plugins::plugins "${@:3}"
      ;;
    *)
      lib_utils::die_usage "$_usage"
      ;;
  esac
}

main "$@"

# vim: sw=2 sts=2 si ai et
