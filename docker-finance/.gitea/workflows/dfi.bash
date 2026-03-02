#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2026 Aaron Fiore (Founder, Evergreen Crypto LLC)
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

set -e

hash vim
declare -xr EDITOR=vim

declare -r ci_shell=("bash" "-i" "-c")

# Defaults to run (most) commands with
declare -r ci_archlinux="dfi archlinux/${USER}:default"
declare -r ci_dev_tools="dfi dev-tools/${USER}:default"
declare -r ci_profile="dfi testprofile/${USER}"

##
## Host (act runner)
##

function host::clean()
{
  # Remove previous containers from any failed builds
  docker rm -f "$(docker ps -aqf "name=^docker-finance")" 2>/dev/null \
    && echo "WARNING: previous docker-finance containers found and removed" \
    || echo "No docker-finance containers found"

  # Remove all images not in use
  local -r _images=("$(docker images -a --format=table | grep docker-finance | gawk '{print $3}' | sort -u)")
  while read _line; do
    docker image rm -f "$_line" || continue
  done < <(echo "${_images[@]}")

  # Remove all leftovers
  docker system prune -f
  docker builder prune -af

  # Remove default environment and layout
  local _paths
  _paths+=(".bashrc")
  _paths+=(".docker")
  _paths+=("docker-finance")
  for _path in "${_paths[@]}"; do
    rm -fr "${HOME:?}/${_path:?}"
  done

  # Prepare for installation
  # *MUST* exist prior to client::install
  touch "${HOME:?}"/.bashrc
}

##
## Client (host)
##

#
# client::install
#

function client::install()
{
  "${HOME:?}"/docker-finance/repo/client/install.bash
  source "${HOME:?}"/.bashrc
}

#
# client::finance
#

function client::finance::gen()
{
  local -r _tags=("micro" "tiny" "slim" "default")
  local -r _types=("env" "build" "superscript" "env,build,superscript")

  for _tag in "${_tags[@]}"; do
    for _type in "${_types[@]}"; do
      local _gen="dfi archlinux/${USER}:${_tag} gen"

      # Invalid
      "${ci_shell[@]}" "$_gen type=${_type}INVALID || exit 0"
      "${ci_shell[@]}" "$_gen INVALID=${_type} || exit 0"

      # Valid
      "${ci_shell[@]}" "$_gen type=${_type} confirm=no"
    done
  done

  # For CI purposes, only generate flow for 'default'
  "${ci_shell[@]}" "$ci_archlinux gen type=flow profile=testprofile/${USER} confirm=no dev=on"
}

function client::finance::edit()
{
  local -r tags=("micro" "tiny" "slim" "default")

  local types=()
  types+=("env")
  types+=("shell" "superscript")
  types+=("build" "dockerfile")
  types+=("env,shell,build")
  types+=("env,superscript,dockerfile")
  declare -r types

  for _tag in "${tags[@]}"; do
    for _type in "${types[@]}"; do
      local _edit="dfi archlinux/${USER}:${_tag} edit"

      # Invalid
      "${ci_shell[@]}" "$_edit type=${_type}INVALID || exit 0"
      "${ci_shell[@]}" "$_edit INVALID=${_type} || exit 0"

      # Valid
      "${ci_shell[@]}" "$_edit type=${_type} & wait ; kill -9 %1"
    done
    # Build: uncomment all optional packages and plugin dependencies
    if [[ "$_tag" == "default" ]]; then
      local _file
      _file="${HOME:?}/docker-finance/conf.d/client/$(uname -s)-$(uname -m)/archlinux/default/Dockerfiles/${USER:?}@$(uname -n)"
      [ ! -f "$_file" ] && exit 1
      sed -i '18,56s/#//' "$_file"
    fi
  done
}

function client::finance::build()
{
  local -r _types=("micro" "tiny" "slim" "default")

  for _type in "${_types[@]}"; do
    local _build="dfi archlinux/${USER}:${_type} build"

    # Invalid
    "${ci_shell[@]}" "$_build type=${_type}INVALID || exit 0"
    "${ci_shell[@]}" "$_build INVALID=${_type} || exit 0"

    # Valid
    "${ci_shell[@]}" "$_build type=${_type}"
  done
}

function client::finance::backup()
{
  local -r _backup=("micro" "tiny" "slim" "default")

  for _type in "${_backup[@]}"; do
    "${ci_shell[@]}" "dfi archlinux/${USER}:${_type} backup"
    sleep 1s
  done
}

function client::finance::up()
{
  # NOTE: the container is detached but what follows is
  #   an interactive terminal (so fork this).
  local -r _detach="&"
  "${ci_shell[@]}" "$ci_archlinux up $_detach wait"
}

function client::finance::stop()
{
  "${ci_shell[@]}" "$ci_archlinux stop"
}

