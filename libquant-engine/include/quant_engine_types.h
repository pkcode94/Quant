#ifndef QUANT_ENGINE_TYPES_H
#define QUANT_ENGINE_TYPES_H

// ============================================================
// quant_engine_types.h — Portable data types for the engine API
//
// These mirror the internal C++ types but use C-compatible
// layout so JNI and other FFI layers can work with them.
// ============================================================

#ifdef __cplusplus
extern "C" {
#endif

// ---- Trade ----

enum QTradeType { Q_BUY = 0, Q_COVERED_SELL = 1 };

typedef struct {
    int         tradeId;
    char        symbol[64];
    int         type;           // QTradeType
    double      value;          // price per unit
    double      quantity;
    long long   timestamp;
    double      buyFees;
    double      sellFees;
} QTrade;

// ---- Entry Point ----

typedef struct {
    int         entryId;
    char        symbol[64];
    int         levelIndex;
    double      entryPrice;
    double      breakEven;
    double      funding;
    double      fundingQty;
    double      effectiveOverhead;
    int         isShort;
    int         traded;
    int         linkedTradeId;
    double      exitTakeProfit;
    double      exitStopLoss;
    double      stopLossFraction;
} QEntryPoint;

// ---- Serial Plan (summary) ----

typedef struct {
    int         index;
    double      entryPrice;
    double      breakEven;
    double      discountPct;
    double      funding;
    double      fundQty;
    double      tpUnit;
    double      tpTotal;
    double      tpGross;
    double      slUnit;
    double      slLoss;
} QSerialEntry;

typedef struct {
    double      overhead;
    double      effectiveOH;
    double      totalFunding;
    double      totalTpGross;
    int         entryCount;
} QSerialPlanSummary;

// ---- Serial Params (input) ----

typedef struct {
    double      currentPrice;
    double      quantity;
    int         levels;
    int         exitLevels;
    double      steepness;
    double      risk;
    int         isShort;
    double      availableFunds;
    double      rangeAbove;
    double      rangeBelow;
    double      rangeAbovePerDt;
    double      rangeBelowPerDt;
    int         autoRange;
    double      feeSpread;
    double      feeHedgingCoefficient;
    double      deltaTime;
    int         symbolCount;
    double      coefficientK;
    double      surplusRate;
    int         futureTradeCount;
    double      maxRisk;
    double      minRisk;
    double      exitRisk;
    double      exitFraction;
    double      exitSteepness;
    int         generateStopLosses;
    double      stopLossFraction;
    int         stopLossHedgeCount;
    int         downtrendCount;
    double      savingsRate;
} QSerialParams;

// ---- Chain ----

typedef struct {
    int         chainId;
    char        name[128];
    char        symbol[64];
    int         active;
    int         theoretical;
    int         currentCycle;
    double      capital;
    double      totalSavings;
    double      savingsRate;
} QManagedChain;

// ---- Cycle Result ----

typedef struct {
    double      totalCost;
    double      totalRevenue;
    double      totalFees;
    double      grossProfit;
    double      savingsAmount;
    double      reinvestAmount;
    double      nextCycleFunds;
} QCycleResult;

// ---- Wallet ----

typedef struct {
    double      balance;
    double      deployed;
    double      total;
} QWalletInfo;

// ---- Profit ----

typedef struct {
    double      grossProfit;
    double      netProfit;
    double      roi;
} QProfitResult;

// ---- Exit Point (per-trade exit strategy level) ----

typedef struct {
    int         exitId;
    int         tradeId;        // parent Buy trade
    char        symbol[64];
    int         levelIndex;
    double      tpPrice;        // per-unit take-profit price
    double      slPrice;        // per-unit stop-loss price
    double      sellQty;        // quantity to sell at this level
    double      sellFraction;   // fraction of position (0-1)
    int         slActive;
    int         executed;
    int         linkedSellId;   // trade ID of the sell, if executed
} QExitPointData;

#ifdef __cplusplus
}
#endif

#endif // QUANT_ENGINE_TYPES_H
