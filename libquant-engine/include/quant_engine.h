#ifndef QUANT_ENGINE_H
#define QUANT_ENGINE_H

// ============================================================
// quant_engine.h — C API facade for the Quant backend engine
//
// This is the sole entry point for external consumers (JNI,
// Python FFI, CLI tools).  All functions use C linkage and
// portable types from quant_engine_types.h.
//
// Lifecycle:
//   qe_open("path/to/db")  → returns opaque handle
//   qe_*() operations      → use the handle
//   qe_close(handle)       → cleanup
//
// Thread safety:
//   Each handle is independent.  Concurrent calls on the SAME
//   handle require external synchronisation.
// ============================================================

#include "quant_engine_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Opaque engine handle
typedef struct QEngine QEngine;

// ---- Lifecycle ----

QEngine*    qe_open(const char* dbDir);
void        qe_close(QEngine* e);

// ---- Wallet ----

QWalletInfo qe_wallet_info(QEngine* e);
void        qe_wallet_deposit(QEngine* e, double amount);
void        qe_wallet_withdraw(QEngine* e, double amount);

// ---- Trades ----

int         qe_trade_count(QEngine* e);
int         qe_trade_list(QEngine* e, QTrade* out, int maxCount);
int         qe_trade_add(QEngine* e, const QTrade* trade);
int         qe_trade_update(QEngine* e, const QTrade* trade);
int         qe_trade_delete(QEngine* e, int tradeId);
int         qe_trade_get(QEngine* e, int tradeId, QTrade* out);
double      qe_trade_sold_qty(QEngine* e, int parentTradeId);

// Execute buy: creates trade AND debits wallet. Returns trade ID.
int         qe_execute_buy(QEngine* e, const char* symbol,
                           double price, double qty, double buyFee);
// Execute sell: creates child sell AND credits wallet. Returns trade ID (-1 on failure).
int         qe_execute_sell(QEngine* e, const char* symbol,
                            double price, double qty, double sellFee);

// ---- Profit ----

QProfitResult qe_profit_calculate(QEngine* e, int tradeId,
                                  double currentPrice);

// ---- Entry Points ----

int         qe_entry_count(QEngine* e);
int         qe_entry_list(QEngine* e, QEntryPoint* out, int maxCount);
int         qe_entry_add(QEngine* e, const QEntryPoint* ep);
int         qe_entry_update(QEngine* e, const QEntryPoint* ep);
int         qe_entry_delete(QEngine* e, int entryId);
int         qe_entry_get(QEngine* e, int entryId, QEntryPoint* out);

// ---- Serial Generator ----

// Generate a serial plan.  Returns entry count.
// Caller provides output buffer; entries written to 'out'.
QSerialPlanSummary qe_serial_generate(const QSerialParams* params,
                                      QSerialEntry* out, int maxEntries);

// ---- Cycle / Chain Math ----

// Compute a single cycle result from a serial plan.
QCycleResult qe_cycle_compute(const QSerialParams* params);

// ---- Managed Chains ----

int         qe_chain_count(QEngine* e);
int         qe_chain_list(QEngine* e, QManagedChain* out, int maxCount);
int         qe_chain_create(QEngine* e, const QManagedChain* chain);
int         qe_chain_update(QEngine* e, const QManagedChain* chain);
int         qe_chain_delete(QEngine* e, int chainId);
int         qe_chain_get(QEngine* e, int chainId, QManagedChain* out);
int         qe_chain_activate(QEngine* e, int chainId);
int         qe_chain_deactivate(QEngine* e, int chainId);

// Add entry IDs to a cycle within a chain.
int         qe_chain_add_entries(QEngine* e, int chainId, int cycle,
                                 const int* entryIds, int count);
// Remove an entry from a cycle.
int         qe_chain_remove_entry(QEngine* e, int chainId, int cycle,
                                  int entryId);

// ---- Exit Strategy (pure math, no DB) ----

typedef struct {
    int         index;
    double      tpPrice;
    double      sellQty;
    double      sellFraction;
    double      sellValue;
    double      grossProfit;
} QExitLevel;

// Generate exit plan for a single entry.
int         qe_exit_generate(double entryPrice, double quantity,
                             double buyFee, double rawOH, double eo,
                             double maxRisk, int horizonCount,
                             double riskCoefficient, double exitFraction,
                             double steepness,
                             QExitLevel* out, int maxLevels);

// ---- Overhead (pure math, no DB) ----

double      qe_overhead(double price, double qty, double feeSpread,
                        double feeHedging, double deltaTime,
                        int symbolCount, double availableFunds,
                        double coefficientK, int futureTradeCount);

double      qe_effective_overhead(double overhead, double surplusRate,
                                   double feeSpread, double feeHedging,
                                   double deltaTime);

// ---- Exit Points (per-trade exit strategy, DB-backed) ----

int         qe_exitpt_count(QEngine* e);
int         qe_exitpt_list(QEngine* e, QExitPointData* out, int maxCount);
int         qe_exitpt_list_for_trade(QEngine* e, int tradeId,
                                     QExitPointData* out, int maxCount);
int         qe_exitpt_add(QEngine* e, QExitPointData* ep);  // sets ep->exitId
int         qe_exitpt_update(QEngine* e, const QExitPointData* ep);
int         qe_exitpt_delete(QEngine* e, int exitId);

#ifdef __cplusplus
}
#endif

#endif // QUANT_ENGINE_H
