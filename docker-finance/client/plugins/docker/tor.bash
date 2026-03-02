#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2024,2026 Aaron Fiore (Founder, Evergreen Crypto LLC)
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
# "Libraries"
#

[ -z "$DOCKER_FINANCE_CLIENT_REPO" ] && exit 1
source "${DOCKER_FINANCE_CLIENT_REPO}/client/src/docker/lib/lib_docker.bash"

[[ -z "$global_platform" || -z "$global_arg_delim_1" || -z "$global_user" || -z "$global_tag" ]] && lib_utils::die_fatal
instance="${global_platform}${global_arg_delim_1}${global_user}:${global_tag}"

# Initialize "constructor"
# NOTE: "constructor" only needed if calling library directly
lib_docker::docker "$instance" || lib_utils::die_fatal

#
# Implementation
#

declare -g tor_container="docker-finance_tor"

# TODO: current implementation will clobber any other proxychains usage (i.e., other SOCKS proxy or overlay network (I2P, etc.))
function tor::start()
{
  [[ -z "$global_container" || -z "$global_network" ]] && lib_utils::die_fatal

  # NOTE: proxychains.conf's [ProxyList] won't allow hostnames (or Docker network container name).
  # So, to avoid conflicting IP address spaces, docker-finance will not hardcode address space.
  # Ergo, an already-running container will be needed (sorry, lib_docker::run())
  lib_docker::exec "" \
    || lib_utils::die_fatal "docker-finance not running! Bring \`up\` a \`dfi\` instance and try again."

  local -r _torrc="/etc/tor/torrc"

  if [[ $(docker container inspect -f '{{.State.Running}}' "$tor_container" 2>/dev/null) == "true" ]]; then
    lib_utils::print_error "${tor_container}: instance already running (consider \`restart\`)"
    return 1
  else
    docker pull alpine:latest || lib_utils::die_fatal
    docker run -it --rm --detach \
      --network "$global_network" \
      --name="${tor_container}" \
      --entrypoint=/bin/sh alpine:latest -c "
          apk update \
            && apk add tor \
            && cp ${_torrc}.sample ${_torrc} \
            && su -s /usr/bin/tor tor" 1>/dev/null || lib_utils::die_fatal
  fi && lib_utils::print_info "${tor_container}: container started"

  # Get Tor container's local IP
  local _ip
  _ip="$(docker inspect -f '{{range.NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $tor_container)"
  lib_utils::print_info "${tor_container}: container IP '${_ip}'"

  # Need to wait for a working installation
  lib_utils::print_info "${tor_container}: waiting for Tor installation"
  while ! docker exec "$tor_container" /bin/sh -c 'apk info -e tor' 1>/dev/null; do
    sleep 1s
  done && lib_utils::print_info "${tor_container}: Tor installation ready"

  # Update torrc with container's local IP (to avoid 0.0.0.0)
  lib_utils::print_info "${tor_container}: updating $_torrc"
  docker exec "$tor_container" \
    /bin/sh -c "
      sed -i '/^SOCKSPort /d' ${_torrc} \
      && echo 'SOCKSPort ${_ip}:9050' >>${_torrc}" || lib_utils::die_fatal

  # Have Tor pickup changes
  lib_utils::print_info "${tor_container}: restarting Tor with updated ${_torrc}"
  docker exec "$tor_container" /bin/sh -c "pkill -HUP tor" || lib_utils::die_fatal

  # Set `dfi`'s proxychains instance to point to Tor instance
  local -r _proxychains="/etc/proxychains.conf"
  lib_utils::print_info "${global_container}: updating $_proxychains"
  docker exec --user root "$global_container" \
    /bin/bash -c "
      sed -i \
        -e 's:^#quiet_mode:quiet_mode:' \
        -e 's:^# localnet 127.0.0.0/255.0.0.0:localnet 127.0.0.0/255.0.0.0:' \
        -e '/^socks/d' $_proxychains \
        && echo 'socks5  $_ip 9050' >>$_proxychains" || lib_utils::die_fatal

  # Test Tor connection
  local -r _sleep="20s"
  lib_utils::print_info "${global_container}: testing connection (bootstrapping ~${_sleep})"
  sleep "$_sleep" # Give time to bootstrap

  local _tries=1
  while [ $_tries -ne 3 ]; do
    lib_docker::exec "proxychains curl -s https://check.torproject.org 2>/dev/null \
        | grep -B3 'Your IP address appears to be' \
        | sed -e 's/^      //g' -e '\$ s/[^\\.0-9]//g' -e '/^\$/d' -e '2,3d' \
        | grep -A2 --color=never Congratulations || exit 1 2>/dev/null" 2>/dev/null
    if [ $? -ne 0 ]; then
      lib_utils::print_warning "Could not bootstrap, trying again (${_tries}/3)"
      docker exec "$tor_container" /bin/sh -c "pkill -HUP tor" || lib_utils::die_fatal
      sleep "$_sleep"
      ((_tries++))
    else
      break
    fi
  done
  if [ $_tries -eq 3 ]; then
    lib_utils::die_fatal "Could not successfully bootstrap! \`restart\` this instance"
  fi
}

function tor::stop()
{
  if ! docker container inspect -f '{{.State.Running}}' "$tor_container" &>/dev/null; then
    lib_utils::print_error "${tor_container}: container not running"
    return 1
  fi
  lib_utils::print_info "${tor_container}: stopping container"
  docker container stop -t 3 "$tor_container" &>/dev/null \
    && lib_utils::print_info "${tor_container}: container stopped"
}

function tor::restart()
{
  tor::stop
  tor::start
}

function main()
{
  [ -z "$global_basename" ] && lib_utils::die_fatal

  local -r _usage="
\e[32mDescription:\e[0m

  Connect docker-finance container's \`fetch\` to The Onion Router.

\e[32mUsage:\e[0m

  \e[37;2m# Client (host) command\e[0m
  $ $global_basename $instance plugins repo${global_arg_delim_1}$(basename $0) <{start|up}|restart|{stop|down}>

  \e[37;2m# Container command (see \`fetch\` help for usage)\e[0m
  $ $global_basename <profile${global_arg_delim_1}subprofile> fetch help
"

  case "$1" in
    start | up)
      tor::start
      ;;
    stop | down)
      tor::stop
      ;;
    restart)
      tor::restart
      ;;
    *)
      lib_utils::die_usage "$_usage"
      ;;
  esac
}

main "$@"

# vim: sw=2 sts=2 si ai et
