#pragma once

#include "MultiHorizonEngine.h"
#include <vector>
#include <cmath>
#include <limits>

// Sigmoid-distributed entry price levels.
//
// When rangeAbove / rangeBelow are both 0 (default):
//   Level 0       = near 0  (deepest discount)
//   Level N-1     = currentPrice (shallowest, at market)
//
// When rangeAbove or rangeBelow > 0:
//   Level 0       = currentPrice - rangeBelow
//   Level N-1     = currentPrice + rangeAbove
//   Levels are sigmoid-interpolated between these bounds.
//   Higher price levels receive less funding (controlled by risk).
//
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
                                            double rangeBelow = 0.0)
    {
        double oh = MultiHorizonEngine::computeOverhead(currentPrice, quantity, p);

        int N = p.horizonCount;
        if (N < 1) N = 1;
        if (steepness < 0.1) steepness = 0.1;

        double risk = (riskCoefficient < 0.0) ? 0.0
                    : (riskCoefficient > 1.0) ? 1.0
                    : riskCoefficient;

        // sigmoid helpers
        auto sigmoid = [](double x) { return 1.0 / (1.0 + std::exp(-x)); };
        double sig0 = sigmoid(-steepness * 0.5);
        double sig1 = sigmoid( steepness * 0.5);
        double sigRange = (sig1 - sig0 > 0.0) ? sig1 - sig0 : 1.0;

        // entry price range
        double priceLow, priceHigh;
        if (rangeAbove > 0.0 || rangeBelow > 0.0)
        {
            priceLow  = currentPrice - rangeBelow;
            if (priceLow < std::numeric_limits<double>::epsilon())
                priceLow = std::numeric_limits<double>::epsilon();
            priceHigh = currentPrice + rangeAbove;
        }
        else
        {
            priceLow  = 0.0;
            priceHigh = currentPrice;
        }

        // first pass: compute sigmoid-normalized values for each level
        std::vector<double> norm(N);
        for (int i = 0; i < N; ++i)
        {
            double t = (N > 1) ? static_cast<double>(i) / static_cast<double>(N - 1)
                               : 1.0;
            double sigVal = sigmoid(steepness * (t - 0.5));
            norm[i] = (sigVal - sig0) / sigRange;
        }

        // inverse-sigmoid funding warped by risk:
        //   risk=0   -> sigmoid weights (more funding near current price)
        //   risk=0.5 -> uniform
        //   risk=1   -> inverse sigmoid (more funding at deep discounts)
        std::vector<double> weights(N);
        double weightSum = 0.0;
        for (int i = 0; i < N; ++i)
        {
            weights[i] = (1.0 - risk) * norm[i] + risk * (1.0 - norm[i]);
            if (weights[i] < 1e-12) weights[i] = 1e-12;
            weightSum += weights[i];
        }

        std::vector<EntryLevel> levels;
        levels.reserve(N);

        for (int i = 0; i < N; ++i)
        {
            EntryLevel el;
            el.index        = i;
            el.costCoverage = static_cast<double>(i + 1);
            el.entryPrice   = priceLow + norm[i] * (priceHigh - priceLow);

            if (el.entryPrice < std::numeric_limits<double>::epsilon())
                el.entryPrice = std::numeric_limits<double>::epsilon();

            el.breakEven    = el.entryPrice * (1.0 + oh);

            el.fundingFraction = (weightSum != 0.0) ? weights[i] / weightSum : 0.0;
            el.funding         = p.portfolioPump * el.fundingFraction;
            el.fundingQty      = (el.entryPrice > 0.0) ? el.funding / el.entryPrice : 0.0;

            el.potentialNet = (currentPrice - el.entryPrice) * el.fundingQty;

            levels.push_back(el);
        }

        return levels;
    }
};
