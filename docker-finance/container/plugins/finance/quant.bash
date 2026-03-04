#!/usr/bin/env bash

# Quant plugin helper for docker-finance
#
# Provides convenience functions for interacting with the Quant
# trade-math plugin from the docker-finance shell.
#
# Usage:
#   source quant.bash
#   quant_plan 2650 1 4 6 0.5 1000
#   quant_profit 100 110 10 0.5 0.5

# ?? Libraries ??????????????????????????????????????????????????

[ -z "$DOCKER_FINANCE_CONTAINER_REPO" ] && exit 1
source "${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/lib_finance.bash"

[[ -z "$global_parent_profile" || -z "$global_arg_delim_1" || -z "$global_child_profile" ]] && lib_utils::die_fatal
instance="${global_parent_profile}${global_arg_delim_1}${global_child_profile}"

# Initialize "constructor"
lib_finance::finance "$instance" || lib_utils::die_fatal

# ?? Plugin path ????????????????????????????????????????????????

readonly QUANT_PLUGIN_PATH="repo/quant/quant.cc"

# ?? Helpers ????????????????????????????????????????????????????

quant_load() {
  echo "Loading Quant plugin..."
  root -l -b -q -e "dfi::plugin::reload(\"${QUANT_PLUGIN_PATH}\")"
}

quant_unload() {
  echo "Unloading Quant plugin..."
  root -l -b -q -e "dfi::plugin::reload(\"${QUANT_PLUGIN_PATH}\"); dfi::plugin::quant::quant_cc::unload();"
}

# Generate and print a serial plan
#   quant_plan <price> <qty> <levels> <steepness> <risk> <funds>
#              [fee_spread] [fee_hedge] [delta_time] [symbol_count]
#              [coeff_k] [surplus_rate] [max_risk] [min_risk]
quant_plan() {
  local price="${1:?price required}"
  local qty="${2:?quantity required}"
  local levels="${3:?levels required}"
  local steepness="${4:-6.0}"
  local risk="${5:-0.5}"
  local funds="${6:?funds required}"
  local fee_spread="${7:-0.001}"
  local fee_hedge="${8:-2.0}"
  local delta_time="${9:-1.0}"
  local symbol_count="${10:-1}"
  local coeff_k="${11:-1.0}"
  local surplus_rate="${12:-0.02}"
  local max_risk="${13:-0.05}"
  local min_risk="${14:-0.01}"

  root -l -b -q -e "
    dfi::plugin::reload(\"${QUANT_PLUGIN_PATH}\");
    namespace q = dfi::plugin::quant;
    q::internal::SerialParams sp;
    sp.current_price = ${price};
    sp.quantity = ${qty};
    sp.levels = ${levels};
    sp.steepness = ${steepness};
    sp.risk = ${risk};
    sp.available_funds = ${funds};
    sp.fee_spread = ${fee_spread};
    sp.fee_hedging_coefficient = ${fee_hedge};
    sp.delta_time = ${delta_time};
    sp.symbol_count = ${symbol_count};
    sp.coefficient_k = ${coeff_k};
    sp.surplus_rate = ${surplus_rate};
    sp.max_risk = ${max_risk};
    sp.min_risk = ${min_risk};
    auto plan = q::Quant::serial_plan(sp);
    q::Quant::print_plan(plan);
  "
}

# Compute profit
#   quant_profit <entry> <exit> <qty> <buy_fee> <sell_fee>
quant_profit() {
  local entry="${1:?entry price required}"
  local exit_p="${2:?exit price required}"
  local qty="${3:?quantity required}"
  local buy_fee="${4:-0.0}"
  local sell_fee="${5:-0.0}"

  root -l -b -q -e "
    dfi::plugin::reload(\"${QUANT_PLUGIN_PATH}\");
    namespace q = dfi::plugin::quant;
    auto p = q::Quant::profit(${entry}, ${exit_p}, ${qty}, ${buy_fee}, ${sell_fee});
    q::Quant::print_profit(p);
  "
}

# Compute overhead
#   quant_overhead <price> <qty> <fee_spread> <fee_hedge> <dt>
#                  <symbol_count> <pump> <coeff_k> <surplus_rate>
quant_overhead() {
  local price="${1:?price required}"
  local qty="${2:?quantity required}"
  local fee_spread="${3:-0.001}"
  local fee_hedge="${4:-2.0}"
  local dt="${5:-1.0}"
  local symbol_count="${6:-1}"
  local pump="${7:?pump required}"
  local coeff_k="${8:-1.0}"
  local surplus_rate="${9:-0.02}"

  root -l -b -q -e "
    dfi::plugin::reload(\"${QUANT_PLUGIN_PATH}\");
    namespace q = dfi::plugin::quant;
    q::Quant::print_overhead(${price}, ${qty}, ${fee_spread}, ${fee_hedge}, ${dt}, ${symbol_count}, ${pump}, ${coeff_k}, ${surplus_rate});
  "
}

# ?? Implementation ?????????????????????????????????????????????

function main()
{
  local -r _help="
Quant plugin helpers loaded.

Available commands:
  quant_load              Load plugin into ROOT
  quant_unload            Unload plugin from ROOT
  quant_plan              Generate serial entry plan
  quant_profit            Compute profit
  quant_overhead          Compute overhead

Arguments: '${*}'
"
  lib_utils::print_custom "$_help"
}

main "$@"

# vim: sw=2 sts=2 si ai et
