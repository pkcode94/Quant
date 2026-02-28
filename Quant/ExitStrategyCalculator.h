#pragma once

#include "MultiHorizonEngine.h"
#include "ProfitCalculator.h"
#include <vector>
#include <cmath>

// Exit strategy: distributes a sigmoidal fraction of one trade's holdings
// across TP levels as pending sell orders.
//
//   effective = overhead + surplusRate
//   TP_price[i] = entry * (1 + effective * (i + 1))
//
// exitFraction in [0, 1]: total fraction of holdings to sell (e.g. 0.5 = 50%)
// steepness: controls sigmoid curve shape (0=uniform, 4=smooth S, 10+=step)
// riskCoefficient: shifts sigmoid center
//   0 = conservative -> sell most at early TP levels (lock in gains)
//   1 = aggressive   -> hold most for deeper TP levels (maximize upside)
//
// Cumulative sold follows a logistic sigmoid:
//   cumFrac(i) = sigmoid(steepness * (i - center))  normalized to [0,1]
//   sellQty[i] = sellableQty * (cumFrac[i+1] - cumFrac[i])

struct ExitLevel
{
    // -- Computed at generation time --
    int    index          = 0;
    double tpPrice        = 0.0;   // take-profit price per unit at this level
    double sellQty        = 0.0;   // quantity to sell at this level
    double sellFraction   = 0.0;   // fraction of sellable position (0-1)
    double sellValue      = 0.0;   // gross proceeds = tpPrice * sellQty
    double grossProfit    = 0.0;   // (tpPrice - entry) * sellQty
    double cumSold        = 0.0;   // cumulative quantity sold through this level

    // -- Per-level fees (filled progressively) --
    double levelBuyFee    = 0.0;   // amortized buy fee for this tranche (auto-set at generation)
    double levelSellFee   = 0.0;   // actual sell fee (set at execution time)
    double netProfit      = 0.0;   // grossProfit - levelBuyFee - levelSellFee
    double cumNetProfit   = 0.0;   // cumulative net profit locked in
};

class ExitStrategyCalculator
{
public:
    static std::vector<ExitLevel> generate(const Trade& trade,
                                           const HorizonParams& p,
                                           double riskCoefficient = 0.0,
                                           double exitFraction = 1.0,
                                           double steepness = 4.0)
    {
        QuantMath::ExitParams ep;
        ep.entryPrice      = trade.value;
        ep.quantity         = trade.quantity;
        ep.buyFee           = trade.buyFee;
        ep.rawOH            = MultiHorizonEngine::computeOverhead(trade, p);
        ep.eo               = MultiHorizonEngine::effectiveOverhead(trade, p);
        ep.maxRisk          = p.maxRisk;
        ep.horizonCount     = p.horizonCount;
        ep.riskCoefficient  = riskCoefficient;
        ep.exitFraction     = exitFraction;
        ep.steepness        = steepness;

        auto plan = QuantMath::generateExitPlan(ep);

        std::vector<ExitLevel> levels;
        levels.reserve(plan.levels.size());
        for (const auto& pl : plan.levels)
        {
            ExitLevel el;
            el.index        = pl.index;
            el.tpPrice      = pl.tpPrice;
            el.sellQty      = pl.sellQty;
            el.sellFraction = pl.sellFraction;
            el.sellValue    = pl.sellValue;
            el.grossProfit  = pl.grossProfit;
            el.cumSold      = pl.cumSold;
            el.levelBuyFee  = pl.levelBuyFee;
            el.levelSellFee = 0.0;
            el.netProfit    = pl.netProfit;
            el.cumNetProfit = pl.cumNetProfit;
            levels.push_back(el);
        }
        return levels;
    }

    // Recompute netProfit and cumNetProfit after per-level fees are updated.
    static void applyFees(std::vector<ExitLevel>& levels)
    {
        double cumNet = 0.0;
        for (auto& el : levels)
        {
            el.netProfit = el.grossProfit - el.levelBuyFee - el.levelSellFee;
            cumNet += el.netProfit;
            el.cumNetProfit = cumNet;
        }
    }
};