function client::finance::start()
{
  "${ci_shell[@]}" "$ci_archlinux start"
}

function client::finance::down()
{
  "${ci_shell[@]}" "$ci_archlinux down"
}

function client::finance::shell()
{
  client::finance::up
  local -r _shell="$ci_archlinux shell"
  local -r _args=(" " "user=${USER}" "user=root")

  for _arg in "${_args[@]}"; do
    # Invalid
    "${ci_shell[@]}" "$_shell INVALID || exit 0"
    "${ci_shell[@]}" "$_shell user=INVALID || exit 0"

    # Valid
    "${ci_shell[@]}" "$_shell $_arg & wait ; kill -9 %1"
  done
  client::finance::down
}

function client::finance::exec()
{
  client::finance::up
  "${ci_shell[@]}" "$ci_archlinux exec 'exit 1' || exit 0"
  "${ci_shell[@]}" "$ci_archlinux exec 'exit 0'"
  client::finance::down
}

function client::finance::plugins::__tor()
{
  [ -z "$_plugins" ] && exit 1

  client::finance::up
  local -r _tor=("start" "restart" "stop")
  for _arg in "${_tor[@]}"; do
    "${ci_shell[@]}" "$_plugins repo/tor.bash $_arg"
  done
  client::finance::down
}

function client::finance::plugins::__bitcoin()
{
  [ -z "$_plugins" ] && exit 1

  client::finance::up
  local _bitcoin
  _bitcoin+=("get" "build")
  # TODO: build is needed for container's bitcoin plugin)
  # _bitcoin+=("clean")

  for _arg in "${_bitcoin[@]}"; do
    "${ci_shell[@]}" "$_plugins repo/bitcoin.bash $_arg"
  done
  client::finance::down
}

function client::finance::plugins()
{
  local -r _plugins="$ci_archlinux plugins"

  # Invalid
  "${ci_shell[@]}" "$_plugins INVALID || exit 0"
  "${ci_shell[@]}" "$_plugins repo/INVALID || exit 0"
  "${ci_shell[@]}" "$_plugins custom/INVALID || exit 0"

  # Valid
  "${ci_shell[@]}" "$_plugins repo/example.bash"
  client::finance::plugins::__tor
  client::finance::plugins::__bitcoin
}

function client::finance::run()
{
  # Invalid
  "${ci_shell[@]}" "$ci_archlinux run 'hash INVALID' || exit 0"

  # Valid
  "${ci_shell[@]}" "$ci_archlinux run 'hash bash'"
}

function client::finance::version()
{
  local -r _tags=("micro" "tiny" "slim" "default")

  local _types
  _types+=("client" "container" "short" "all")
  _types+=("client,container,short")
  declare -r _types

  for _tag in "${_tags[@]}"; do
    for _type in "${_types[@]}"; do
      local _version="dfi archlinux/${USER}:${_tag} version"

      # Invalid
      "${ci_shell[@]}" "$_version type=${_type}INVALID || exit 0"
      "${ci_shell[@]}" "$_version INVALID=${_type} || exit 0"

      # Valid
      "${ci_shell[@]}" "$_version type=${_type}"
    done
  done
}

function client::finance::update()
{
  local -r _types=("micro" "tiny" "slim" "default")

  for _type in "${_types[@]}"; do
    local _update="dfi archlinux/${USER}:${_type} update"

    # Invalid
    "${ci_shell[@]}" "$_update type=${_type}INVALID || exit 0"
    "${ci_shell[@]}" "$_update INVALID=${_type} || exit 0"

    # Valid
    "${ci_shell[@]}" "$_update type=${_type}"
  done
}

function client::finance::rm()
{
  local -r _build=("micro" "tiny" "slim" "default")

  for _type in "${_build[@]}"; do
    local _image="dfi archlinux/${USER}:${_type}"

    "${ci_shell[@]}" "$_image rm"
    docker images -q "$_image" && return $?
    # NOTE: don't attempt `dfi` container commands or else image will try to pull/re-build
  done

  # Re-build (from cache) for future test cases
  client::finance::build
}

##
## Container (finance)
##

