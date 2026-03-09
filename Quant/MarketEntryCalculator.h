#pragma once

#include "MultiHorizonEngine.h"
#include <vector>
#include <cmath>
#include <limits>

// Sigmoid-distributed entry price levels.
//
// When rangeAbove / rangeBelow are both 0 (default):
//   autoRange=false: Level 0 = near 0, Level N-1 = currentPrice
//   autoRange=true:  [price × (1 - 3×EO), price]  (heuristic)
//   adaptiveRange=true: [currentPrice, maxHistoricalPrice] (default, enabled)
//
// When rangeAbove or rangeBelow > 0:
//   Level 0       = currentPrice - rangeBelow
//   Level N-1     = currentPrice + rangeAbove
//   Levels are sigmoid-interpolated between these bounds.
//   Higher price levels receive less funding (controlled by risk).
//
// ADAPTIVE RANGE (default):
//   When adaptiveRange=true:
//   - rangeBelow is set to 0 (entries start at current price)
//   - rangeAbove is set to max(historicalEntryPrice - currentPrice)
//   - This creates entries from current up to historical highs
//   - Ignores manually specified rangeAbove/rangeBelow values
//
// LONG positions:
//   overhead  = computeOverhead(price, qty, params)
//   BreakEven = entryPrice * (1 + overhead)
//
// Funding allocation (portfolioPump = total funds):
//   riskCoefficient in [0, 1] warps a sigmoid funding curve:
//     0 = conservative -> sigmoid weights (more funds at higher prices)
//     0.5 = uniform distribution
//     1 = aggressive   -> inverse sigmoid (more funds at lower prices)
//   weight[i] = (1 - risk) * norm[i] + risk * (1 - norm[i])
//   allocation[i] = weight[i] / sum(weights) * totalFunds

struct EntryLevel
{
    int    index           = 0;
    double entryPrice      = 0.0;   // suggested entry price per unit
    double breakEven       = 0.0;   // price needed to break even after costs
    double costCoverage    = 0.0;   // how many layers of overhead are covered
    double potentialNet    = 0.0;   // net profit if price returns to currentPrice
    double funding         = 0.0;   // allocated funds for this level
    double fundingFraction = 0.0;   // fraction of total funds (0-1)
    double fundingQty      = 0.0;   // how many units this funding buys at entryPrice
};

class MarketEntryCalculator
{
public:
    static std::vector<EntryLevel> generate(double currentPrice,
                                            double quantity,
                                            const HorizonParams& p,
                                            double riskCoefficient = 0.0,
                                            double steepness = 6.0,
                                            double rangeAbove = 0.0,
                                            double rangeBelow = 0.0,
                                            bool   autoRange = false)
    {
        double oh = MultiHorizonEngine::computeOverhead(currentPrice, quantity, p);

        int N = p.horizonCount;
        if (N < 1) N = 1;
        if (steepness < 0.1) steepness = 0.1;

        double risk = QuantMath::clamp01(riskCoefficient);

        // sigmoid helpers
        auto norm = QuantMath::sigmoidNormN(N, steepness);

        // entry price range
        double priceLow, priceHigh;
        if (rangeAbove > 0.0 || rangeBelow > 0.0)
        {
            priceLow  = currentPrice - rangeBelow;
            priceLow  = QuantMath::floorEps(priceLow);
            priceHigh = currentPrice + rangeAbove;
        }
        else if (autoRange)
        {
            double eo = QuantMath::effectiveOverhead(oh,
                p.surplusRate, p.feeSpread,
                p.feeHedgingCoefficient, p.deltaTime);
            double band = eo * 3.0;
            if (band < 0.01) band = 0.01;
            if (band > 0.99) band = 0.99;
            priceLow  = QuantMath::floorEps(currentPrice * (1.0 - band));
            priceHigh = currentPrice;
        }
        else
        {
            priceLow  = 0.0;
            priceHigh = currentPrice;
        }

        // inverse-sigmoid funding warped by risk:
        //   risk=0   -> sigmoid weights (more funding near current price)
        //   risk=0.5 -> uniform
        //   risk=1   -> inverse sigmoid (more funding at deep discounts)
        auto weights = QuantMath::riskWeights(norm, risk);
        double weightSum = 0.0;
        for (double w : weights) weightSum += w;

        std::vector<EntryLevel> levels;
        levels.reserve(N);

        for (int i = 0; i < N; ++i)
        {
            EntryLevel el;
            el.index        = i;
            el.costCoverage = static_cast<double>(i + 1);
            el.entryPrice   = QuantMath::lerp(priceLow, priceHigh, norm[i]);
            el.entryPrice   = QuantMath::floorEps(el.entryPrice);

            el.breakEven    = QuantMath::breakEven(el.entryPrice, oh);

            el.fundingFraction = (weightSum != 0.0) ? weights[i] / weightSum : 0.0;
            el.funding         = p.portfolioPump * el.fundingFraction;
            el.fundingQty      = QuantMath::fundedQty(el.entryPrice, el.funding);

            el.potentialNet = QuantMath::grossProfit(el.entryPrice, currentPrice, el.fundingQty);

            levels.push_back(el);
        }

        return levels;
    }
};
