#!/usr/bin/env bash

# Quant trade framework plugin for docker-finance
#
# Implements the core equations from the Quant framework (paper.md)
# as a lightweight bash plugin using bc for arithmetic.
#
# Equations implemented:
#   - Overhead (OH)
#   - Effective overhead (EO)
#   - Sigmoid entry levels with funding allocation
#   - Take-profit targets (per-level)
#   - Fee-neutral break-even
#   - Cycle profit estimation
#   - Savings extraction
#
# Usage (inside dfi container):
#   dfi <instance> plugins repo/quant.bash <command> [args]
#
# Commands:
#   help                Show this help
#   overhead            Compute overhead ratio
#   entries             Generate sigmoid entry levels with funding
#   tp                  Compute take-profit targets for entries
#   cycle               Simulate a full cycle (entries ? TPs ? profit)
#   journal             Generate hledger journal entries for a cycle

#
# "Libraries"
#

[ -z "$DOCKER_FINANCE_CONTAINER_REPO" ] && exit 1
source "${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/lib_finance.bash"

[[ -z "$global_parent_profile" || -z "$global_arg_delim_1" || -z "$global_child_profile" ]] && lib_utils::die_fatal
instance="${global_parent_profile}${global_arg_delim_1}${global_child_profile}"

lib_finance::finance "$instance" || lib_utils::die_fatal

#
# Math helpers (bc wrappers)
#

_bc() { echo "$1" | bc -l; }

_sigmoid() {
  # sigmoid(x) = 1 / (1 + e^(-x))
  local x="$1"
  _bc "1.0 / (1.0 + e(-1.0 * ($x)))"
}

_max() { _bc "if ($1 > $2) $1 else $2"; }
_min() { _bc "if ($1 < $2) $1 else $2"; }
_clamp() { _bc "if ($1 < $2) $2 else if ($1 > $3) $3 else $1"; }

#
# Core equations (from paper.md / MultiHorizonEngine.h)
#

# OH = (f_s * f_h * dt * n_sym * (1 + n_d)) / ((P/Q) * T + K)
quant::overhead() {
  local price="$1" qty="$2" fee_spread="$3" fee_hedge="$4"
  local dt="$5" sym_count="$6" pump="$7" coeff_k="$8" future_n="${9:-0}"

  local fee_comp=$(_bc "$fee_spread * $fee_hedge * $dt")
  local trade_scale=$(_bc "1.0 + $future_n")
  local numerator=$(_bc "$fee_comp * $sym_count * $trade_scale")
  local price_per_qty=$(_bc "$price / $qty")
  local denominator=$(_bc "$price_per_qty * $pump + $coeff_k")

  if [[ $(_bc "$denominator == 0") == "1" ]]; then
    echo "0"
  else
    _bc "$numerator / $denominator"
  fi
}

# EO = OH + s * f_h * dt + f_s * f_h * dt
quant::effective_overhead() {
  local oh="$1" surplus="$2" fee_spread="$3" fee_hedge="$4" dt="$5"
  _bc "$oh + $surplus * $fee_hedge * $dt + $fee_spread * $fee_hedge * $dt"
}