function container::finance::edit()
{
  client::finance::up
  local -r _exec="$ci_archlinux exec"
  local -r _edit="$ci_profile edit"
  local -r _types=("add" "iadd" "manual" "fetch" "hledger" "meta" "shell" "subscript" "rules" "preprocess")

  # Invalid
  for _type in "${_types[@]}"; do
    "${ci_shell[@]}" "$_exec '$_edit type=${_type}INVALID' || exit 0"
    "${ci_shell[@]}" "$_exec '$_edit INVALID=${_type}' || exit 0"

    if [[ "$_type" =~ ^manual$ ]]; then
      "${ci_shell[@]}" "$_exec '$_edit type=${_type} year=abc123' || exit 0"
    fi
  done

  # Valid
  for _type in "${_types[@]}"; do
    if [[ "$_type" =~ ^rules$|^preprocess$ ]]; then

      # Valid type w/ valid account
      "${ci_shell[@]}" "$_exec '$_edit type=${_type} account=coinbase & wait ; kill -9 %1'"
      "${ci_shell[@]}" "$_exec '$_edit type=${_type} account=coinbase/platform & wait ; kill -9 %1'"
      "${ci_shell[@]}" "$_exec '$_edit type=${_type} account=coinbase/platform,electrum/wallet-1 & wait ; kill -9 %1'"

      # Valid type w/ *invalid* account
      "${ci_shell[@]}" "$_exec '$_edit type=${_type} account=coinbase/does-not-exist' || exit 0"
      "${ci_shell[@]}" "$_exec '$_edit type=${_type} account=does-not-exist/here-either' || exit 0"
      "${ci_shell[@]}" "$_exec '$_edit type=${_type} account=does-not-exist/here-either,nor/here' || exit 0"

    elif [[ "$_type" =~ ^manual$ ]]; then
      "${ci_shell[@]}" "$_exec '$_edit type=${_type} year=$(date +%Y) & wait ; kill -9 %1'"
    else
      "${ci_shell[@]}" "$_exec '$_edit type=${_type} & wait ; kill -9 %1'"
    fi
  done
  "${ci_shell[@]}" "$_exec '$_edit type=add,iadd,manual,fetch,hledger,meta,shell,subscript & wait ; kill -9 %1'"
  "${ci_shell[@]}" "$_exec '$_edit type=rules,preprocess account=coinbase/platform,electrum/wallet-1 & wait ; kill -9 %1'"

  #
  # Fetch: add API secrets for CI
  #

  [[ -z "$CI_DFI_FETCH_MOBULA" || -z "$CI_DFI_FETCH_ETHERSCAN" ]] && exit 1

  local -r _file="${HOME}/docker-finance/flow/profiles/testprofile/${USER}/conf.d/fetch/fetch.yaml"
  [ ! -f "$_file" ] && exit 1

  # `prices`
  sed -i "77s/.*/        key: \"${CI_DFI_FETCH_MOBULA}\"/" "$_file"
  [ ! -z "$CI_DFI_FETCH_COINGECKO" ] \
    && sed -i "118s/.*/        key: \"${CI_DFI_FETCH_COINGECKO}\"/" "$_file" \
    || echo "[INFO] CI_DFI_FETCH_COINGECKO is empty (using free plan)"

  # `accounts`
  #
  #   NOTE: For CI, a single etherscan API key is used per account
  #   (but the flexibility remains to use multiple keys if needed)
  #
  sed -i "s:etherscan/XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:etherscan/${CI_DFI_FETCH_ETHERSCAN}:g" "$_file"
  client::finance::down
}

