#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2024-2025 Aaron Fiore (Founder, Evergreen Crypto LLC)
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

# shellcheck disable=SC2154
[ "$DOCKER_FINANCE_DEBUG" == 2 ] && set -xv

[ -z "$DOCKER_FINANCE_CONTAINER_FLOW" ] && exit 1

function docker-finance::completion()
{
  local -r _cur="${COMP_WORDS[COMP_CWORD]}"
  local -r _prev="${COMP_WORDS[COMP_CWORD - 1]}"

  local _profiles
  mapfile -t _profiles < <(pushd "${DOCKER_FINANCE_CONTAINER_FLOW}"/profiles &>/dev/null && ls -d */*)
  declare -r _profiles

  local -r _commands=("all" "edit" "fetch" "hledger" "hledger-ui" "hledger-vui" "hledger-web" "import" "meta" "plugins" "reports" "root" "taxes" "times")

  local _reply

  case $COMP_CWORD in
    1)
      mapfile -t _reply < <(compgen -W "${_profiles[*]}" -- "$_cur")
      declare -r _reply
      COMPREPLY=("${_reply[@]}")
      ;;
    2)
      mapfile -t _reply < <(compgen -W "${_commands[*]}" -- "$_cur")
      declare -r _reply
      COMPREPLY=("${_reply[@]}")
      ;;
    3)
      # NOTE: same as in impl
      [ -z "$global_arg_delim_2" ] && global_arg_delim_2="="

      # Disable space after completion
      compopt -o nospace

      case "$_prev" in
        all)
          mapfile -t _reply < <(compgen -W "help year${global_arg_delim_2}" -- "$_cur")
          ;;
        edit)
          mapfile -t _reply < <(compgen -W "help type${global_arg_delim_2} account${global_arg_delim_2}" -- "$_cur")
          ;;
        fetch)
          mapfile -t _reply < <(compgen -W "help all${global_arg_delim_2} price${global_arg_delim_2} api${global_arg_delim_2} account${global_arg_delim_2} year${global_arg_delim_2} chain${global_arg_delim_2} blockchain${global_arg_delim_2}" -- "$_cur")
          ;;
        import)
          mapfile -t _reply < <(compgen -W "help year${global_arg_delim_2}" -- "$_cur")
          ;;
        hledger)
          #
          # Commands (as described in v1.40)
          #
          local _hledger=()

          # Help
          _hledger+=("-h" "help" "demo" "--tldr" "--info")

          # "User Interfaces"
          # NOTE: `ui` and `web` are to be called outside of this scope (via the `dfi` interface)
          _hledger+=("repl" "run")

          # "Entering Data"
          # NOTE: no commands here as they should be called outside of this scope (via the `dfi` interface)

          # "Basic"
          _hledger+=("accounts" "codes" "commodities" "descriptions" "files" "notes" "payees" "prices" "stats" "tags")

          # "Standard"
          _hledger+=("print" "areg" "reg" "bs" "bse" "cf" "is")

          # "Advanced"
          _hledger+=("bal" "roi")

          # "Charts"
          _hledger+=("activity")

          # "Generating"
          _hledger+=("close" "rewrite")

          # "Maintenance"
          _hledger+=("check" "diff" "test")

          mapfile -t _reply < <(compgen -W "${_hledger[*]}" -- "$_cur")
          ;;
        hledger-ui)
          mapfile -t _reply < <(compgen -W "-h" -- "$_cur")
          ;;
        hledger-vui)
          mapfile -t _reply < <(compgen -W "-h" -- "$_cur")
          ;;
        hledger-web)
          mapfile -t _reply < <(compgen -W "-h" -- "$_cur")
          ;;
        meta)
          # NOTE: args are dependent upon metadata contents
          mapfile -t _reply < <(compgen -W "help" -- "$_cur")
          ;;
        plugins)
          # Re-enable space after completion (for plugin args)
          compopt +o nospace

          [ -z "$DOCKER_FINANCE_CONTAINER_REPO" ] && lib_utils::die_fatal
          [ -z "$DOCKER_FINANCE_CONTAINER_PLUGINS" ] && lib_utils::die_fatal

          local -r _args=("-type" "f" "-executable" "-not" "-path" "*/internal/*" "-not" "-path" "*/impl/*")
          local _plugins
          mapfile -t _plugins < <({
            find "${DOCKER_FINANCE_CONTAINER_PLUGINS}"/finance "${_args[@]}" -printf 'custom/%P\n' 2>/dev/null
            find "${DOCKER_FINANCE_CONTAINER_REPO}"/plugins/finance "${_args[@]}" -printf 'repo/%P\n' 2>/dev/null
          })
          declare -r _plugins
          mapfile -t _reply < <(compgen -W "help ${_plugins[*]}" -- "$_cur")
          ;;
        reports)
          mapfile -t _reply < <(compgen -W "help all${global_arg_delim_2} type${global_arg_delim_2} interval${global_arg_delim_2} year${global_arg_delim_2}" -- "$_cur")
          ;;
        root)
          compopt +o nospace

          [ -z "$DOCKER_FINANCE_CONTAINER_REPO" ] && lib_utils::die_fatal
          [ -z "$DOCKER_FINANCE_CONTAINER_PLUGINS" ] && lib_utils::die_fatal

          local -r _args=("-type" "f" "-not" "-path" "*/internal/*" "-not" "-path" "*/impl/*" "-not" "-path" "*/common/*" "!" "-name" "rootlogon.C")
          local _root
          mapfile -t _root < <({
            find "${DOCKER_FINANCE_CONTAINER_PLUGINS}"/root "${_args[@]}" -printf 'plugins/custom/%P\n' 2>/dev/null
            find "${DOCKER_FINANCE_CONTAINER_REPO}"/plugins/root "${_args[@]}" -printf 'plugins/repo/%P\n' 2>/dev/null
            find "${DOCKER_FINANCE_CONTAINER_REPO}"/src/root/macro "${_args[@]}" -printf 'macros/repo/%P\n' 2>/dev/null
          })
          declare -r _root
          mapfile -t _reply < <(compgen -W "help ${_root[*]}" -- "$_cur")
          ;;
        taxes)
          mapfile -t _reply < <(compgen -W "help all${global_arg_delim_2} tag${global_arg_delim_2} account${global_arg_delim_2} year${global_arg_delim_2} write${global_arg_delim_2}" -- "$_cur")
          ;;
        times)
          mapfile -t _reply < <(compgen -W "help" -- "$_cur")
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

complete -F docker-finance::completion finance
complete -F docker-finance::completion finance.bash
complete -F docker-finance::completion docker-finance
complete -F docker-finance::completion dfi

# vim: sw=2 sts=2 si ai et