# Generate N sigmoid-distributed entry levels
quant::generate_entries() {
  local current_price="$1" n_levels="$2" steepness="$3" risk="$4" total_funds="$5"
  local range_below="${6:-0}" range_above="${7:-0}"

  local price_low price_high
  if [[ $(_bc "$range_above > 0 || $range_below > 0") == "1" ]]; then
    price_low=$(_max "$(_bc "$current_price - $range_below")" "0.00001")
    price_high=$(_bc "$current_price + $range_above")
  else
    price_low="0.00001"
    price_high="$current_price"
  fi

  # Sigmoid endpoints for normalization
  local sig0=$(_sigmoid "$(_bc "-1 * $steepness * 0.5")")
  local sig1=$(_sigmoid "$(_bc "$steepness * 0.5")")
  local sig_range=$(_bc "$sig1 - $sig0")

  # First pass: compute weights
  local -a norms weights
  local weight_sum="0"
  for ((i = 0; i < n_levels; i++)); do
    local t
    if ((n_levels > 1)); then
      t=$(_bc "$i / ($n_levels - 1)")
    else
      t="1.0"
    fi
    local sig_val=$(_sigmoid "$(_bc "$steepness * ($t - 0.5)")")
    norms[$i]=$(_bc "($sig_val - $sig0) / $sig_range")

    # risk-warped funding: risk=0 conservative, 0.5 uniform, 1 aggressive
    weights[$i]=$(_bc "(1.0 - $risk) * ${norms[$i]} + $risk * (1.0 - ${norms[$i]})")
    weight_sum=$(_bc "$weight_sum + ${weights[$i]}")
  done

  # Second pass: output levels
  printf "%-6s %-14s %-14s %-12s %-14s %-10s\n" \
    "Level" "Entry Price" "Break Even" "Funding %" "Funding \$" "Qty"

  for ((i = 0; i < n_levels; i++)); do
    local entry_price=$(_bc "$price_low + ${norms[$i]} * ($price_high - $price_low)")
    local fraction=$(_bc "${weights[$i]} / $weight_sum")
    local funding=$(_bc "$total_funds * $fraction")
    local funding_qty=$(_bc "$funding / $entry_price")

    # Store for caller
    _ENTRY_PRICES[$i]="$entry_price"
    _ENTRY_FUNDING[$i]="$funding"
    _ENTRY_QTY[$i]="$funding_qty"

    printf "%-6d %-14.2f %-14.2f %-12.4f %-14.2f %-10.6f\n" \
      "$i" \
      "$(_bc "$entry_price / 1")" \
      "$(_bc "$entry_price * (1.0 + $__OH) / 1")" \
      "$(_bc "$fraction * 100 / 1")" \
      "$(_bc "$funding / 1")" \
      "$(_bc "$funding_qty / 1")"
  done
}

# TP for a single level
quant::take_profit() {
  local entry_price="$1" eo="$2" surplus="$3"
  _bc "$entry_price * (1.0 + $eo + $surplus)"
}

# Cycle profit: sum of (TP_i - entry_i) * qty_i for all levels
quant::cycle_profit() {
  local n_levels="$1" savings_rate="$2"
  local total_profit="0"

  for ((i = 0; i < n_levels; i++)); do
    local entry="${_ENTRY_PRICES[$i]}"
    local qty="${_ENTRY_QTY[$i]}"
    local tp=$(quant::take_profit "$entry" "$__EO" "$__SURPLUS")
    local level_profit=$(_bc "($tp - $entry) * $qty")
    total_profit=$(_bc "$total_profit + $level_profit")
  done

  local savings=$(_bc "$total_profit * $savings_rate")
  local reinvest=$(_bc "$total_profit - $savings")

  echo "  Gross cycle profit: \$$(_bc "$total_profit / 1")"
  echo "  Savings extracted:  \$$(_bc "$savings / 1") ($(_bc "$savings_rate * 100")%)"
  echo "  Reinvested:         \$$(_bc "$reinvest / 1")"
  echo "  Next cycle pump:    \$$(_bc "$__PUMP + $reinvest / 1")"
}