function container::finance::fetch()
{
  client::finance::up
  local -r _exec="$ci_archlinux exec"
  local -r _fetch="$ci_profile fetch"

  #
  # `price`
  #

  # Invalid
  "${ci_shell[@]}" "$_exec '$_fetch price=INVALID api=mobula,coingecko' || exit 0"
  "${ci_shell[@]}" "$_exec '$_fetch price=bitcoin/BTC,ethereum/ETH api=INVALID' || exit 0"

  # Valid
  "${ci_shell[@]}" "$_exec '$_fetch all=price'"
  "${ci_shell[@]}" "$_exec '$_fetch price=bitcoin/BTC,ethereum/ETH api=mobula,coingecko'"
  "${ci_shell[@]}" "$_exec '$_fetch price=algorand/ALGO,tezos/XTZ api=mobula year=2024'"

  #
  # `account`
  #

  # Test given wallet/chain arguments with test wallets picked at random (mockups)
  # NOTE: these are also all caught with `all=account` or `all=all`
  local -r _algo_wallet="algorand/phone:mobile-1/55YXQ2AC7PUOOYIWUFIOGFZ7M5CBWFUDOIT7L3FMZVE7HGC3IKABL7HVOE"
  local -r _tezos_wallet="tezos/nano:x-1:staking-1/tz1S1ayWDiHzmL6zFNnY1ivvUkEgDcH88cjx"

  local -r _ethereum_wallet="ethereum/laptop:wallet-1/0x6546d43EA6DE45EB7298A2074e239D5573cA02F3"
  local -r _polygon_wallet="polygon/laptop:wallet-2/0xEad0B2b6f6ab84d527569835cd7fe364e067cFFf"

  local -r _arbitrum_wallet="arbitrum/phone:wallet-2/0xd3b29C94a67Cfa949FeD7dd1474B71d006fa0A2A"
  local -r _base_wallet="base/laptop:wallet-3/0x4C7219b760b71B9415E0e01Abd34d0f65631e57e"
  local -r _optimism_wallet="optimism/phone:wallet-3/0x53004E863Aa0F4028B154ECA65CFb32Eb5a8f5bB"

  # Invalid
  "${ci_shell[@]}" "$_exec '$_fetch account=INVALID chain=algorand,tezos year=2024' || exit 0"
  "${ci_shell[@]}" "$_exec '$_fetch account=ledger chain=INVALID year=2024' || exit 0"
  "${ci_shell[@]}" "$_exec '$_fetch account=ledger chain=algorand,tezos year=INVALID' || exit 0"

  "${ci_shell[@]}" "$_exec '$_fetch account=INVALID chain=${_algo_wallet} year=2024' || exit 0"
  "${ci_shell[@]}" "$_exec '$_fetch account=pera-wallet chain=INVALID year=2024' || exit 0"
  "${ci_shell[@]}" "$_exec '$_fetch account=pera-wallet chain=${_algo_wallet} year=INVALID' || exit 0"

  "${ci_shell[@]}" "$_exec '$_fetch account=ledger,pera-walletINVALID chain=${_tezos_wallet},${_algo_wallet} year=2024' || exit 0"
  "${ci_shell[@]}" "$_exec '$_fetch account=ledger,pera-wallet chain=INVALID${_tezos_wallet},${_algo_wallet} year=2024' || exit 0"
  "${ci_shell[@]}" "$_exec '$_fetch account=ledger,pera-wallet chain=${_tezos_wallet},${_algo_wallet} year=INVALID' || exit 0"

  "${ci_shell[@]}" "$_exec '$_fetch account=metamaskINVALID chain=arbitrum year=2025' || exit 0"
  "${ci_shell[@]}" "$_exec '$_fetch account=metamask chain=INVALIDarbitrum year=2025' || exit 0"
  "${ci_shell[@]}" "$_exec '$_fetch account=metamask chain=arbitrum year=INVALID' || exit 0"

  "${ci_shell[@]}" "$_exec '$_fetch account=metamaskINVALID chain=${_ethereum_wallet},${_polygon_wallet},${_arbitrum_wallet},${_base_wallet},${_optimism_wallet} year=2025' || exit 0"
  "${ci_shell[@]}" "$_exec '$_fetch account=metamask chain=INVALID${_ethereum_wallet},${_polygon_wallet},${_arbitrum_wallet},${_base_wallet},${_optimism_wallet} year=2025' || exit 0"
  "${ci_shell[@]}" "$_exec '$_fetch account=metamask chain=${_ethereum_wallet},${_polygon_wallet},${_arbitrum_wallet},${_base_wallet},${_optimism_wallet} year=INVALID' || exit 0"

  "${ci_shell[@]}" "$_exec '$_fetch account=INVALIDcoinbase-wallet,coinomi,ledger,metamask,pera-wallet chain=algorand,arbitrum,ethereum,polygon,tezos year=2024' || exit 0"
  "${ci_shell[@]}" "$_exec '$_fetch account=coinbase-wallet,coinomi,ledger,metamask,pera-wallet chain=INVALIDalgorand,arbitrum,ethereum,polygon,tezos year=2024' || exit 0"
  "${ci_shell[@]}" "$_exec '$_fetch account=coinbase-wallet,coinomi,ledger,metamask,pera-wallet chain=algorand,arbitrum,ethereum,polygon,tezos year=INVALID' || exit 0"

  # Valid
  #
  # NOTE: not currently fetching Optimism or Base for CI
  #
  #   "[FATAL] Free API access is not supported for this chain. Please upgrade your api plan for full chain coverage. https://etherscan.io/apis"
  #
  "${ci_shell[@]}" "$_exec '$_fetch account=ledger chain=${_tezos_wallet} year=2024'"
  "${ci_shell[@]}" "$_exec '$_fetch account=pera-wallet chain=${_algo_wallet} year=2024'"
  "${ci_shell[@]}" "$_exec '$_fetch account=ledger,pera-wallet chain=${_tezos_wallet},${_algo_wallet} year=2024'"

  "${ci_shell[@]}" "$_exec '$_fetch account=metamask chain=arbitrum year=2025'"
  "${ci_shell[@]}" "$_exec '$_fetch account=metamask chain=${_ethereum_wallet},${_polygon_wallet},${_arbitrum_wallet} year=2025'"

  "${ci_shell[@]}" "$_exec '$_fetch account=coinbase-wallet,coinomi,ledger,metamask,pera-wallet chain=algorand,arbitrum,ethereum,polygon,tezos year=2024'"
  client::finance::down
}

