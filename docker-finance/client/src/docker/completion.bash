#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2024-2026 Aaron Fiore (Founder, Evergreen Crypto LLC)
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

# WARNING: because of completion, the docker-finance environment file
# is never read. Ergo, for debugging, you'll need to run the following
# with log-level 1 or 2:
#
#   `export DOCKER_FINANCE_DEBUG=2 && . ~/.bashrc`
#
# and then proceed to call `docker-finance` / `dfi` with your commands.

# shellcheck disable=SC2154
[ "$DOCKER_FINANCE_DEBUG" == 2 ] && set -xv

# If not yet installed, gracefully exit
if ! hash docker &>/dev/null; then
  echo -e "\e[33;1m [WARN] Docker installation not found for docker-finance!\e[0m" >&2
  return
fi

# TODO: HACK: needed for `plugins` completion
declare -gx instance

function docker-finance::completion()
{
  local _cur="${COMP_WORDS[COMP_CWORD]}"
  local _prev="${COMP_WORDS[COMP_CWORD - 1]}"

  local _images
  mapfile -t _images < <(docker images --format "{{.Repository}}:{{.Tag}}" --filter=reference='docker-finance/*/*:*' | cut -d'/' -f2- | grep $USER)
  declare -r _images

  local -r _commands=("gen" "edit" "build" "update" "backup" "rm" "up" "down" "start" "stop" "run" "shell" "exec" "version" "license" "linter" "doxygen" "plugins")

  local _reply

  case $COMP_CWORD in
    1)
      mapfile -t _reply < <(compgen -W "${_images[*]}" -- "$_cur")
      declare -r _reply
      # TODO: HACK for colon tag. Consider using bash-completion package.
      for _r in "${_reply[@]}"; do
        COMPREPLY+=("'${_r}'")
      done
      instance="${COMPREPLY[*]}"
      ;;
    2)
      mapfile -t _reply < <(compgen -W "${_commands[*]}" -- "$_cur")
      declare -r _reply
      COMPREPLY=("${_reply[@]}")
      ;;
    # TODO: multiple options (in 3) should have multiple completions
    3)
      # NOTE: same as in client impl
      [ -z "$global_arg_delim_2" ] && global_arg_delim_2="="

      # Disable space after completion
      compopt -o nospace

      case "$_prev" in
        gen)
          mapfile -t _reply < <(compgen -W "help all${global_arg_delim_2} type${global_arg_delim_2} profile${global_arg_delim_2} config${global_arg_delim_2} account${global_arg_delim_2} dev${global_arg_delim_2} confirm${global_arg_delim_2}" -- "$_cur")
          ;;
        edit)
          mapfile -t _reply < <(compgen -W "help type${global_arg_delim_2}" -- "$_cur")
          ;;
        build)
          mapfile -t _reply < <(compgen -W "help type${global_arg_delim_2}" -- "$_cur")
          ;;
        update)
          mapfile -t _reply < <(compgen -W "help type${global_arg_delim_2}" -- "$_cur")
          ;;
        backup | rm | up | down | start | stop | exec)
          # TODO: _currently no-op
          ;;
        shell)
          mapfile -t _reply < <(compgen -W "help user${global_arg_delim_2}" -- "$_cur")
          ;;
        version)
          mapfile -t _reply < <(compgen -W "help type${global_arg_delim_2}" -- "$_cur")
          ;;
        license)
          mapfile -t _reply < <(compgen -W "help file${global_arg_delim_2}" -- "$_cur")
          ;;
        linter)
          mapfile -t _reply < <(compgen -W "help type${global_arg_delim_2} file${global_arg_delim_2}" -- "$_cur")
          ;;
        doxygen)
          mapfile -t _reply < <(compgen -W "help gen${global_arg_delim_2} rm${global_arg_delim_2}" -- "$_cur")
          ;;
        plugins)
          # Re-enable space after completion (for plugin args)
          compopt +o nospace

          # TODO: HACK: since this is completion, lib_docker is never initialized, prior to completing
          # (environment variables are needed).
          if [ -z "$DOCKER_FINANCE_CLIENT_REPO" ]; then
            _aliases=~/.bash_aliases
            ! test -f "$_aliases" && _aliases=~/.bashrc

            declare -g DOCKER_FINANCE_CLIENT_REPO
            DOCKER_FINANCE_CLIENT_REPO="$(dirname "$(grep '^alias docker-finance=' $_aliases | cut -d'=' -f2 | sed -e "s:'::g" -e 's:"::g')")/../"
            source "${DOCKER_FINANCE_CLIENT_REPO}/client/src/docker/lib/lib_docker.bash" || echo $?

            local _instance
            declare -r _instance="$(echo $instance | sed "s:'::g")" # TODO: remove after hack for colon tag is resolved
            lib_docker::docker "$_instance" || lib_utils::print_fatal "$_instance not initialized"
          fi
          [ -z "$DOCKER_FINANCE_CLIENT_PLUGINS" ] && lib_utils::print_fatal "DOCKER_FINANCE_CLIENT_PLUGINS not set"

          local -r _args=("-type" "f" "-executable" "-not" "-path" "*/internal/*" "-not" "-path" "*/impl/*")
          local _plugins
          mapfile -t _plugins < <({
            find "${DOCKER_FINANCE_CLIENT_PLUGINS}"/client/docker "${_args[@]}" -printf 'custom/%P\n' 2>/dev/null
            find "${DOCKER_FINANCE_CLIENT_REPO}"/client/plugins/docker "${_args[@]}" -printf 'repo/%P\n' 2>/dev/null
          })
          declare -r _plugins
          mapfile -t _reply < <(compgen -W "help ${_plugins[*]}" -- "$_cur")
          ;;
      esac
      declare -r _reply
      COMPREPLY=("${_reply[@]}")
      ;;
    *)
      COMPREPLY=()
      ;;
  esac
}

complete -F docker-finance::completion docker.bash
complete -F docker-finance::completion docker-finance
complete -F docker-finance::completion dfi

# vim: sw=2 sts=2 si ai et