# Generate hledger journal entries for a simulated cycle
quant::journal() {
  local symbol="$1" n_levels="$2" date="${3:-$(date +%Y-%m-%d)}" savings_rate="$4"

  local total_cost="0"
  local total_revenue="0"
  local total_fees="0"

  echo "; Quant cycle — ${symbol} — generated $(date +%Y-%m-%d)"
  echo ""

  # Buy entries
  for ((i = 0; i < n_levels; i++)); do
    local entry="${_ENTRY_PRICES[$i]}"
    local qty="${_ENTRY_QTY[$i]}"
    local funding="${_ENTRY_FUNDING[$i]}"
    local fee=$(_bc "$funding * $__FEE_SPREAD")
    total_cost=$(_bc "$total_cost + $funding")
    total_fees=$(_bc "$total_fees + $fee")

    cat <<EOF
${date} Buy ${symbol} level ${i}
    assets:crypto:${symbol,,}    ${qty} ${symbol} @ \$$(_bc "$entry / 1")
    expenses:fees:exchange       \$$(_bc "$fee / 1")
    assets:bank:trading         \$-$(_bc "($funding + $fee) / 1")

EOF
  done

  # Sell exits (TPs)
  for ((i = 0; i < n_levels; i++)); do
    local entry="${_ENTRY_PRICES[$i]}"
    local qty="${_ENTRY_QTY[$i]}"
    local tp=$(quant::take_profit "$entry" "$__EO" "$__SURPLUS")
    local revenue=$(_bc "$tp * $qty")
    local fee=$(_bc "$revenue * $__FEE_SPREAD")
    total_revenue=$(_bc "$total_revenue + $revenue")
    total_fees=$(_bc "$total_fees + $fee")

    cat <<EOF
${date} Sell ${symbol} TP level ${i}
    assets:bank:trading          \$$(_bc "($revenue - $fee) / 1")
    expenses:fees:exchange       \$$(_bc "$fee / 1")
    assets:crypto:${symbol,,}   -${qty} ${symbol} @ \$$(_bc "$tp / 1")

EOF
  done

  # Savings extraction
  local gross=$(_bc "$total_revenue - $total_cost - $total_fees")
  local savings=$(_bc "$gross * $savings_rate")

  if [[ $(_bc "$savings > 0") == "1" ]]; then
    cat <<EOF
${date} Quant savings extraction
    assets:savings:quant         \$$(_bc "$savings / 1")
    income:trading:quant        \$-$(_bc "$savings / 1")

EOF
  fi

  echo "; Total fees: \$$(_bc "$total_fees / 1")"
  echo "; Net profit: \$$(_bc "$gross / 1")"
  echo "; Savings:    \$$(_bc "$savings / 1")"
}

#
# Command interface
#

function main() {
  local cmd="${1:-help}"
  shift 2>/dev/null

  case "$cmd" in
    help)
      cat <<'USAGE'
Quant Framework Plugin for docker-finance

Commands:

  overhead <price> <qty> <fee_spread> <fee_hedge> <dt> <sym_count> <pump> <K> [n_d]
      Compute overhead (OH) and effective overhead (EO)

  entries <price> <levels> <steepness> <risk> <funds> <surplus> <fee_spread> <fee_hedge> <dt> <sym_count> <K> [range_below] [range_above]
      Generate sigmoid entry levels with funding allocation

  cycle <price> <levels> <steepness> <risk> <funds> <surplus> <fee_spread> <fee_hedge> <dt> <sym_count> <K> <savings_rate> [range_below] [range_above]
      Full cycle simulation: entries ? TPs ? profit ? savings

  journal <symbol> <price> <levels> <steepness> <risk> <funds> <surplus> <fee_spread> <fee_hedge> <dt> <sym_count> <K> <savings_rate> [date] [range_below] [range_above]
      Generate hledger journal for a cycle (pipe to file)

  balance
      Show current crypto balances from hledger

Example:
  # BTC at $100000, 10 levels, steepness 6, risk 0.5, $5000 capital,
  # 2% surplus, 0.1% fee, hedge 1.5, dt 1, 1 symbol, K=0, save 10%
  quant.bash cycle 100000 10 6 0.5 5000 0.02 0.001 1.5 1 1 0 0.10