function container::finance::import()
{
  client::finance::up
  local -r _exec="$ci_archlinux exec"
  local -r _import="$ci_profile import"

  # mockups are guaranteed to start from this year
  local -r _year="2018"

  "${ci_shell[@]}" "$_exec '$_import year=${_year}'"
  client::finance::down
}

function container::finance::hledger()
{
  client::finance::up
  local -r _exec="$ci_archlinux exec"
  local -r _hledger="$ci_profile hledger"

  # Invalid
  "${ci_shell[@]}" "$_exec '$_hledger INVALID' || exit 0"

  # Valid
  "${ci_shell[@]}" "$_exec '$_hledger --pager=N bal assets liabilities cur:BTC'"
  client::finance::down
}

function container::finance::hledger-ui()
{
  client::finance::up
  local -r _exec="$ci_archlinux exec"
  local -r _hledger_ui="$ci_profile hledger-ui"

  # Invalid
  "${ci_shell[@]}" "$_exec '$_hledger_ui --INVALID' || exit 0"

  # Valid
  "${ci_shell[@]}" "$_exec '$_hledger_ui --pager=N bal assets liabilities cur:BTC & wait ; kill -9 %1'"
  "${ci_shell[@]}" "$_exec '$_hledger_ui & wait ; kill -9 %1'"
  client::finance::down
}

function container::finance::hledger-vui()
{
  client::finance::up
  local -r _exec="$ci_archlinux exec"
  local -r _hledger_vui="$ci_profile hledger-vui"

  # Invalid
  "${ci_shell[@]}" "$_exec '$_hledger_vui --INVALID' || exit 0"

  # Valid
  "${ci_shell[@]}" "$_exec '$_hledger_vui & wait ; kill -9 %1'"
  client::finance::down
}

function container::finance::hledger-web()
{
  client::finance::up
  local -r _exec="$ci_archlinux exec"
  local -r _hledger_web="$ci_profile hledger-web"

  # Invalid
  "${ci_shell[@]}" "$_exec '$_hledger_web --INVALID' || exit 0"

  # Valid
  "${ci_shell[@]}" "$_exec '$_hledger_web && pkill -9 hledger-web'"
  client::finance::down
}

function container::finance::meta()
{
  client::finance::up
  local -r _exec="$ci_archlinux exec"
  local -r _meta="$ci_profile meta"

  # Invalid
  "${ci_shell[@]}" "$_exec '$_meta INVALID' || exit 0"
  "${ci_shell[@]}" "$_exec '$_meta INVALID=' || exit 0"
  "${ci_shell[@]}" "$_exec '$_meta INVALID=INVALID' || exit 0"

  # Valid
  "${ci_shell[@]}" "$_exec '$_meta ticker=btc & wait ; kill -9 %1'"
  "${ci_shell[@]}" "$_exec '$_meta ticker=btc format=bech32 & wait ; kill -9 %1'"
  "${ci_shell[@]}" "$_exec '$_meta ticker=btc format=bech32 account=electrum & wait ; kill -9 %1'"
  client::finance::down
}

function container::finance::reports()
{
  client::finance::up
  local -r _exec="$ci_archlinux exec"
  local -r _reports="$ci_profile reports"

  # Invalid
  "${ci_shell[@]}" "$_exec '$_reports INVALID' || exit 0"
  "${ci_shell[@]}" "$_exec '$_reports INVALID=INVALID' || exit 0"
  "${ci_shell[@]}" "$_exec '$_reports INVALID=INVALID' || exit 0"

  "${ci_shell[@]}" "$_exec '$_reports all=INVALID' || exit 0"
  "${ci_shell[@]}" "$_exec '$_reports all=type interval=INVALID year=2022' || exit 0"
  "${ci_shell[@]}" "$_exec '$_reports all=type interval=annual year=INVALID' || exit 0"

  "${ci_shell[@]}" "$_exec '$_reports type=INVALID,INVALID interval=quarterly,monthly' || exit 0"
  "${ci_shell[@]}" "$_exec '$_reports type=is,bs interval=INVALID,INVALID' || exit 0"

  # Valid
  "${ci_shell[@]}" "$_exec '$_reports all=all'"
  "${ci_shell[@]}" "$_exec '$_reports all=type interval=annual year=2022'"
  "${ci_shell[@]}" "$_exec '$_reports type=is,bs interval=quarterly,monthly'"
  client::finance::down
}

