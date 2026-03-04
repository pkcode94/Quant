// Quant plugin for docker-finance | internal math engine
//
// Pure-math implementation of the Quant overhead, serial plan,
// exit plan, chain, and profit calculators.  No UI, no database,
// no HTTP, no login, no premium.
//
// Loaded by the parent plugin entrypoint; not intended to be
// used directly.

//! \file
//! \note File intended to be loaded into ROOT.cern framework / Cling interpreter
//! \since docker-finance 1.1.0

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace dfi {
namespace plugin {
namespace quant {
namespace internal {

// ?? Clamping ??????????????????????????????????????????????????

inline double clamp01(double v)
{
  return (v < 0.0) ? 0.0 : (v > 1.0) ? 1.0 : v;
}

inline double clamp(double v, double lo, double hi)
{
  return (v < lo) ? lo : (v > hi) ? hi : v;
}

inline double floor_eps(double v)
{
  return (v < std::numeric_limits<double>::epsilon())
       ? std::numeric_limits<double>::epsilon() : v;
}

// ?? Sigmoid core ??????????????????????????????????????????????

inline double sigmoid(double x)
{
  return 1.0 / (1.0 + std::exp(-x));
}

struct SigmoidRange
{
  double s0    = 0.0;
  double s1    = 0.0;
  double range = 1.0;
};

inline SigmoidRange sigmoid_range(double steepness)
{
  SigmoidRange sr;
  sr.s0 = sigmoid(-steepness * 0.5);
  sr.s1 = sigmoid( steepness * 0.5);
  sr.range = (sr.s1 - sr.s0 > 0.0) ? sr.s1 - sr.s0 : 1.0;
  return sr;
}

inline double sigmoid_norm(double t, double steepness)
{
  auto sr = sigmoid_range(steepness);
  double val = sigmoid(steepness * (t - 0.5));
  return (val - sr.s0) / sr.range;
}

inline std::vector<double> sigmoid_norm_n(int n, double steepness)
{
  auto sr = sigmoid_range(steepness);
  std::vector<double> norm(n);
  for (int i = 0; i < n; ++i)
  {
    double t = (n > 1)
        ? static_cast<double>(i) / static_cast<double>(n - 1)
        : 1.0;
    double val = sigmoid(steepness * (t - 0.5));
    norm[i] = (val - sr.s0) / sr.range;
  }
  return norm;
}

// ?? Risk warp ?????????????????????????????????????????????????

inline double risk_warp(double norm, double risk)
{
  double r = clamp01(risk);
  return (1.0 - r) * norm + r * (1.0 - norm);
}

inline std::vector<double> risk_weights(
    const std::vector<double>& norms, double risk)
{
  std::vector<double> w(norms.size());
  for (size_t i = 0; i < norms.size(); ++i)
  {
    w[i] = risk_warp(norms[i], risk);
    if (w[i] < 1e-12) w[i] = 1e-12;
  }
  return w;
}

// ?? Allocation ????????????????????????????????????????????????

inline std::vector<double> norm_weights(
    const std::vector<double>& weights)
{
  double sum = 0.0;
  for (double w : weights) sum += w;
  std::vector<double> fracs(weights.size());
  for (size_t i = 0; i < weights.size(); ++i)
    fracs[i] = (sum > 0.0) ? weights[i] / sum : 0.0;
  return fracs;
}

inline std::vector<double> allocate(
    const std::vector<double>& weights, double total)
{
  auto fracs = norm_weights(weights);
  std::vector<double> alloc(fracs.size());
  for (size_t i = 0; i < fracs.size(); ++i)
    alloc[i] = fracs[i] * total;
  return alloc;
}

// ?? Interpolation ?????????????????????????????????????????????

inline double lerp(double low, double high, double t)
{
  return low + t * (high - low);
}

inline double sigmoid_lerp(
    double low, double high, int i, int n, double steepness)
{
  double t = sigmoid_norm(
      (n > 1) ? static_cast<double>(i) / static_cast<double>(n - 1)
              : 1.0,
      steepness);
  return lerp(low, high, t);
}

// ?? Hyperbolic compression ????????????????????????????????????

inline double hyperbolic_compress(double x)
{
  return (x > 0.0) ? x / (x + 1.0) : 0.0;
}

// ?? Fee overhead ??????????????????????????????????????????????

inline double overhead(
    double price, double quantity,
    double fee_spread, double fee_hedge, double delta_time,
    int symbol_count, double pump, double coeff_k,
    int future_trade_count = 0)
{
  double fee_component = fee_spread * fee_hedge * delta_time;
  double trade_scale =
      1.0 + static_cast<double>(std::max(0, future_trade_count));
  double numerator =
      fee_component * static_cast<double>(symbol_count) * trade_scale;
  double price_per_qty = (quantity > 0.0) ? price / quantity : 0.0;
  double denominator = price_per_qty * pump + coeff_k;
  return (denominator != 0.0) ? numerator / denominator : 0.0;
}

inline double effective_overhead(
    double raw_oh, double surplus_rate,
    double fee_spread, double fee_hedge, double delta_time)
{
  return raw_oh
       + surplus_rate * fee_hedge * delta_time
       + fee_spread * fee_hedge * delta_time;
}

// ?? Position metrics ??????????????????????????????????????????

inline double position_delta(
    double price, double quantity, double pump)
{
  return (pump > 0.0) ? (price * quantity) / pump : 0.0;
}

inline double break_even(double entry_price, double oh)
{
  return entry_price * (1.0 + oh);
}

inline double funded_qty(double price, double funds)
{
  return (price > 0.0 && funds > 0.0) ? funds / price : 0.0;
}

// ?? Profit ????????????????????????????????????????????????????

inline double gross_profit(
    double entry, double exit, double qty, bool is_short = false)
{
  return is_short ? (entry - exit) * qty : (exit - entry) * qty;
}

inline double net_profit(double gross, double buy_fee, double sell_fee)
{
  return gross - buy_fee - sell_fee;
}

inline double roi(double net, double cost)
{
  return (cost != 0.0) ? (net / cost) * 100.0 : 0.0;
}

// ?? Buffers ???????????????????????????????????????????????????

inline double sigmoid_buffer(
    double delta, double lower, double upper, int count)
{
  if (count <= 0 || delta <= 0.0)
    return 1.0;

  double t = hyperbolic_compress(delta);
  double alpha = (delta < 0.1) ? 0.1 : delta;

  auto sr = sigmoid_range(alpha);
  double val = sigmoid(alpha * (t - 0.5));
  double norm = (val - sr.s0) / sr.range;

  double per_cycle = lerp(lower, upper, norm);
  return 1.0 + static_cast<double>(count) * per_cycle;
}

inline double clamp_sl_fraction(
    double sl_frac, double eo,
    const std::vector<double>& fundings, double available_capital)
{
  if (sl_frac <= 0.0 || available_capital <= 0.0)
    return sl_frac;

  double total_exposure = 0.0;
  for (double f : fundings)
    total_exposure += eo * f;
  total_exposure *= sl_frac;

  if (total_exposure <= 0.0 || total_exposure <= available_capital)
    return sl_frac;

  double clamped = sl_frac * (available_capital / total_exposure);
  return clamp01(clamped);
}

// ?? Discount / gain ???????????????????????????????????????????

inline double discount(double ref_price, double entry_price)
{
  return (ref_price > 0.0)
       ? ((ref_price - entry_price) / ref_price) * 100.0
       : 0.0;
}

inline double pct_gain(double entry, double exit)
{
  return (entry > 0.0) ? ((exit - entry) / entry) * 100.0 : 0.0;
}

// ?? Savings ???????????????????????????????????????????????????

inline double savings(double profit, double rate)
{
  return profit * clamp01(rate);
}

inline double reinvest(double profit, double rate)
{
  return profit - savings(profit, rate);
}

// ?? Trade arithmetic ??????????????????????????????????????????

inline double cost(double price, double qty)
{
  return price * qty;
}

inline double proceeds(double price, double qty, double sell_fee)
{
  return price * qty - sell_fee;
}

inline double fee_from_rate(double notional, double rate)
{
  return notional * rate;
}

inline double avg_entry(double total_cost, double total_qty)
{
  return (total_qty > 0.0) ? total_cost / total_qty : 0.0;
}

inline double fee_hedging_coverage(double hedge_pool, double total_fees)
{
  return (total_fees > 0.0) ? hedge_pool / total_fees : 0.0;
}

// ?? Chain helpers ?????????????????????????????????????????????

inline int chain_future_trade_count(int total_cycles, int current_cycle)
{
  int nf = total_cycles - 1 - current_cycle;
  return (nf > 0) ? nf : 0;
}

// ?? Horizon factor ????????????????????????????????????????????

inline double horizon_factor(
    double raw_oh, double eo, double max_risk,
    double steepness, int level_index, int total_levels)
{
  if (max_risk > 0.0)
  {
    double min_f = raw_oh;
    double max_f = (max_risk > min_f) ? max_risk : min_f;
    double steep = (steepness > 0.0) ? steepness : 0.01;
    double t = (total_levels > 1)
        ? static_cast<double>(level_index)
        / static_cast<double>(total_levels - 1)
        : 1.0;
    double norm = sigmoid_norm(t, steep);
    return lerp(min_f, max_f, norm);
  }
  return eo * static_cast<double>(level_index + 1);
}

// ?? Per-level TP/SL ???????????????????????????????????????????

inline double level_tp_impl(
    double entry_price, double oh, double eo,
    double max_risk, double min_risk, double risk,
    bool is_short, double steepness,
    int level_index, int total_levels,
    double reference_price)
{
  if (max_risk <= 0.0)
    return 0.0;

  double r = clamp01(risk);
  double tp_ref = (reference_price > 0.0) ? reference_price : entry_price;
  double steep = (steepness > 0.1) ? steepness * 0.5 : 0.1;
  double t = static_cast<double>(level_index + 1)
           / static_cast<double>(total_levels + 1);
  double raw_norm = sigmoid_norm(t, steep);
  double norm = risk_warp(raw_norm, r);

  if (!is_short)
  {
    double min_tp = entry_price * (1.0 + eo + min_risk);
    double max_tp = tp_ref * (1.0 + max_risk);
    if (max_tp <= min_tp) return min_tp;
    return lerp(min_tp, max_tp, norm);
  }
  else
  {
    double max_tp = entry_price * (1.0 - eo - min_risk);
    if (max_tp < 0.0) max_tp = 0.0;
    double floor_tp = tp_ref * (1.0 - max_risk);
    if (floor_tp < 0.0) floor_tp = 0.0;
    if (floor_tp >= max_tp) return max_tp;
    return lerp(max_tp, floor_tp, norm);
  }
}

inline double level_tp(
    double entry_price, double oh, double eo,
    double max_risk, double min_risk, double risk,
    bool is_short, double steepness,
    int level_index, int total_levels,
    double reference_price)
{
  return level_tp_impl(
      entry_price, oh, eo, max_risk, min_risk, risk,
      is_short, steepness, level_index, total_levels,
      reference_price);
}

inline double level_sl(
    double entry_price, double eo, bool is_short)
{
  double sl = is_short ? entry_price * (1.0 + eo)
                       : entry_price * (1.0 - eo);
  return (sl < 0.0) ? 0.0 : sl;
}

// ?? High-level data types ?????????????????????????????????????

struct SerialParams
{
  double current_price           = 0.0;
  double quantity                = 1.0;
  int    levels                  = 4;
  double steepness               = 6.0;
  double risk                    = 0.5;
  bool   is_short                = false;
  double available_funds         = 0.0;

  double range_above             = 0.0;
  double range_below             = 0.0;

  double fee_spread              = 0.0;
  double fee_hedging_coefficient = 1.0;
  double delta_time              = 1.0;
  int    symbol_count            = 1;
  double coefficient_k           = 0.0;
  double surplus_rate            = 0.0;
  int    future_trade_count      = 0;

  double max_risk                = 0.0;
  double min_risk                = 0.0;

  bool   generate_stop_losses    = false;
  double stop_loss_fraction      = 1.0;
  int    stop_loss_hedge_count   = 0;

  int    downtrend_count         = 1;

  double savings_rate            = 0.0;
};

struct SerialEntry
{
  int    index        = 0;
  double entry_price  = 0.0;
  double break_even   = 0.0;
  double discount_pct = 0.0;
  double funding      = 0.0;
  double fund_frac    = 0.0;
  double fund_qty     = 0.0;
  double tp_unit      = 0.0;
  double tp_total     = 0.0;
  double tp_gross     = 0.0;
  double sl_unit      = 0.0;
  double sl_total     = 0.0;
  double sl_loss      = 0.0;
  double sl_qty       = 0.0;
  double effective_oh = 0.0;
};

struct SerialPlan
{
  double overhead_val     = 0.0;
  double effective_oh     = 0.0;
  double dt_buffer        = 1.0;
  double sl_buffer        = 1.0;
  double combined_buffer  = 1.0;
  double sl_fraction      = 1.0;
  double total_sl_loss    = 0.0;

  std::vector<SerialEntry> entries;

  double total_funding    = 0.0;
  double total_tp_gross   = 0.0;
};

struct CycleTrade
{
  int    index       = 0;
  double entry_price = 0.0;
  double qty         = 0.0;
  double funding     = 0.0;
  double buy_fee     = 0.0;
  double tp_price    = 0.0;
  double sell_fee    = 0.0;
  double revenue     = 0.0;
  double net         = 0.0;
};

struct CycleResult
{
  double total_cost       = 0.0;
  double total_revenue    = 0.0;
  double total_fees       = 0.0;
  double gross_profit_val = 0.0;
  double savings_amount   = 0.0;
  double reinvest_amount  = 0.0;
  double next_cycle_funds = 0.0;

  std::vector<CycleTrade> trades;
};

struct ChainCycle
{
  int         cycle   = 0;
  double      capital = 0.0;
  SerialPlan  plan;
  CycleResult result;
};

struct ChainResult
{
  std::vector<ChainCycle> cycles;
  double initial_overhead  = 0.0;
  double initial_effective = 0.0;
};

struct ExitPlanLevel
{
  int    index          = 0;
  double tp_price       = 0.0;
  double sell_qty       = 0.0;
  double sell_fraction  = 0.0;
  double sell_value     = 0.0;
  double gross_profit_val = 0.0;
  double cum_sold       = 0.0;
  double level_buy_fee  = 0.0;
  double net_profit_val = 0.0;
  double cum_net_profit = 0.0;
};

struct ExitPlan
{
  std::vector<ExitPlanLevel> levels;
};

struct ExitParams
{
  double entry_price       = 0.0;
  double quantity           = 0.0;
  double buy_fee            = 0.0;
  double raw_oh             = 0.0;
  double eo                 = 0.0;
  double max_risk           = 0.0;
  int    horizon_count      = 1;
  double risk_coefficient   = 0.0;
  double exit_fraction      = 1.0;
  double steepness          = 4.0;
};

struct ProfitOut
{
  double gross   = 0.0;
  double net     = 0.0;
  double roi_pct = 0.0;
};

// ?? generateSerialPlan ????????????????????????????????????????

inline SerialPlan generate_serial_plan(const SerialParams& sp)
{
  SerialPlan plan;
  int n = (sp.levels < 1) ? 1 : sp.levels;
  double steep = (sp.steepness < 0.1) ? 0.1 : sp.steepness;
  double rsk   = clamp01(sp.risk);

  plan.overhead_val = overhead(
      sp.current_price, sp.quantity,
      sp.fee_spread, sp.fee_hedging_coefficient, sp.delta_time,
      sp.symbol_count, sp.available_funds, sp.coefficient_k,
      sp.future_trade_count);
  plan.effective_oh = effective_overhead(
      plan.overhead_val, sp.surplus_rate,
      sp.fee_spread, sp.fee_hedging_coefficient, sp.delta_time);

  double delta = position_delta(
      sp.current_price, sp.quantity, sp.available_funds);
  double lower = sp.min_risk;
  double upper = (sp.max_risk > 0.0) ? sp.max_risk : plan.effective_oh;
  if (upper < lower) upper = lower;

  plan.dt_buffer = sigmoid_buffer(delta, lower, upper, sp.downtrend_count);
  plan.sl_fraction = clamp01(sp.stop_loss_fraction);
  plan.sl_buffer = sigmoid_buffer(
      delta, lower * plan.sl_fraction,
      upper * plan.sl_fraction, sp.stop_loss_hedge_count);
  plan.combined_buffer = plan.dt_buffer * plan.sl_buffer;

  double price_low, price_high;
  if (sp.range_above > 0.0 || sp.range_below > 0.0)
  {
    price_low  = floor_eps(sp.current_price - sp.range_below);
    price_high = sp.current_price + sp.range_above;
  }
  else
  {
    price_low  = 0.0;
    price_high = sp.current_price;
  }

  auto norm    = sigmoid_norm_n(n, steep);
  auto weights = risk_weights(norm, rsk);
  double w_sum = 0.0;
  for (double w : weights) w_sum += w;

  std::vector<double> fundings(n);
  for (int i = 0; i < n; ++i)
    fundings[i] = (w_sum > 0) ? sp.available_funds * weights[i] / w_sum : 0;

  if (sp.generate_stop_losses)
    plan.sl_fraction = clamp_sl_fraction(
        plan.sl_fraction, plan.effective_oh,
        fundings, sp.available_funds);

  plan.entries.resize(n);
  for (int i = 0; i < n; ++i)
  {
    auto& e = plan.entries[i];
    e.index        = i;
    e.entry_price  = floor_eps(lerp(price_low, price_high, norm[i]));
    e.break_even   = break_even(e.entry_price, plan.overhead_val);
    e.discount_pct = discount(sp.current_price, e.entry_price);
    e.fund_frac    = (w_sum > 0) ? weights[i] / w_sum : 0;
    e.funding      = sp.available_funds * e.fund_frac;
    e.fund_qty     = funded_qty(e.entry_price, e.funding);
    e.effective_oh = plan.effective_oh;

    e.tp_unit = level_tp_impl(
        e.entry_price, plan.overhead_val, plan.effective_oh,
        sp.max_risk, sp.min_risk, sp.risk,
        sp.is_short, steep, i, n, price_high);
    if (plan.combined_buffer > 1.0)
      e.tp_unit *= plan.combined_buffer;
    double entry_cost = e.entry_price * e.fund_qty;
    e.tp_total = e.tp_unit * e.fund_qty;
    e.tp_gross = e.tp_total - entry_cost;

    if (sp.generate_stop_losses)
    {
      e.sl_unit  = level_sl(e.entry_price, plan.effective_oh, sp.is_short);
      e.sl_qty   = e.fund_qty * plan.sl_fraction;
      e.sl_total = e.sl_unit * e.sl_qty;
      e.sl_loss  = e.sl_total - e.entry_price * e.sl_qty;
    }

    plan.total_funding += e.funding;
    plan.total_tp_gross += e.tp_gross;
    if (sp.generate_stop_losses)
      plan.total_sl_loss += std::abs(e.sl_loss);
  }

  return plan;
}

// ?? computeCycle ??????????????????????????????????????????????

inline CycleResult compute_cycle(
    const SerialPlan& plan, const SerialParams& sp)
{
  CycleResult cr;

  for (const auto& e : plan.entries)
  {
    if (e.funding <= 0) continue;

    double buy_fee_val  = e.funding * sp.fee_spread;
    double revenue      = e.tp_unit * e.fund_qty;
    double sell_fee_val = revenue * sp.fee_spread;
    double n            = revenue - e.funding - buy_fee_val - sell_fee_val;

    cr.total_cost    += e.funding;
    cr.total_revenue += revenue;
    cr.total_fees    += buy_fee_val + sell_fee_val;

    CycleTrade ct;
    ct.index       = e.index;
    ct.entry_price = e.entry_price;
    ct.qty         = e.fund_qty;
    ct.funding     = e.funding;
    ct.buy_fee     = buy_fee_val;
    ct.tp_price    = e.tp_unit;
    ct.sell_fee    = sell_fee_val;
    ct.revenue     = revenue;
    ct.net         = n;
    cr.trades.push_back(ct);
  }

  cr.gross_profit_val = cr.total_revenue - cr.total_cost - cr.total_fees;
  cr.savings_amount   = savings(cr.gross_profit_val, sp.savings_rate);
  cr.reinvest_amount  = cr.gross_profit_val - cr.savings_amount;
  cr.next_cycle_funds = sp.available_funds + cr.reinvest_amount;

  return cr;
}

// ?? generateChain ?????????????????????????????????????????????

inline ChainResult generate_chain(
    const SerialParams& sp, int total_cycles)
{
  ChainResult cr;
  if (total_cycles < 1) total_cycles = 1;

  auto plan0 = generate_serial_plan(sp);
  cr.initial_overhead  = plan0.overhead_val;
  cr.initial_effective = plan0.effective_oh;

  double capital = sp.available_funds;

  for (int ci = 0; ci < total_cycles; ++ci)
  {
    SerialParams csp = sp;
    csp.available_funds    = capital;
    csp.future_trade_count = chain_future_trade_count(total_cycles, ci);

    ChainCycle cc;
    cc.cycle   = ci;
    cc.capital = capital;
    cc.plan    = generate_serial_plan(csp);
    cc.result  = compute_cycle(cc.plan, csp);

    capital = cc.result.next_cycle_funds;
    cr.cycles.push_back(std::move(cc));
  }

  return cr;
}

// ?? generateExitPlan ??????????????????????????????????????????

inline ExitPlan generate_exit_plan(const ExitParams& ep)
{
  ExitPlan plan;
  int n = (ep.horizon_count < 1) ? 1 : ep.horizon_count;
  double frac  = clamp01(ep.exit_fraction);
  double rsk   = clamp01(ep.risk_coefficient);
  double steep = (ep.steepness > 0.0) ? ep.steepness : 0.01;

  double sellable_qty = ep.quantity * frac;

  double center = rsk * static_cast<double>(n - 1);
  std::vector<double> cum_sigma(n + 1);
  for (int i = 0; i <= n; ++i)
  {
    double x = static_cast<double>(i) - 0.5;
    cum_sigma[i] = sigmoid(steep * (x - center));
  }
  double lo = cum_sigma[0], hi = cum_sigma[n];
  for (int i = 0; i <= n; ++i)
    cum_sigma[i] = (hi > lo)
        ? (cum_sigma[i] - lo) / (hi - lo)
        : static_cast<double>(i) / static_cast<double>(n);

  plan.levels.reserve(n);
  double cum_sold = 0.0;
  double cum_net  = 0.0;

  for (int i = 0; i < n; ++i)
  {
    double factor = horizon_factor(
        ep.raw_oh, ep.eo, ep.max_risk, steep, i, n);

    ExitPlanLevel el;
    el.index           = i;
    el.tp_price        = ep.entry_price * (1.0 + factor);
    el.sell_fraction   = cum_sigma[i + 1] - cum_sigma[i];
    el.sell_qty        = sellable_qty * el.sell_fraction;
    el.sell_value      = el.tp_price * el.sell_qty;
    el.gross_profit_val = gross_profit(ep.entry_price, el.tp_price, el.sell_qty);
    el.level_buy_fee   = ep.buy_fee * el.sell_fraction;
    el.net_profit_val  = el.gross_profit_val - el.level_buy_fee;

    cum_sold += el.sell_qty;
    cum_net  += el.net_profit_val;
    el.cum_sold       = cum_sold;
    el.cum_net_profit = cum_net;

    plan.levels.push_back(el);
  }

  return plan;
}

// ?? computeProfit ?????????????????????????????????????????????

inline ProfitOut compute_profit(
    double entry_price, double exit_price,
    double qty, double buy_fee, double sell_fee,
    bool is_short = false)
{
  ProfitOut p;
  p.gross   = gross_profit(entry_price, exit_price, qty, is_short);
  p.net     = net_profit(p.gross, buy_fee, sell_fee);
  double c  = entry_price * qty + buy_fee;
  p.roi_pct = roi(p.net, c);
  return p;
}

// ?? DCA ???????????????????????????????????????????????????????

struct DcaResult
{
  double total_cost   = 0.0;
  double total_qty    = 0.0;
  double avg_price    = 0.0;
  double min_price    = 0.0;
  double max_price    = 0.0;
  double spread       = 0.0;
  int    count        = 0;
};

inline DcaResult compute_dca(
    const std::vector<std::pair<double, double>>& entries)
{
  DcaResult r;
  r.min_price = std::numeric_limits<double>::max();
  r.max_price = 0.0;

  for (const auto& [price, qty] : entries)
  {
    r.total_cost += cost(price, qty);
    r.total_qty  += qty;
    if (price < r.min_price) r.min_price = price;
    if (price > r.max_price) r.max_price = price;
    ++r.count;
  }

  r.avg_price = avg_entry(r.total_cost, r.total_qty);
  r.spread    = r.max_price - r.min_price;

  if (r.count == 0)
  {
    r.min_price = 0.0;
    r.max_price = 0.0;
  }

  return r;
}

}  // namespace internal

// ?? Public API ????????????????????????????????????????????????

//! \brief Quant trade-math engine
//!
//! \details Wraps the internal math primitives in a user-friendly
//!   static API for use from the ROOT prompt.
//!
//! \ingroup cpp_plugin_impl
//! \since docker-finance 1.1.0
class Quant final
{
 public:
  Quant() = default;
  ~Quant() = default;

  Quant(const Quant&) = delete;
  Quant& operator=(const Quant&) = delete;

  Quant(Quant&&) = default;
  Quant& operator=(Quant&&) = default;

 public:
  // ?? Serial plan ???????????????????????????????????????????

  static internal::SerialPlan serial_plan(const internal::SerialParams& sp)
  {
    return internal::generate_serial_plan(sp);
  }

  static void print_plan(const internal::SerialPlan& plan)
  {
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  Overhead    = " << (plan.overhead_val * 100) << "%\n"
              << "  Effective   = " << (plan.effective_oh * 100) << "%\n"
              << "  DT buffer   = " << plan.dt_buffer << "x\n"
              << "  SL buffer   = " << plan.sl_buffer << "x\n"
              << "  Combined    = " << plan.combined_buffer << "x\n"
              << "  SL fraction = " << plan.sl_fraction << "\n"
              << "  Funding     = " << plan.total_funding << "\n"
              << "  TP gross    = " << plan.total_tp_gross << "\n\n";

    std::cout << "  Lvl  Entry          Disc%    Qty            "
              << "Cost           TP/unit        TP Total";
    if (!plan.entries.empty() && plan.entries[0].sl_unit > 0)
      std::cout << "       SL/unit";
    std::cout << "\n";

    for (const auto& e : plan.entries)
    {
      double c = internal::cost(e.entry_price, e.fund_qty);
      std::cout << "  " << std::setw(3) << e.index
                << "  " << std::setw(14) << e.entry_price
                << "  " << std::setw(7) << e.discount_pct << "%"
                << "  " << std::setw(14) << e.fund_qty
                << "  " << std::setw(14) << c
                << "  " << std::setw(14) << e.tp_unit
                << "  " << std::setw(14) << e.tp_total;
      if (e.sl_unit > 0)
        std::cout << "  " << std::setw(14) << e.sl_unit;
      std::cout << "\n";
    }
  }

  // ?? Cycle computation ?????????????????????????????????????

  static internal::CycleResult cycle(
      const internal::SerialPlan& plan,
      const internal::SerialParams& sp)
  {
    return internal::compute_cycle(plan, sp);
  }

  static void print_cycle(const internal::CycleResult& cr)
  {
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  Cost     = " << cr.total_cost << "\n"
              << "  Revenue  = " << cr.total_revenue << "\n"
              << "  Fees     = " << cr.total_fees << "\n"
              << "  Gross    = " << cr.gross_profit_val << "\n"
              << "  Savings  = " << cr.savings_amount << "\n"
              << "  Reinvest = " << cr.reinvest_amount << "\n"
              << "  Next cap = " << cr.next_cycle_funds << "\n";
  }

  // ?? Chain planning ????????????????????????????????????????

  static internal::ChainResult chain(
      const internal::SerialParams& sp, int total_cycles)
  {
    return internal::generate_chain(sp, total_cycles);
  }

  static void print_chain(const internal::ChainResult& cr)
  {
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  Initial OH  = " << (cr.initial_overhead * 100) << "%\n"
              << "  Initial Eff = " << (cr.initial_effective * 100) << "%\n\n";

    for (const auto& cc : cr.cycles)
    {
      std::cout << "  Cycle " << cc.cycle
                << "  Capital=" << cc.capital
                << "  Profit=" << cc.result.gross_profit_val
                << "  Savings=" << cc.result.savings_amount
                << "  Next=" << cc.result.next_cycle_funds
                << "  (" << cc.plan.entries.size() << " levels)\n";
    }
  }

  // ?? Exit plan ?????????????????????????????????????????????

  static internal::ExitPlan exit_plan(const internal::ExitParams& ep)
  {
    return internal::generate_exit_plan(ep);
  }

  static void print_exit_plan(const internal::ExitPlan& plan)
  {
    std::cout << std::fixed << std::setprecision(6);
    for (const auto& el : plan.levels)
    {
      std::cout << "  [" << el.index << "]"
                << "  TP=" << el.tp_price
                << "  qty=" << el.sell_qty
                << " (" << (el.sell_fraction * 100) << "%)"
                << "  value=" << el.sell_value
                << "  net=" << el.net_profit_val
                << "  cum=" << el.cum_net_profit
                << "\n";
    }
  }

  // ?? Profit ????????????????????????????????????????????????

  static internal::ProfitOut profit(
      double entry, double exit, double qty,
      double buy_fee, double sell_fee, bool is_short = false)
  {
    return internal::compute_profit(
        entry, exit, qty, buy_fee, sell_fee, is_short);
  }

  static void print_profit(const internal::ProfitOut& p)
  {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Gross  = " << p.gross << "\n"
              << "  Net    = " << p.net << "\n"
              << "  ROI    = " << p.roi_pct << "%\n";
  }

  // ?? DCA ???????????????????????????????????????????????????

  static internal::DcaResult dca(
      const std::vector<std::pair<double, double>>& entries)
  {
    return internal::compute_dca(entries);
  }

  static void print_dca(const internal::DcaResult& r)
  {
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  Entries     = " << r.count << "\n"
              << "  Total cost  = " << r.total_cost << "\n"
              << "  Total qty   = " << r.total_qty << "\n"
              << "  Avg entry   = " << r.avg_price << "\n"
              << "  Min entry   = " << r.min_price << "\n"
              << "  Max entry   = " << r.max_price << "\n"
              << "  Spread      = " << r.spread << "\n";
  }

  // ?? Overhead (standalone) ?????????????????????????????????

  static double compute_overhead(
      double price, double quantity,
      double fee_spread, double fee_hedge, double delta_time,
      int symbol_count, double pump, double coeff_k,
      int future_trade_count = 0)
  {
    return internal::overhead(
        price, quantity, fee_spread, fee_hedge, delta_time,
        symbol_count, pump, coeff_k, future_trade_count);
  }

  static double compute_effective_overhead(
      double raw_oh, double surplus_rate,
      double fee_spread, double fee_hedge, double delta_time)
  {
    return internal::effective_overhead(
        raw_oh, surplus_rate, fee_spread, fee_hedge, delta_time);
  }

  static void print_overhead(
      double price, double quantity,
      double fee_spread, double fee_hedge, double delta_time,
      int symbol_count, double pump, double coeff_k,
      double surplus_rate)
  {
    double oh = internal::overhead(
        price, quantity, fee_spread, fee_hedge, delta_time,
        symbol_count, pump, coeff_k);
    double eo = internal::effective_overhead(
        oh, surplus_rate, fee_spread, fee_hedge, delta_time);
    double pd = internal::position_delta(price, quantity, pump);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  Overhead   = " << oh
              << "  (" << (oh * 100) << "%)\n"
              << "  Surplus    = " << surplus_rate
              << "  (" << (surplus_rate * 100) << "%)\n"
              << "  Effective  = " << eo
              << "  (" << (eo * 100) << "%)\n"
              << "  Pos delta  = " << pd
              << "  (" << (pd * 100) << "%)\n";
  }

  // ?? Convenience helpers ???????????????????????????????????

  static double break_even(double entry_price, double oh)
  {
    return internal::break_even(entry_price, oh);
  }

  static double discount(double ref_price, double entry_price)
  {
    return internal::discount(ref_price, entry_price);
  }

  static double pct_gain(double entry, double exit)
  {
    return internal::pct_gain(entry, exit);
  }

  static double avg_entry(double total_cost, double total_qty)
  {
    return internal::avg_entry(total_cost, total_qty);
  }

  static double level_tp(
      double entry_price, double oh, double eo,
      double max_risk, double min_risk, double risk,
      bool is_short, double steepness,
      int level_index, int total_levels,
      double reference_price)
  {
    return internal::level_tp(
        entry_price, oh, eo, max_risk, min_risk, risk,
        is_short, steepness, level_index, total_levels,
        reference_price);
  }

  static double level_sl(
      double entry_price, double eo, bool is_short)
  {
    return internal::level_sl(entry_price, eo, is_short);
  }
};

}  // namespace quant
}  // namespace plugin
}  // namespace dfi

// # vim: sw=2 sts=2 si ai et
