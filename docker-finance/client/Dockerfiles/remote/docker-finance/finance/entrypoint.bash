#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2021-2024 Aaron Fiore (Founder, Evergreen Crypto LLC)
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
# *** WARNING: REQUIRES GENERATED CLIENT ENVIRONMENT ***
#

if [ -z "$DOCKER_FINANCE_CONTAINER_REPO" ]; then
  echo "FATAL: DOCKER_FINANCE_CONTAINER_REPO not set" >&2
  exit 1
fi

if [ -z "$DOCKER_FINANCE_CONTAINER_FLOW" ]; then
  echo "FATAL: DOCKER_FINANCE_CONTAINER_FLOW not set" >&2
  exit 1
fi

# Dynamically connect hledger-flow source with end-user hledger-flow structure
# TODO: annoying, find a better way
ln -f -s "${DOCKER_FINANCE_CONTAINER_REPO}/src/hledger-flow" "${DOCKER_FINANCE_CONTAINER_FLOW}/src"

# Keep container running
tail -f /dev/null

exec "$@"