function container::finance::taxes()
{
  client::finance::up
  local -r _exec="$ci_archlinux exec"
  local -r _taxes="$ci_profile taxes"

  # Invalid
  "${ci_shell[@]}" "$_exec '$_taxes INVALID' || exit 0"
  "${ci_shell[@]}" "$_exec '$_taxes INVALID=INVALID' || exit 0"
  "${ci_shell[@]}" "$_exec '$_taxes INVALID=INVALID' || exit 0"

  "${ci_shell[@]}" "$_exec '$_taxes all=INVALID' || exit 0"
  "${ci_shell[@]}" "$_exec '$_taxes all=all year=INVALID' || exit 0"

  "${ci_shell[@]}" "$_exec '$_taxes all=account tag=INVALID write=off year=2022' || exit 0"
  "${ci_shell[@]}" "$_exec '$_taxes all=account tag=income write=INVALID year=2022' || exit 0"
  "${ci_shell[@]}" "$_exec '$_taxes all=account tag=income write=off year=INVALID' || exit 0"

  "${ci_shell[@]}" "$_exec '$_taxes tag=INVALID,INVALID account=gemini,coinbase year=2022' || exit 0"
  "${ci_shell[@]}" "$_exec '$_taxes tag=income,spends account=INVALID,INVALID year=2022' || exit 0"
  "${ci_shell[@]}" "$_exec '$_taxes tag=income,spends account=gemini,coinbase year=INVALID' || exit 0"

  "${ci_shell[@]}" "$_exec '$_taxes all=tag account=INVALID year=all write=off' || exit 0"
  "${ci_shell[@]}" "$_exec '$_taxes all=tag account=nexo year=INVALID write=off' || exit 0"
  "${ci_shell[@]}" "$_exec '$_taxes all=tag account=nexo year=all write=INVALID' || exit 0"

  # Valid
  "${ci_shell[@]}" "$_exec '$_taxes all=all'"
  "${ci_shell[@]}" "$_exec '$_taxes all=all year=all'"
  "${ci_shell[@]}" "$_exec '$_taxes all=account tag=income write=off'"
  "${ci_shell[@]}" "$_exec '$_taxes tag=income,spends account=gemini,coinbase year=2022'"
  "${ci_shell[@]}" "$_exec '$_taxes all=tag account=nexo year=all write=off'"
  client::finance::down
}

function container::finance::times()
{
  client::finance::up
  local -r _exec="$ci_archlinux exec"
  local -r _times="$ci_profile times"

  # TODO: invalid

  # NOTE: impl-specific: currently, timewarrior requires confirmation
  #   which cannot be passed here (via "< <(echo yes)").
  local -r _path="${HOME:?}/docker-finance/flow/times/timew"
  mkdir -p "$_path" && touch "${_path}/timewarrior.cfg"

  # Initial run presents confirmation dialog (so echo "yes")
  "${ci_shell[@]}" "$_exec '$_times start desc=\"Task 1\" comment=\"My long comment\" tag:type=consulting'"

  # Create time
  sleep 3s

  # Stop time
  "${ci_shell[@]}" "$_exec '$_times stop'"

  # If no tag found in summary, will return a single line but exit 0
  [[ $("${ci_shell[@]}" "$_exec '$_times summary tag:type=consulting' | wc -l") -eq 1 ]] && exit 1

  # A single entry should return only 3 lines
  [[ $("${ci_shell[@]}" "$_exec '$_times export' | wc -l") -ne 3 ]] && exit 1
  client::finance::down
}

function container::finance::plugins()
{
  client::finance::up
  local -r _exec="$ci_archlinux exec"
  local -r _plugins="$ci_profile plugins"

  # Invalid
  "${ci_shell[@]}" "$_exec '$_plugins INVALID' || exit 0"
  "${ci_shell[@]}" "$_exec '$_plugins repo/INVALID' || exit 0"
  "${ci_shell[@]}" "$_exec '$_plugins custom/INVALID' || exit 0"

  # Valid
  "${ci_shell[@]}" "$_exec '$_plugins repo/example.bash'"
  # WARNING: timeclock plugin requires dfi `times` setup/functionality
  # and must run *AFTER* the above container::finance::times() test
  "${ci_shell[@]}" "$_exec '$_plugins repo/timew_to_timeclock.bash'"
  client::finance::down
}

