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

# Dependencies
deps=("docker" "sed")
lib_utils::deps_check "${deps[@]}"

if ! docker compose version 1>/dev/null; then
  lib_utils::die_fatal "Docker compose plugin not installed"
fi

if [ -z "$EDITOR" ]; then
  editors=("vim" "vi" "emacs" "nano")
  for editor in "${editors[@]}"; do
    hash "$editor" &>/dev/null && export EDITOR="$editor" && break
  done
  if [ $? -ne 0 ]; then
    lib_utils::die_fatal "Shell EDITOR is not set, export EDITOR in your shell"
  fi
fi

# Remaining "constructor" implementation
function lib_docker::__docker()
{
  # Inherited from caller
  [ -z "$global_container" ] && lib_utils::die_fatal
  [ -z "$global_image" ] && lib_utils::die_fatal
  [ -z "$global_network" ] && lib_utils::die_fatal
  [ -z "$global_platform" ] && lib_utils::die_fatal
  [ -z "$global_tag" ] && lib_utils::die_fatal

  # Inherited from caller (via `lib_env`)
  [ -z "$global_client_version" ] && lib_utils::die_fatal
  [ -z "$global_repo_dockerfiles" ] && lib_utils::die_fatal

  #
  # Generate docker-compose.yml
  #

  local _path="${global_repo_dockerfiles}/docker-compose.yml"
  lib_utils::print_debug "Generating '${_path}'"

  sed \
    -e "s|@DOCKER_FINANCE_VERSION@|${global_client_version}|g" \
    -e "s|@DOCKER_FINANCE_IMAGE@|${global_image}:${global_tag}|g" \
    -e "s|@DOCKER_FINANCE_CONTAINER@|${global_container}|g" \
    -e "s|@DOCKER_FINANCE_NETWORK@|${global_network}|g" \
    -e "/^ *#/d" -e "/^$/d" \
    "${_path}.${global_platform}.in" >"$_path" || return $?
}

# `docker compose` wrapper
function lib_docker::__docker_compose()
{
  [ -z "$global_env_file" ] && lib_utils::die_fatal
  [ -z "$global_repo_dockerfiles" ] && lib_utils::die_fatal

  [ ! -f "$global_env_file" ] \
    && lib_utils::die_fatal "$global_env_file not found! Run the gen command!"

  pushd "$global_repo_dockerfiles" 1>/dev/null || return $?
  docker compose -f docker-compose.yml --env-file "$global_env_file" "$@" || return $?
  popd 1>/dev/null || return $?
}

