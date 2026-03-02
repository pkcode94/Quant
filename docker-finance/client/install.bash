#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2024 Aaron Fiore (Founder, Evergreen Crypto LLC)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation  either version 3 of the License  or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not  see <https://www.gnu.org/licenses/>.

bashrc=~/.bashrc
aliases=~/.bash_aliases

function docker-finance::install()
{
  # Environment expectations
  local -r _alias=("docker-finance" "dfi")
  local _path # full path to docker-finance repository
  _path="$(dirname "$(realpath -s $0)" | rev | cut -d'/' -f2- | rev)"

  # Install (or re-install) aliases
  # NOTE: caller must `source` (as `unalias` and `alias` are in subshell)
  for _a in "${_alias[@]}"; do
    if grep "^alias ${_a}=" "$aliases" &>/dev/null; then
      sed -i "/^alias ${_a}=/d" "$aliases"
    fi
    echo "alias ${_a}='${_path}/client/docker.bash'" >>"$aliases"
  done

  # Set bash completion
  _completion="${_path}/client/src/docker/completion.bash"
  if [ ! -f "$_completion" ]; then
    echo "WARNING: bash completion not found" >&2
  else
    if ! grep "^source '${_completion}'" "$aliases" &>/dev/null; then
      echo "source '${_completion}'" >>"$aliases"
    fi
  fi
}

if ! test -f "$bashrc" || ! hash bash &>/dev/null; then
  echo "FATAL: unsupported bash installation" >&2
else
  if [[ ! -z "$SHELL" && "$SHELL" =~ bash ]]; then
    if [ ! -f "$aliases" ]; then
      if ! grep -E "(\. ~/.bash_aliases|^source ~/.bash_aliases|^source ${aliases})" "$bashrc" &>/dev/null; then
        aliases="$bashrc"
      fi
    fi
    docker-finance::install \
      && echo "SUCCESS: installation complete" \
      || echo "FATAL: could not complete installation"
  else
    echo "FATAL: unsupported bash environment" >&2
  fi
fi

# vim: sw=2 sts=2 si ai et