function container::finance::root()
{
  client::finance::up
  local -r _exec="$ci_archlinux exec"
  local -r _root="$ci_profile root"

  # TODO: if tests failing, interpreter not bailing because gtest will catch throws
  local -r _exit="| grep FAILED && exit 1 || exit 0"

  #
  # Base
  #

  "${ci_shell[@]}" "$_exec '$_root INVALID' || exit 0"
  "${ci_shell[@]}" "$_exec '$_root & wait ; kill -9 %1'"

  #
  # Macros
  #

  # Invalid
  "${ci_shell[@]}" "$_exec '$_root INVALID' || exit 0"
  "${ci_shell[@]}" "$_exec '$_root macro/hash.C' || exit 0"
  "${ci_shell[@]}" "$_exec '$_root macro/crypto/hash.C' || exit 0"
  "${ci_shell[@]}" "$_exec '$_root macro/repo/crypto/hash.C' || exit 0"

  # Valid
  "${ci_shell[@]}" "$_exec '$_root macros/repo/crypto/hash.C'"
  "${ci_shell[@]}" "$_exec '$_root macros/repo/crypto/hash.C test\\\\\ message'"

  "${ci_shell[@]}" "$_exec '$_root macros/repo/crypto/random.C'"

  "${ci_shell[@]}" "$_exec '$_root macros/repo/test/benchmark.C'"
  "${ci_shell[@]}" "$_exec '$_root macros/repo/test/benchmark.C Random\*'"

  "${ci_shell[@]}" "$_exec '$_root macros/repo/test/unit.C' $_exit"
  "${ci_shell[@]}" "$_exec '$_root macros/repo/test/unit.C Pluggable\*:Macro\*:Plugin\*' $_exit"

  "${ci_shell[@]}" "$_exec '$_root macros/repo/web/server.C dfi::common::exit\(0,\\\\\\\"Server\\\\\\\"\)\;'"
  # TODO: ^ return value of macro

  #
  # Plugins
  #

  "${ci_shell[@]}" "$_exec '$_root plugins/repo/example/example.cc dfi::plugin::example::MyExamples\\ my\;my.example1\(\)\;dfi::common::exit\(0,\\\\\\\"MyExamples\\\\\\\"\)\;'"

  # WARNING: bitcoin plugin requires dfi client-side bitcoin plugin
  # and must run *AFTER* the above client::finance::plugins() test
  "${ci_shell[@]}" "$_exec '$_root plugins/repo/bitcoin/bitcoin.cc try{dfi::macro::load\(\\\\\\\"repo/test/unit.C\\\\\\\"\)\;}catch\(...\){dfi::common::exit\(1\)\;}dfi::common::exit\(0\)\;' $_exit"
  client::finance::down
}

# TODO: all           Fetches, imports, generates taxes and reports w/ [args] year

#
# client::dev-tools
#

function client::dev-tools::gen()
{
  local -r _tags=("micro" "tiny" "slim" "default")
  local -r _types=("env" "build" "superscript" "env,build,superscript")

  # TODO: 'default' should only be supported
  for _tag in "${_tags[@]}"; do
    for _type in "${_types[@]}"; do
      local _gen="dfi dev-tools/${USER}:${_tag} gen"

      # Invalid
      "${ci_shell[@]}" "$_gen type=${_type}INVALID || exit 0"
      "${ci_shell[@]}" "$_gen INVALID=${_type} || exit 0"

      # Valid
      "${ci_shell[@]}" "$_gen type=${_type} confirm=no"
    done
  done
}

function client::dev-tools::edit()
{
  local -r _tags=("micro" "tiny" "slim" "default")

  local _types=()
  _types+=("env")
  _types+=("shell" "superscript")
  _types+=("build" "dockerfile")
  _types+=("env,shell,build")
  _types+=("env,superscript,dockerfile")
  declare -r _types

  # Currently, only 'default' is supported
  for _tag in "${_tags[@]}"; do
    for _type in "${_types[@]}"; do
      local _edit="dfi dev-tools/${USER}:${_tag} edit"

      # Invalid
      "${ci_shell[@]}" "$_edit type=${_type}INVALID & wait ; kill -9 %1 || exit 0"
      "${ci_shell[@]}" "$_edit INVALID=${_type} & wait ; kill -9 %1 || exit 0"

      # Valid
      if "${ci_shell[@]}" "$_edit type=${_type} & wait ; kill -9 %1"; then
        [[ "$_tag" != default ]] || continue
      fi
    done
  done
}

# shellcheck disable=SC2178
# shellcheck disable=SC2128
function client::dev-tools::build()
{
  local -r types=("micro" "tiny" "slim" "default")

  # Currently, only 'default' is supported
  for _type in "${types[@]}"; do
    local _build="dfi dev-tools/${USER}:${_type} build"

    # Invalid
    "${ci_shell[@]}" "$_build type=${_type}INVALID || exit 0"
    "${ci_shell[@]}" "$_build INVALID=${_type} || exit 0"

    # Valid
    if "${ci_shell[@]}" "$_build type=${_type}"; then
      [[ "$_type" != default ]] || continue
    fi
  done
}

function client::dev-tools::backup()
{
  local -r _backup=("micro" "tiny" "slim")

  # Currently, only 'default' is supported
  for _type in "${_backup[@]}"; do
    if "${ci_shell[@]}" "dfi dev-tools/${USER}:${_type} backup"; then
      [[ "$_type" != default ]] || continue
    fi
  done

  "${ci_shell[@]}" "$ci_dev_tools backup"
}

function client::dev-tools::up()
{
  local -r _detach="&"
  "${ci_shell[@]}" "$ci_dev_tools up $_detach wait"
}

