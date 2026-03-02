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

#
# Generates default environment - required by all client and container libs
#

# Different platforms require slightly different environment
[ -z "$global_platform" ] && exit 1

# Version is internally set on-the-fly
[ -z "$global_client_version" ] && exit 1

export DOCKER_FINANCE_VERSION="$global_client_version"

# Developer related

if [[ -z "$DOCKER_FINANCE_DEBUG" || ! "$DOCKER_FINANCE_DEBUG" =~ ^0$|^1$|^2$ ]]; then
  export DOCKER_FINANCE_DEBUG=0
fi

# Allows transparent r/w of mounted volumes

if [ -z "$DOCKER_FINANCE_UID" ]; then
  DOCKER_FINANCE_UID="$(id -u)"
  export DOCKER_FINANCE_UID
fi

if [ -z "$DOCKER_FINANCE_GID" ]; then
  DOCKER_FINANCE_GID="$(id -g)"
  export DOCKER_FINANCE_GID
fi

if [ -z "$DOCKER_FINANCE_USER" ]; then
  DOCKER_FINANCE_USER="$(whoami)"
  export DOCKER_FINANCE_USER
fi

# Performance settings

if [ -z "$DOCKER_FINANCE_CPUS" ]; then
  DOCKER_FINANCE_CPUS="$(($(nproc --all) / 2))"
  export DOCKER_FINANCE_CPUS
fi

if [ -z "$DOCKER_FINANCE_MEMORY" ]; then
  DOCKER_FINANCE_MEMORY="$(($(grep MemTotal /proc/meminfo | rev | cut -d' ' -f2 | rev) / 3))k"
  export DOCKER_FINANCE_MEMORY
fi

# Client-specific environment, including bind mounts (client view)

if [ -z "$global_client_base_path" ]; then
  global_client_base_path="$(realpath -s "${BASH_SOURCE[0]}" | rev | cut -d'/' -f7- | rev)"
fi

if [ -z "$DOCKER_FINANCE_CLIENT_CONF" ]; then
  export DOCKER_FINANCE_CLIENT_CONF="${global_client_base_path}/conf.d"
fi

if [ -z "$DOCKER_FINANCE_CLIENT_REPO" ]; then
  export DOCKER_FINANCE_CLIENT_REPO="${global_client_base_path}/repo"
fi

if [[ -z "$DOCKER_FINANCE_CLIENT_PLUGINS" ]]; then
  export DOCKER_FINANCE_CLIENT_PLUGINS="${global_client_base_path}/plugins"
fi

if [[ "$global_platform" != "dev-tools" ]]; then
  if [[ -z "$DOCKER_FINANCE_CLIENT_FLOW" ]]; then
    export DOCKER_FINANCE_CLIENT_FLOW="${global_client_base_path}/flow"
  fi

  if [[ -z "$DOCKER_FINANCE_CLIENT_SHARED" ]]; then
    export DOCKER_FINANCE_CLIENT_SHARED="${global_client_base_path}/share.d"
  fi

  # hledger-web
  if [[ -z "$DOCKER_FINANCE_PORT_HLEDGER" ]]; then
    export DOCKER_FINANCE_PORT_HLEDGER="5000"
  fi

  # ROOT.cern
  if [[ -z "$DOCKER_FINANCE_PORT_ROOT" ]]; then
    export DOCKER_FINANCE_PORT_ROOT="8080"
  fi
fi

# Container-specific environment, including bind mounts (container view)

if [[ "$global_platform" != "dev-tools" ]]; then
  if [ -z "$DOCKER_FINANCE_CONTAINER_CONF" ]; then
    export DOCKER_FINANCE_CONTAINER_CONF="/home/${DOCKER_FINANCE_USER}/docker-finance/conf.d"
  fi

  if [ -z "$DOCKER_FINANCE_CONTAINER_REPO" ]; then
    export DOCKER_FINANCE_CONTAINER_REPO="/home/${DOCKER_FINANCE_USER}/docker-finance/repo"
  fi

  if [[ -z "$DOCKER_FINANCE_CONTAINER_FLOW" ]]; then
    export DOCKER_FINANCE_CONTAINER_FLOW="/home/${DOCKER_FINANCE_USER}/docker-finance/flow"
  fi

  if [[ -z "$DOCKER_FINANCE_CONTAINER_SHARED" ]]; then
    export DOCKER_FINANCE_CONTAINER_SHARED="/home/${DOCKER_FINANCE_USER}/docker-finance/share.d"
  fi

  if [[ -z "$DOCKER_FINANCE_CONTAINER_PLUGINS" ]]; then
    export DOCKER_FINANCE_CONTAINER_PLUGINS="/home/${DOCKER_FINANCE_USER}/docker-finance/plugins"
  fi

  if [[ -z "$DOCKER_FINANCE_CONTAINER_CMD" ]]; then
    export DOCKER_FINANCE_CONTAINER_CMD="finance.bash"
  fi

  if [[ -z "$DOCKER_FINANCE_CONTAINER_EDITOR" ]]; then
    export DOCKER_FINANCE_CONTAINER_EDITOR="vim"
  fi
fi

# vim: sw=2 sts=2 si ai et