function lib_docker::__parse_args_build()
{
  [ -z "$global_usage" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_1" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_2" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_3" ] && lib_utils::die_fatal

  [ -z "$global_platform" ] && lib_utils::die_fatal
  [ -z "$global_command" ] && lib_utils::die_fatal

  # Re-seat global usage for tag options
  local _global_usage
  _global_usage="$(echo $global_usage | cut -d: -f1)"

  case "$global_platform" in
    archlinux | ubuntu)
      local -r _usage="
\e[32mDescription:\e[0m

  $global_command 'finance' image of given type

\e[32mUsage:\e[0m

  $ $global_usage type${global_arg_delim_2}<type>

\e[32mArguments:\e[0m

  Image type:

    type${global_arg_delim_2}<default|slim|tiny|micro>

\e[32mExamples:\e[0m

  \e[37;2m# $global_command the latest 'default' image, tagged as 'default'\e[0m
  $ ${_global_usage}:default $global_command type${global_arg_delim_2}default

  \e[37;2m# $global_command the latest 'default' image *without* 'root' module (ROOT.cern), tagged as 'slim'\e[0m
  $ ${_global_usage}:slim $global_command type${global_arg_delim_2}slim

  \e[37;2m# $global_command the latest 'slim' image *without* 'fetch' module (remote APIs), tagged as 'tiny'\e[0m
  $ ${_global_usage}:tiny $global_command type${global_arg_delim_2}tiny

  \e[37;2m# $global_command the latest 'tiny' image *without* 'track' support (time, metadata), tagged as 'micro'\e[0m
  $ ${_global_usage}:micro $global_command type${global_arg_delim_2}micro

\e[32mNotes:\e[0m

  - Image tags are *not* connected to build type
    (e.g., you can have an 'experimental' tag with a 'micro' image)

  - All builds will continue to append your custom Dockerfile (see \`edit help\`)
"
      ;;
    dev-tools)
      local -r _usage="
\e[32mDescription:\e[0m

  Build 'dev-tools' image of given type

\e[32mUsage:\e[0m

  $ $global_usage type${global_arg_delim_2}<type>

\e[32mArguments:\e[0m

  Build type:

    type${global_arg_delim_2}<default>

\e[32mNotes:\e[0m

  - All builds will continue to incorporate custom Dockerfile (see \`edit help\`)

\e[32mExamples:\e[0m

  \e[37;2m# $global_command the latest 'default' image (tagged as default\e[0m
  $ ${_global_usage}:default $global_command type${global_arg_delim_2}default
"
      ;;
    *)
      lib_utils::die_fatal "unsupported platform"
      ;;
  esac

  [ $# -eq 0 ] && lib_utils::die_usage "$_usage"

  for _arg in "$@"; do
    [[ ! "$_arg" =~ ^type[s]?${global_arg_delim_2} ]] \
      && lib_utils::die_usage "$_usage"
  done

  # Parse key for value
  for _arg in "$@"; do

    local _key="${_arg%${global_arg_delim_2}*}"
    local _len="$((${#_key} + 1))"

    if [[ "$_key" =~ ^type[s]?$ ]]; then
      local _arg_type="${_arg:${_len}}"
      [ -z "$_arg_type" ] && lib_utils::die_usage "$_usage"
    fi
  done

  IFS="$global_arg_delim_3"

  # Arg: type
  if [ ! -z "$_arg_type" ]; then
    [[ ! "$_arg_type" =~ ^default$|^slim$|^tiny$|^micro$ ]] \
      && lib_utils::die_usage "$_usage"

    declare -gr global_arg_type="$_arg_type"
  fi
}

function lib_docker::__build()
{
  lib_docker::__parse_args_build "$@"

  #
  # Generate Dockerfile
  #

  [ -z "$DOCKER_FINANCE_UID" ] && lib_utils::die_fatal
  [ -z "$DOCKER_FINANCE_GID" ] && lib_utils::die_fatal
  [ -z "$DOCKER_FINANCE_USER" ] && lib_utils::die_fatal

  [ -z "$global_repo_dockerfiles" ] && lib_utils::die_fatal

  local _final="${global_repo_dockerfiles}/Dockerfile"
  local _in_file="${global_repo_dockerfiles}/Dockerfile.${global_platform}.in"

  lib_utils::print_debug "Generating '${_final}' from '${_in_file}"

  sed \
    -e "s:@DOCKER_FINANCE_UID@:${DOCKER_FINANCE_UID}:g" \
    -e "s:@DOCKER_FINANCE_GID@:${DOCKER_FINANCE_GID}:g" \
    -e "s:@DOCKER_FINANCE_USER@:${DOCKER_FINANCE_USER}:g" \
    "$_in_file" >"$_final" || return $?

  #
  # Append platform build modules according to build type
  #

  local _files=()

  case "$global_platform" in
    archlinux | ubuntu)
      local -r _path="${global_repo_dockerfiles}/${global_platform}"
      case "$global_arg_type" in
        default)
          _files+=("${_path}/Dockerfile.track.in")
          _files+=("${_path}/Dockerfile.fetch.in")
          _files+=("${_path}/Dockerfile.root.in")
          _files+=("${_path}/Dockerfile.user.in")
          ;;
        slim)
          _files+=("${_path}/Dockerfile.track.in")
          _files+=("${_path}/Dockerfile.fetch.in")
          _files+=("${_path}/Dockerfile.user.in")
          lib_utils::print_warning "not building module: 'root'"
          ;;
        tiny)
          _files+=("${_path}/Dockerfile.track.in")
          _files+=("${_path}/Dockerfile.user.in")
          lib_utils::print_warning "not building module: 'fetch'"
          lib_utils::print_warning "not building module: 'root'"
          ;;
        micro)
          _files+=("${_path}/Dockerfile.user.in")
          lib_utils::print_warning "not building module: 'track'"
          lib_utils::print_warning "not building module: 'fetch'"
          lib_utils::print_warning "not building module: 'root'"
          ;;
        *)
          lib_utils::die_fatal "unsupported build"
          ;;
      esac
      ;;
    dev-tools)
      case "$global_arg_type" in
        default)
          local -r _path="${global_repo_dockerfiles}/ubuntu"
          _files+=("${_path}/Dockerfile.user.in")
          ;;
        *)
          lib_utils::die_fatal "'${global_arg_type}' is not supported for 'dev-tools', use 'default' instead"
          ;;
      esac
      ;;
    *)
      lib_utils::die_fatal "unsupported platform"
      ;;
  esac

  for _file in "${_files[@]}"; do
    lib_utils::print_debug "Appending '${_file}' to '${_final}'"
    sed \
      -e "s:@DOCKER_FINANCE_UID@:${DOCKER_FINANCE_UID}:g" \
      -e "s:@DOCKER_FINANCE_USER@:${DOCKER_FINANCE_USER}:g" \
      "$_file" >>"$_final" || return $?
  done

  #
  # Append to Dockerfile end-user's custom Dockerfile
  #

  [ -z "$global_custom_dockerfile" ] && lib_utils::die_fatal

  if [[ ! -f "$global_custom_dockerfile" ]]; then
    lib_utils::print_debug "'${global_custom_dockerfile}' not found, skipping"
  else
    lib_utils::print_debug "Appending '${global_custom_dockerfile}' to '${_final}'"
    sed \
      -e "s:@DOCKER_FINANCE_USER@:${DOCKER_FINANCE_USER}:g" \
      "$global_custom_dockerfile" >>"$_final" || return $?
  fi

  #
  # Finalize Dockerfile
  #

  # Guarantees that any additions to custom Dockerfile (or a missing custom)
  # will not prevent bringing `up` a container into the correct environment.
  lib_utils::print_debug "Finalizing '${_final}'"
  echo -e "# WARNING: keep at end of file\nUSER ${DOCKER_FINANCE_USER}\nWORKDIR /home/${DOCKER_FINANCE_USER}\n" \
    >>"$_final"

  # Remove all comments and empty lines
  sed -i -e "/^ *#/d" -e "/^$/d" "$_final"

  #
  # Execute
  #

  local _build_args=()
  case "$global_command" in
    build)
      _build_args+=("--pull")
      ;;
    update)
      _build_args+=("--pull" "--no-cache")
      ;;
    *)
      lib_utils::die_fatal "not implemented"
      ;;
  esac
  time lib_docker::__docker_compose build "${_build_args[@]}" docker-finance
}

function lib_docker::__update()
{
  # A wrapper to build (which should simply rebuild without cache)
  lib_docker::__build "$@"
}

function lib_docker::__up()
{
  # NOTE: for openvpn support, input this into /etc/docker/daemon.json and restart docker:
  # {
  #   "default-address-pools" : [
  #     {
  #       "base" : "172.31.0.0/16",
  #       "size" : 24
  #     }
  #   ]
  # }
  lib_utils::print_debug "Creating network"
  docker network create "$global_network" 2>/dev/null

  lib_utils::print_debug "Bringing up container"
  local _tty
  test -t 1 && _tty="t"
  lib_docker::__docker_compose up -d docker-finance "$@" \
    && docker exec -i"${_tty}" "$global_container" /bin/bash
}

function lib_docker::__down()
{
  [ -z "$global_network" ] && lib_utils::die_fatal

  lib_utils::print_debug "Bringing down container and network"
  lib_docker::__docker_compose down "$@" \
    && docker network rm "$global_network" 1>/dev/null
}

function lib_docker::__start()
{
  lib_utils::print_debug "Starting container"
  lib_docker::__docker_compose start "$@"
}

function lib_docker::__stop()
{
  lib_utils::print_debug "Stopping container"
  lib_docker::__docker_compose stop "$@"
}

function lib_docker::__backup()
{
  # NOTE: does not export to tar, simply tags
  local -r _backup="${global_image}:backup_$(date +%s)"
  if docker tag "${global_image}:${global_tag}" "$_backup"; then
    lib_utils::print_info "Created backup: ${global_image}:${global_tag} -> $_backup"
  else
    lib_utils::die_fatal "Could not create backup -> $_backup"
  fi
}

function lib_docker::__rm()
{
  [ -z "$global_image" ] && lib_utils::die_fatal
  [ -z "$global_tag" ] && lib_utils::die_fatal

  lib_utils::print_debug "Removing image"
  docker image rm "${global_image}:${global_tag}" "$@"
}

function lib_docker::__run()
{
  [ -z "$global_network" ] && lib_utils::die_fatal

  lib_utils::print_debug "Creating network"
  docker network create "$global_network" &>/dev/null

  lib_utils::print_debug "Spawning container with command '$*'"

  # Terminal requirements (e.g., for cron, don't allocate TTY)
  local _tty
  test -t 1 && _tty="t"

  # NOTE: runs bash as interactive so .bashrc doesn't `return` (sources/aliases are needed)
  lib_docker::__docker_compose run -i"${_tty}" --rm --entrypoint='/bin/bash -i -c' docker-finance "$@"
  local -r _return=$?

  lib_utils::print_debug "Removing network"
  docker network rm "$global_network" &>/dev/null # Don't force, if in use

  return $_return
}

function lib_docker::__parse_args_shell()
{
  [ -z "$global_usage" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_2" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_3" ] && lib_utils::die_fatal
  [ -z "$global_user" ] && lib_utils::die_fatal

  local -r _arg="$1"

  local -r _usage="
\e[32mDescription:\e[0m

  Open a shell as given container user

\e[32mUsage:\e[0m

  $ $global_usage user${global_arg_delim_2}<user>

\e[32mArguments:\e[0m

  Container user:

    user${global_arg_delim_2}<${global_user}|root|...>

\e[32mExamples:\e[0m

  \e[37;2m# Open shell as current user (no arg)\e[0m
  $ $global_usage

  \e[37;2m# Open shell as current user (with arg)\e[0m
  $ $global_usage user${global_arg_delim_2}${global_user}

  \e[37;2m# Open root shell\e[0m
  $ $global_usage user${global_arg_delim_2}root
"

  if [ $# -gt 1 ]; then
    lib_utils::die_usage "$_usage"
  fi

  # Get/Set user
  if [ ! -z "$_arg" ]; then
    if [[ ! "$_arg" =~ ^user[s]?${global_arg_delim_2} ]]; then
      lib_utils::die_usage "$_usage"
    fi

    # Parse key for value
    local _key="${_arg%${global_arg_delim_2}*}"
    local _len="$((${#_key} + 1))"
    if [[ "$_key" =~ ^user[s]?$ ]]; then
      local _arg_user="${_arg:${_len}}"
      if [ -z "$_arg_user" ]; then
        lib_utils::die_usage "$_usage"
      fi
      declare -gr global_arg_user="${_arg_user}"
    fi
  else
    # Use default user
    declare -gr global_arg_user="${global_user}"
  fi

  if [ -z "$global_arg_user" ]; then
    lib_utils::die_usage "$_usage"
  fi
}

function lib_docker::__shell()
{
  lib_docker::__parse_args_shell "$@"

  [ -z "$global_container" ] && lib_utils::die_fatal
  lib_utils::print_debug "Spawning shell in container '${global_container}'"

  docker exec -it --user "$global_arg_user" --workdir / "$global_container" /bin/bash
}

function lib_docker::__exec()
{
  [ -z "$DOCKER_FINANCE_DEBUG" ] && lib_utils::die_fatal

  # Allocate pseudo-TTY, if needed
  # NOTE: interactive is kept for cases in which the passed command requires it
  local _tty
  test -t 1 && _tty="t"

  local -r _exec=("docker" "exec" "-i$_tty" "$global_container" "/bin/bash" "-i" "-c" "$@")
  if [ "$DOCKER_FINANCE_DEBUG" -eq 0 ]; then
    # Silence `bash: cannot set terminal process group (-1): Inappropriate ioctl for device`
    # NOTE: lib_utils error handling will still function as expected
    "${_exec[@]}" 2>/dev/null
  else
    "${_exec[@]}"
  fi
}

function lib_docker::__parse_args_edit()
{
  [ -z "$global_usage" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_2" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_3" ] && lib_utils::die_fatal

  local -r _usage="
\e[32mDescription:\e[0m

  Edit client configuration files

\e[32mUsage:\e[0m

  $ $global_usage type${global_arg_delim_2}<type>

\e[32mArguments:\e[0m

  Configuration type:

    type${global_arg_delim_2}<env|{shell|superscript}|{build|dockerfile}>

\e[32mExamples:\e[0m

  \e[37;2m# Edit client/container environment variables\e[0m
  $ $global_usage type${global_arg_delim_2}env

  \e[37;2m# Edit client/container shell (superscript) \e[0m
  $ $global_usage type${global_arg_delim_2}shell

  \e[37;2m# Edit client's custom Dockerfile (appended to final Dockerfile) \e[0m
  $ $global_usage type${global_arg_delim_2}build

  \e[37;2m# Previous commands with alternate wordings\e[0m
  $ $global_usage type${global_arg_delim_2}superscript${global_arg_delim_3}dockerfile${global_arg_delim_3}env
"

  if [ $# -ne 1 ]; then
    lib_utils::die_usage "$_usage"
  fi

  # Get/Set type
  local -r _arg="$1"

  if [ ! -z "$_arg" ]; then
    if [[ ! "$_arg" =~ ^type[s]?${global_arg_delim_2} ]]; then
      lib_utils::die_usage "$_usage"
    fi

    # Parse key for value
    local _key="${_arg%${global_arg_delim_2}*}"
    local _len="$((${#_key} + 1))"
    if [[ "$_key" =~ ^type[s]?$ ]]; then
      local _arg_type="${_arg:${_len}}"
      if [ -z "$_arg_type" ]; then
        lib_utils::die_usage "$_usage"
      fi
    fi
  fi

  IFS="$global_arg_delim_3"

  # Arg: type
  if [ ! -z "$_arg_type" ]; then
    read -ra _read <<<"$_arg_type"

    for _type in "${_read[@]}"; do
      if [[ ! "$_type" =~ ^env$|^shell$|^superscript$|^build$|^dockerfile$ ]]; then
        lib_utils::die_usage "$_usage"
      fi
    done

    declare -gr global_arg_type=("${_read[@]}")
  fi
}

function lib_docker::__edit()
{
  lib_docker::__parse_args_edit "$@"

  [ -z "$EDITOR" ] \
    && lib_utils::die_fatal "Export EDITOR to your preferred editor"

  [ -z "$global_env_file" ] && lib_utils::die_fatal
  [ ! -f "$global_env_file" ] \
    && lib_utils::die_fatal "Environment file not found"

  [ -z "$global_superscript" ] && lib_utils::die_fatal
  [ ! -f "$global_superscript" ] \
    && lib_utils::die_fatal "Shell (superscript) file not found"

  # Run all files through one editor instance
  local _paths=()

  for _type in "${global_arg_type[@]}"; do
    case "$_type" in
      env)
        _paths+=("$global_env_file")
        ;;
      shell | superscript)
        [ -z "$global_platform" ] && lib_utils::die_fatal

        [[ "$global_platform" == "dev-tools" ]] \
          && lib_utils::die_fatal "Invalid platform, use finance image"

        _paths+=("$global_superscript")
        ;;
      build | dockerfile)
        [ -z "$global_custom_dockerfile" ] && lib_utils::die_fatal

        _paths+=("$global_custom_dockerfile")
        ;;
    esac
  done

  # Execute
  lib_utils::print_debug "Editing '${_paths[*]}'"
  $EDITOR "${_paths[@]}"
}

function lib_docker::__parse_args_version()
{
  [ -z "$global_usage" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_1" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_2" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_3" ] && lib_utils::die_fatal

  local -r _usage="
\e[32mDescription:\e[0m

  Print version information

\e[32mUsage:\e[0m

  $ $global_usage type${global_arg_delim_2}<type>

\e[32mArguments:\e[0m

  Version type:

    type${global_arg_delim_2}<client|container|short|all>

\e[32mExamples:\e[0m

  \e[37;2m# Print docker-finance client dependency versions\e[0m
  $ $global_usage type${global_arg_delim_2}client

  \e[37;2m# Print docker-finance container dependency versions\e[0m
  $ $global_usage type${global_arg_delim_2}container

  \e[37;2m# Print only docker-finance version\e[0m
  $ $global_usage type${global_arg_delim_2}short

  \e[37;2m# Print both docker-finance and client dependency versions\e[0m
  $ $global_usage type${global_arg_delim_2}client${global_arg_delim_3}short

  \e[37;2m# Print docker-finance version and all client/container dependency versions\e[0m
  $ $global_usage type${global_arg_delim_2}all
"

  [ $# -eq 0 ] && lib_utils::die_usage "$_usage"

  for _arg in "$@"; do
    [[ ! "$_arg" =~ ^type[s]?${global_arg_delim_2} ]] \
      && lib_utils::die_usage "$_usage"
  done

  # Parse key for value
  for _arg in "$@"; do

    local _key="${_arg%${global_arg_delim_2}*}"
    local _len="$((${#_key} + 1))"

    if [[ "$_key" =~ ^type[s]?$ ]]; then
      local _arg_type="${_arg:${_len}}"
      [ -z "$_arg_type" ] && lib_utils::die_usage "$_usage"
    fi
  done

  IFS="$global_arg_delim_3"

  # Arg: type
  read -ra _read <<<"$_arg_type"
  for _arg in "${_read[@]}"; do
    if [[ ! "$_arg" =~ ^client$|^container$|^short$|^all$ ]]; then
      lib_utils::die_usage "$_usage"
    fi
  done

  if [[ "${_read[*]}" =~ all ]]; then # Note: not anchored
    declare -gr global_arg_type=("client" "container" "short")
  else
    declare -gr global_arg_type=("${_read[@]}")
  fi
}

function lib_docker::__version()
{
  lib_docker::__parse_args_version "$@"

  [ -z "$global_platform" ] && lib_utils::die_fatal
  [ -z "$global_platform_image" ] && lib_utils::die_fatal

  [ -z "${global_arg_type[*]}" ] && lib_utils::die_fatal

  for _type in "${global_arg_type[@]}"; do

    case "$_type" in

      client)
        echo
        echo "client.system:"

        echo -e \\t"$(uname -rmo)"
        echo -e \\t"$(bash --version | head -n1)"
        echo -e \\t"$(sed --version | head -n1)"

        echo
        echo "client.docker:"

        echo -e \\t"$(docker compose version)"
        docker version \
          | while read _line; do
            echo -e \\t"${_line}"
          done
        ;;

      container)
        # Feed dependency manifest (client is not bind-mounted for security reasons)
        [ -z "$global_repo_manifest" ] && lib_utils::die_fatal
        local _manifest
        _manifest="$(<$global_repo_manifest)"

        # NOTE: uses variable "callback"
        case "$global_platform" in
          archlinux)
            _platform="$global_platform"
            _image="$global_platform_image"
            _pkg="pacman -Q \$_package"
            ;;
          ubuntu | dev-tools)
            _platform="ubuntu" # NOTE: current dev-tools is treated as an ubuntu-based image
            _image="$global_platform_image"
            _pkg="dpkg -s \$_package | grep -E \"(^Package:|^Version:)\" | cut -d' ' -f2 | paste -s | awk '{print \$1 \" \" \$2}'"
            ;;
          *)
            lib_utils::die_fatal "unsupported platform"
            ;;
        esac

        local -r _yq=("yq" "-M" "-y" "--indentless" "-e")
        local -r _sanitize=("sed" "'s:^- ::'" "|" "grep" "-v" "-E" "'(^null\$|\.\.\.)'")

        lib_docker::__run "
          echo '${_manifest}' \\
            | $(echo "${_yq[@]}") \".container.${_platform}.\\\"${_image}\\\" | keys\" | sed \"s:^- ::\" \\
            | while read _key; do
                echo -e \\\ncontainer.${_platform}.${_image}.\${_key}:
                echo '${_manifest}' | $(echo "${_yq[@]}") .container.${_platform}.\\\"${_image}\\\".\${_key}.packages | $(echo "${_sanitize[@]}") 2>/dev/null \\
                | while read _package; do
                    echo -e \\\t\$($_pkg)
                  done
                echo '${_manifest}' | $(echo "${_yq[@]}") .container.${_platform}.\\\"${_image}\\\".\${_key}.commands | $(echo "${_sanitize[@]}") 2>/dev/null \\
                | while read _command; do
                    echo -e \\\t\$(\$_command)
                  done
              done
          echo"
        ;;

      short)
        [ -z "$DOCKER_FINANCE_CLIENT_REPO" ] && lib_utils::die_fatal
        [ -z "$DOCKER_FINANCE_VERSION" ] && lib_utils::die_fatal

        local _hash
        if pushd "$DOCKER_FINANCE_CLIENT_REPO" 1>/dev/null; then
          _hash="$(git log --pretty='format:%h' -n 1)"
          popd 1>/dev/null || lib_utils::die_fatal
        else
          lib_utils::die_fatal
        fi

        # TODO: add build type
        echo
        echo -e "docker-finance $DOCKER_FINANCE_VERSION | commit: $_hash | platform: $global_platform | image: $global_platform_image"
        ;;

      *)
        lib_utils::die_fatal "unsupported type"
        ;;
    esac
  done
}

# vim: sw=2 sts=2 si ai et