function client::dev-tools::stop()
{
  "${ci_shell[@]}" "$ci_dev_tools stop"
}

function client::dev-tools::start()
{
  "${ci_shell[@]}" "$ci_dev_tools start"
}

function client::dev-tools::down()
{
  "${ci_shell[@]}" "$ci_dev_tools down"
}

function client::dev-tools::shell()
{
  client::dev-tools::up
  local -r _shell="$ci_dev_tools shell"
  local -r _args=(" " "user=${USER}" "user=root")

  for _arg in "${_args[@]}"; do
    # Invalid
    "${ci_shell[@]}" "$_shell INVALID || exit 0"
    "${ci_shell[@]}" "$_shell user=INVALID || exit 0"

    # Valid
    "${ci_shell[@]}" "$_shell $_arg & wait ; kill -9 %1"
  done
  client::dev-tools::down
}

function client::dev-tools::exec()
{
  client::dev-tools::up
  local -r _exec="$ci_dev_tools exec"
  "${ci_shell[@]}" "$_exec 'exit 1' || exit 0"
  "${ci_shell[@]}" "$_exec 'exit 0'"
  client::dev-tools::down
}

function client::dev-tools::plugins()
{
  local -r _plugins="$ci_dev_tools plugins"

  # Invalid
  "${ci_shell[@]}" "$_plugins INVALID || exit 0"
  "${ci_shell[@]}" "$_plugins repo/INVALID || exit 0"
  "${ci_shell[@]}" "$_plugins custom/INVALID || exit 0"

  # Valid
  "${ci_shell[@]}" "$_plugins repo/example.bash"
}

function client::dev-tools::run()
{
  # Invalid
  "${ci_shell[@]}" "$ci_dev_tools run 'hash INVALID' || exit 0"

  # Valid
  "${ci_shell[@]}" "$ci_dev_tools run 'hash bash'"
}

function client::dev-tools::version()
{
  local _types
  _types+=("client" "container" "short" "all")
  _types+=("client,container,short")

  for _type in "${_types[@]}"; do
    local _version="$ci_dev_tools version"
    # Invalid
    "${ci_shell[@]}" "$_version type=${_type}INVALID || exit 0"
    "${ci_shell[@]}" "$_version INVALID=${_type} || exit 0"

    # Valid
    "${ci_shell[@]}" "$_version type=${_type}"
  done
}

function client::dev-tools::update()
{
  local -r types=("micro" "tiny" "slim" "default")

  # Currently, only 'default' is supported
  for _type in "${types[@]}"; do
    local _update="dfi dev-tools/${USER}:${_type} update"

    # Invalid
    "${ci_shell[@]}" "$_update type=${_type}INVALID || exit 0"
    "${ci_shell[@]}" "$_update INVALID=${_type} || exit 0"

    # Valid
    if "${ci_shell[@]}" "$_update type=${_type}"; then
      [[ "$_type" != default ]] || continue
    fi
  done
}

function client::dev-tools::rm()
{
  local _image="dev-tools/${USER}:default"

  "${ci_shell[@]}" "dfi $_image rm"
  docker images -q "$_image" && return $?
  # NOTE: don't attempt `dfi` container commands or else image will try to pull/re-build

  # Re-build (from cache) for future test cases
  client::dev-tools::build
}

function client::dev-tools::license()
{
  local _files
  _files+=("README.md")
  _files+=("client/Dockerfiles/local/dev-tools/Dockerfile")
  _files+=("client/docker-finance.yaml")
  _files+=("container/.clang-format")
  _files+=("container/plugins/root/example/example.cc")
  _files+=("container/src/finance/completion.bash")
  _files+=("container/src/finance/lib/internal/fetch/fetch.php")
  _files+=("container/src/hledger-flow/accounts/coinbase/coinbase-shared.rules")
  _files+=("container/src/hledger-flow/accounts/coinbase/template/coinbase/platform/1-in/mockup/2023/XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXBTC_transactions.csv")
  _files+=("container/src/root/common/utility.hh")
  _files+=("container/src/root/macro/rootlogon.C")

  for _file in "${_version[@]}"; do
    "${ci_shell[@]}" "$ci_dev_tools license file=${_file}"
  done
}

function client::dev-tools::linter()
{
  local _type
  _type+=("bash" "c++" "php")
  _type+=("bash,c++,php")
  _type+=("all")

  for _type in "${_version[@]}"; do
    "${ci_shell[@]}" "$ci_dev_tools linter type=${_type}"
  done
}

function client::dev-tools::doxygen()
{
  local -r _args=("gen" "rm")
  for _arg in "${_args[@]}"; do
    "${ci_shell[@]}" "$ci_dev_tools doxygen $_arg"
  done
}

"$@"
