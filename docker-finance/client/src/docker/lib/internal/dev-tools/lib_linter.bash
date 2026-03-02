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

source "${DOCKER_FINANCE_CLIENT_REPO}/client/src/docker/lib/internal/lib_docker.bash" || exit 1
source "${DOCKER_FINANCE_CLIENT_REPO}/container/src/finance/lib/internal/lib_utils.bash" || exit 1

#
# Implementation
#

function lib_linter::__parse_args()
{
  [ -z "$global_usage" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_2" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_3" ] && lib_utils::die_fatal

  local -r _usage="
\e[32mDescription:\e[0m

  Lint docker-finance source files

\e[32mUsage:\e[0m

  $ $global_usage type${global_arg_delim_2}<type> [file${global_arg_delim_2}<file>]

\e[32mArguments:\e[0m

  File type:

    type${global_arg_delim_2}<bash|php|c++>

  File path:

    file${global_arg_delim_2}<path to file>

\e[32mNotes:\e[0m

  Cannot use multiple types with file argument present

\e[32mExamples:\e[0m

  \e[37;2m# Lint only PHP files\e[0m
  $ $global_usage type${global_arg_delim_2}php

  \e[37;2m# Lint *all* development files\e[0m
  $ $global_usage type${global_arg_delim_2}bash${global_arg_delim_3}php${global_arg_delim_3}c++

  \e[37;2m# Lint given C++ files\e[0m
  $ $global_usage type${global_arg_delim_2}c++ file${global_arg_delim_2}path/file_1.hh${global_arg_delim_3}path/file_2.hh
"

  #
  # Ensure supported arguments
  #

  [ $# -eq 0 ] && lib_utils::die_usage "$_usage"

  for _arg in "$@"; do
    [[ ! "$_arg" =~ ^type[s]?${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^file[s]?${global_arg_delim_2} ]] \
      && lib_utils::die_usage "$_usage"
  done

  #
  # Parse arguments before testing
  #

  # Parse key for value
  for _arg in "$@"; do
    local _key="${_arg%${global_arg_delim_2}*}"
    local _len="$((${#_key} + 1))"

    if [[ "$_key" =~ ^type[s]?$ ]]; then
      local _arg_type="${_arg:${_len}}"
      [ -z "$_arg_type" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^file[s]?$ ]]; then
      local _arg_file="${_arg:${_len}}"
      [ -z "$_arg_file" ] && lib_utils::die_usage "$_usage"
    fi
  done

  #
  # Test argument values, set globals
  #

  IFS="$global_arg_delim_3"

  # Arg: type
  if [ ! -z "$_arg_type" ]; then
    read -ra _read <<<"$_arg_type"

    for _type in "${_read[@]}"; do
      if [[ ! "$_type" =~ ^bash$|^shell$|^c\+\+$|^cpp$|^php$ ]]; then
        lib_utils::die_usage "$_usage"
      fi
    done

    declare -gr global_arg_type=("${_read[@]}")
  fi

  # Arg: file
  if [ ! -z "$_arg_file" ]; then
    read -ra _read <<<"$_arg_file"

    if [ "${#global_arg_type[@]}" -gt 1 ]; then
      # Multiple types not supported if file is given
      lib_utils::die_usage "$_usage"
    fi

    declare -gr global_arg_file=("${_read[@]}")
  fi
}

function lib_linter::__lint_bash()
{
  local -r _path=("$@")
  local -r _exts=("bash" "bash.in")

  local -r _shfmt="shfmt -ln bash -i 2 -ci -bn -fn -w"
  local -r _shellcheck="shellcheck --color=always --norc --enable=all --shell=bash --severity=warning"

  if [ -z "${_path[*]}" ]; then
    # Do all
    for _ext in "${_exts[@]}"; do
      lib_docker::exec \
        "find ${DOCKER_FINANCE_CLIENT_REPO}/ ${DOCKER_FINANCE_CLIENT_PLUGINS}/ -type f -name \*.${_ext} \
        | while read _file
          do echo Linting \'\${_file}\'
            $_shfmt \$_file \$_file && $_shellcheck \${_file}
          done"
    done
  else
    # Do only given file(s)
    for _p in "${_path[@]}"; do
      local _file
      _file="$(dirs +1)/${_p}"
      lib_utils::print_normal "Linting '${_file}'"
      lib_docker::exec \
        "$_shfmt $_file $_file && $_shellcheck $_file"
    done
  fi
}

function lib_linter::__lint_cpp()
{
  local -r _path=("$@")
  local -r _exts=("hh" "cc" "cpp" "C")

  local _clang_file
  _clang_file="$(find ${DOCKER_FINANCE_CLIENT_REPO}/ -name .clang-format)"
  [ -z "$_clang_file" ] && lib_utils::die_fatal ".clang-format not found"
  declare -r _clang_file

  local -r _clang_format="clang-format -i --style=file:${_clang_file}"
  local -r _cpplint="cpplint --root=${DOCKER_FINANCE_CLIENT_REPO} --filter=-whitespace/braces,-whitespace/newline,-whitespace/line_length,-build/c++11 --headers=hh --extensions=hh,cc,cpp,C"
  local -r _cppcheck="cppcheck --enable=warning,style,performance,portability --inline-suppr --std=c++20"

  if [ -z "${_path[*]}" ]; then
    # Do all
    for _ext in "${_exts[@]}"; do
      lib_docker::exec \
        "find ${DOCKER_FINANCE_CLIENT_REPO}/ ${DOCKER_FINANCE_CLIENT_PLUGINS}/ -type f -name \*.${_ext} \
        | while read _file
          do echo Linting \'\${_file}\'
            $_clang_format \$_file && $_cpplint \$_file && $_cppcheck \$_file
          done"
    done
  else
    # Do only given file(s)
    for _p in "${_path[@]}"; do
      local _file
      _file="$(dirs +1)/${_p}"
      lib_utils::print_normal "Linting '${_file}'"
      lib_docker::exec \
        "$_clang_format $_file && $_cpplint $_file && $_cppcheck $_file"
    done
  fi
}

function lib_linter::__lint_php()
{
  local -r _path=("$@")
  local -r _ext="php"

  local -r _php_cs_fixer_init="php-cs-fixer -n init"
  local -r _php_cs_fixer_fix="time php-cs-fixer fix --rules=@PSR12 --verbose"

  local -r _phpstan="time phpstan analyse --debug --autoload-file /usr/local/lib/php/vendor/autoload.php --level 6" # TODO: incrementally increase to 9

  if [ -z "${_path[*]}" ]; then
    # Do all
    lib_docker::exec \
      "$_php_cs_fixer_init \
      && $_php_cs_fixer_fix $DOCKER_FINANCE_CLIENT_REPO \
      && $_php_cs_fixer_fix $DOCKER_FINANCE_CLIENT_PLUGINS \
      && $_phpstan $DOCKER_FINANCE_CLIENT_REPO $DOCKER_FINANCE_CLIENT_PLUGINS"
  else
    # Do only given file(s)
    for _p in "${_path[@]}"; do
      local _file
      _file="$(dirs +1)/${_p}"
      lib_utils::print_normal "Linting '${_file}'"
      lib_docker::exec \
        "$_php_cs_fixer_init \
        && $_php_cs_fixer_fix $_file \
        && $_phpstan $_file"
    done
  fi
}

function lib_linter::__linter()
{
  [[ -z "$DOCKER_FINANCE_CLIENT_REPO" || -z "$DOCKER_FINANCE_CLIENT_PLUGINS" ]] && lib_utils::die_fatal

  [[ ! "$PWD" =~ ^$DOCKER_FINANCE_CLIENT_REPO && ! -z "${global_arg_file[*]}" ]] \
    && lib_utils::die_fatal "Sorry, you must work (and lint) from within parent directory '${DOCKER_FINANCE_CLIENT_REPO}'"

  lib_utils::print_debug "Bringing up network and container"

  [ -z "$global_network" ] && lib_utils::die_fatal
  local -r _no_funny_business="--internal=true"
  docker network create "$_no_funny_business" "$global_network" 2>/dev/null
  lib_docker::__docker_compose up -d docker-finance || lib_utils::die_fatal

  # Transparent bind-mount: although it's the container repo, its path is identical to client repo
  pushd "$DOCKER_FINANCE_CLIENT_REPO" 1>/dev/null || lib_utils::die_fatal

  for _type in "${global_arg_type[@]}"; do
    case "$_type" in
      bash | shell)
        lib_linter::__lint_bash "${global_arg_file[@]}"
        ;;
      c++ | cpp)
        lib_linter::__lint_cpp "${global_arg_file[@]}"
        ;;
      php)
        lib_linter::__lint_php "${global_arg_file[@]}"
        ;;
    esac
    lib_utils::print_custom "\n"
  done

  popd 1>/dev/null || lib_utils::die_fatal

  lib_docker::__docker_compose down
  local -r _ret=$?

  docker network rm "$global_network" 2>/dev/null # Don't force, if in use
  return $_ret
}

function lib_linter::linter()
{
  lib_linter::__parse_args "$@"
  lib_linter::__linter && lib_utils::print_info "Linting completed successfully!"
}

# vim: sw=2 sts=2 si ai et
