#pragma once

#include <string>
#include <stdexcept>
#include <algorithm>
#include <cctype>

enum class TradeType
{
    Buy,
    CoveredSell   // asset-backed sell only, no short trades
};

inline std::string normalizeSymbol(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return s;
}

struct Trade
{
    std::string symbol;
    int         tradeId       = 0;
    TradeType   type          = TradeType::Buy;
    double      value         = 0.0;   // entry price per unit
    double      quantity      = 0.0;
    int         parentTradeId = -1;    // -1 = no parent (Buy); >= 0 = parent Buy trade id (CoveredSell)

    // Fees paid at execution
    double buyFee          = 0.0;
    double sellFee         = 0.0;
    //
    // Take-profit / stop-loss (meaningful for Buy trades only)
    double takeProfit          = 0.0;
    double stopLoss            = 0.0;
    double takeProfitFraction   = 0.0;  // 0=TP off, 0.5=sell 50%, 1.0=sell 100%
    double stopLossFraction     = 0.0;  // 0=SL off, 0.5=sell 50%, 1.0=sell 100%
    bool   takeProfitActive     = false; // derived: takeProfitFraction > 0
    bool   stopLossActive       = false; // derived: stopLossFraction > 0
    bool   shortEnabled         = false; // short trades disabled by default

    // Timestamp (unix seconds, 0 = not set)
    long long timestamp    = 0;

    Trade() = default;

    bool isChild()  const { return parentTradeId >= 0; }
    bool isParent() const { return type == TradeType::Buy && parentTradeId < 0; }

    void setTakeProfit(double tp, double fraction = 1.0)
    {
        if (type != TradeType::Buy)
            throw std::logic_error("Take-profit is only supported for Buy trades");
        takeProfit = tp;
        takeProfitFraction = fraction;
        takeProfitActive = (takeProfitFraction > 0.0);
    }

    void setStopLoss(double sl, double fraction = 1.0)
    {
        if (type != TradeType::Buy)
            throw std::logic_error("Stop-loss is only supported for Buy trades");
        stopLoss         = sl;
        stopLossFraction = fraction;
        stopLossActive   = (stopLossFraction > 0.0);
    }
};