USAGE
      ;;

    overhead)
      local price="$1" qty="$2" fs="$3" fh="$4" dt="$5" sc="$6" pump="$7" k="$8" nd="${9:-0}"
      local oh=$(quant::overhead "$price" "$qty" "$fs" "$fh" "$dt" "$sc" "$pump" "$k" "$nd")
      local eo=$(quant::effective_overhead "$oh" "0" "$fs" "$fh" "$dt")
      echo "  Overhead (OH):           $oh ($(_bc "$oh * 100")%)"
      echo "  Effective overhead (EO): $eo ($(_bc "$eo * 100")%)"
      ;;

    entries)
      local price="$1" levels="$2" steep="$3" risk="$4" funds="$5"
      local surplus="$6" fs="$7" fh="$8" dt="$9" sc="${10}" k="${11}"
      local rb="${12:-0}" ra="${13:-0}"

      __PUMP="$funds"; __FEE_SPREAD="$fs"; __SURPLUS="$surplus"
      __OH=$(quant::overhead "$price" "1" "$fs" "$fh" "$dt" "$sc" "$funds" "$k")
      __EO=$(quant::effective_overhead "$__OH" "$surplus" "$fs" "$fh" "$dt")

      echo "  OH=$__OH  EO=$__EO"
      echo ""
      declare -a _ENTRY_PRICES _ENTRY_FUNDING _ENTRY_QTY
      quant::generate_entries "$price" "$levels" "$steep" "$risk" "$funds" "$rb" "$ra"
      ;;

    cycle)
      local price="$1" levels="$2" steep="$3" risk="$4" funds="$5"
      local surplus="$6" fs="$7" fh="$8" dt="$9" sc="${10}" k="${11}"
      local save="${12}" rb="${13:-0}" ra="${14:-0}"

      __PUMP="$funds"; __FEE_SPREAD="$fs"; __SURPLUS="$surplus"
      __OH=$(quant::overhead "$price" "1" "$fs" "$fh" "$dt" "$sc" "$funds" "$k")
      __EO=$(quant::effective_overhead "$__OH" "$surplus" "$fs" "$fh" "$dt")

      echo "=== Quant Cycle Simulation ==="
      echo "  Price: \$$price  Levels: $levels  Funds: \$$funds"
      echo "  Surplus: $(_bc "$surplus * 100")%  Fee: $(_bc "$fs * 100")%  Hedge: $fh"
      echo "  OH=$(_bc "$__OH * 100")%  EO=$(_bc "$__EO * 100")%"
      echo ""
      echo "--- Entry Levels ---"
      declare -a _ENTRY_PRICES _ENTRY_FUNDING _ENTRY_QTY
      quant::generate_entries "$price" "$levels" "$steep" "$risk" "$funds" "$rb" "$ra"
      echo ""
      echo "--- Take Profit Targets ---"
      printf "%-6s %-14s %-14s %-14s\n" "Level" "Entry" "TP" "Profit/unit"
      for ((i = 0; i < levels; i++)); do
        local e="${_ENTRY_PRICES[$i]}"
        local tp=$(quant::take_profit "$e" "$__EO" "$surplus")
        printf "%-6d %-14.2f %-14.2f %-14.6f\n" \
          "$i" "$(_bc "$e / 1")" "$(_bc "$tp / 1")" "$(_bc "($tp - $e) / 1")"
      done
      echo ""
      echo "--- Cycle Result ---"
      quant::cycle_profit "$levels" "$save"
      ;;

    journal)
      local symbol="$1" price="$2" levels="$3" steep="$4" risk="$5" funds="$6"
      local surplus="$7" fs="$8" fh="$9" dt="${10}" sc="${11}" k="${12}"
      local save="${13}" jdate="${14:-$(date +%Y-%m-%d)}" rb="${15:-0}" ra="${16:-0}"

      __PUMP="$funds"; __FEE_SPREAD="$fs"; __SURPLUS="$surplus"
      __OH=$(quant::overhead "$price" "1" "$fs" "$fh" "$dt" "$sc" "$funds" "$k")
      __EO=$(quant::effective_overhead "$__OH" "$surplus" "$fs" "$fh" "$dt")

      declare -a _ENTRY_PRICES _ENTRY_FUNDING _ENTRY_QTY
      quant::generate_entries "$price" "$levels" "$steep" "$risk" "$funds" "$rb" "$ra" >/dev/null
      quant::journal "$symbol" "$levels" "$jdate" "$save"
      ;;

    balance)
      echo "Current crypto balances:"
      echo ""
      lib_finance::hledger bal assets:crypto
      ;;

    *)
      lib_utils::print_error "Unknown command: $cmd"
      main help
      ;;
  esac
}

main "$@"

# vim: sw=2 sts=2 si ai et
