#pragma once

#include "Trade.h"
#include "QuantMath.h"

struct ProfitResult
{
    double grossProfit = 0.0;   // before fees
    double netProfit   = 0.0;   // after sell fees
    double roi         = 0.0;   // return on investment (%)
};

class ProfitCalculator
{
public:
    static ProfitResult calculate(const Trade& trade, double currentPrice,
                                  double buyFees, double sellFees)
    {
        return calculateForQty(trade, currentPrice, trade.quantity, buyFees, sellFees);
    }

    // Calculate on a specific quantity (e.g. remaining after partial sells)
    static ProfitResult calculateForQty(const Trade& trade, double currentPrice,
                                         double qty, double buyFees, double sellFees)
    {
        ProfitResult r{};
        bool isShort = (trade.type != TradeType::Buy);
        r.grossProfit = QuantMath::grossProfit(trade.value, currentPrice, qty, isShort);
        r.netProfit = QuantMath::netProfit(r.grossProfit, buyFees, sellFees);
        double cost = trade.value * qty + buyFees;
        r.roi = QuantMath::roi(r.netProfit, cost);
        return r;
    }

    // Overload using stored fees from the trade
    static ProfitResult calculate(const Trade& trade, double currentPrice)
    {
        return calculate(trade, currentPrice, trade.buyFee, trade.sellFee);
    }

    // Realized profit for a child sell against a parent buy entry
    static ProfitResult childProfit(const Trade& sell, double parentEntry)
    {
        ProfitResult r{};
        r.grossProfit = QuantMath::grossProfit(parentEntry, sell.value, sell.quantity);
        r.netProfit   = QuantMath::netProfit(r.grossProfit, sell.buyFee, sell.sellFee);
        double cost   = parentEntry * sell.quantity + sell.buyFee;
        r.roi = QuantMath::roi(r.netProfit, cost);
        return r;
    }
};
