// ============================================================
// quant_engine.cpp — Implementation of the C API facade
//
// Wraps TradeDatabase, QuantMath, ProfitCalculator, etc.
// into the C-linkage API defined in quant_engine.h.
// ============================================================

#ifndef QUANT_ENGINE_H
#include "quant_engine.h"
#endif

// Internal C++ headers from Quant/
#include "Trade.h"
#include "TradeDatabase.h"
#include "ProfitCalculator.h"
#include "QuantMath.h"

#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

// ---- Opaque handle ----

struct QEngine {
    TradeDatabase db;
    QEngine(const std::string& dir) : db(dir) {}
};

// ---- Helpers ----

#ifndef QUANT_ENGINE_COPYSTR_DEFINED
#define QUANT_ENGINE_COPYSTR_DEFINED
static void copyStr(char* dst, size_t dstSize, const std::string& src)
{
    size_t len = (src.size() < dstSize - 1) ? src.size() : dstSize - 1;
    std::memcpy(dst, src.c_str(), len);
    dst[len] = '\0';
}
#endif

static QTrade toC(const Trade& t)
{
    QTrade q{};
    q.tradeId   = t.tradeId;
    copyStr(q.symbol, sizeof(q.symbol), t.symbol);
    q.type      = (t.type == TradeType::Buy) ? Q_BUY : Q_COVERED_SELL;
    q.value     = t.value;
    q.quantity  = t.quantity;
    q.timestamp = t.timestamp;
    q.buyFees   = t.buyFee;
    q.sellFees  = t.sellFee;
    return q;
}

static Trade fromC(const QTrade& q)
{
    Trade t;
    t.tradeId   = q.tradeId;
    t.symbol    = q.symbol;
    t.type      = (q.type == Q_BUY) ? TradeType::Buy : TradeType::CoveredSell;
    t.value     = q.value;
    t.quantity  = q.quantity;
    t.timestamp = q.timestamp;
    t.buyFee    = q.buyFees;
    t.sellFee   = q.sellFees;
    return t;
}

static QEntryPoint toCEntry(const TradeDatabase::EntryPoint& ep)
{
    QEntryPoint q{};
    q.entryId           = ep.entryId;
    copyStr(q.symbol, sizeof(q.symbol), ep.symbol);
    q.levelIndex        = ep.levelIndex;
    q.entryPrice        = ep.entryPrice;
    q.breakEven         = ep.breakEven;
    q.funding           = ep.funding;
    q.fundingQty        = ep.fundingQty;
    q.effectiveOverhead = ep.effectiveOverhead;
    q.isShort           = ep.isShort ? 1 : 0;
    q.traded            = ep.traded ? 1 : 0;
    q.linkedTradeId     = ep.linkedTradeId;
    q.exitTakeProfit    = ep.exitTakeProfit;
    q.exitStopLoss      = ep.exitStopLoss;
    q.stopLossFraction  = ep.stopLossFraction;
    return q;
}

static TradeDatabase::EntryPoint fromCEntry(const QEntryPoint& q)
{
    TradeDatabase::EntryPoint ep;
    ep.entryId           = q.entryId;
    ep.symbol            = q.symbol;
    ep.levelIndex        = q.levelIndex;
    ep.entryPrice        = q.entryPrice;
    ep.breakEven         = q.breakEven;
    ep.funding           = q.funding;
    ep.fundingQty        = q.fundingQty;
    ep.effectiveOverhead = q.effectiveOverhead;
    ep.isShort           = (q.isShort != 0);
    ep.traded            = (q.traded != 0);
    ep.linkedTradeId     = q.linkedTradeId;
    ep.exitTakeProfit    = q.exitTakeProfit;
    ep.exitStopLoss      = q.exitStopLoss;
    ep.stopLossFraction  = q.stopLossFraction;
    ep.stopLossActive    = (q.stopLossFraction > 0.0);
    return ep;
}

static QManagedChain toCChain(const TradeDatabase::ManagedChain& c)
{
    QManagedChain q{};
    q.chainId      = c.chainId;
    copyStr(q.name,   sizeof(q.name),   c.name);
    copyStr(q.symbol, sizeof(q.symbol), c.symbol);
    q.active       = c.active ? 1 : 0;
    q.theoretical  = c.theoretical ? 1 : 0;
    q.currentCycle = c.currentCycle;
    q.capital      = c.capital;
    q.totalSavings = c.totalSavings;
    q.savingsRate  = c.savingsRate;
    return q;
}

static QuantMath::SerialParams fromCParams(const QSerialParams& p)
{
    QuantMath::SerialParams sp;
    sp.currentPrice          = p.currentPrice;
    sp.quantity              = p.quantity;
    sp.levels                = p.levels;
    sp.exitLevels            = p.exitLevels;
    sp.steepness             = p.steepness;
    sp.risk                  = p.risk;
    sp.isShort               = (p.isShort != 0);
    sp.availableFunds        = p.availableFunds;
    sp.rangeAbove            = p.rangeAbove;
    sp.rangeBelow            = p.rangeBelow;
    sp.rangeAbovePerDt       = p.rangeAbovePerDt;
    sp.rangeBelowPerDt       = p.rangeBelowPerDt;
    sp.autoRange             = (p.autoRange != 0);
    sp.feeSpread             = p.feeSpread;
    sp.feeHedgingCoefficient = p.feeHedgingCoefficient;
    sp.deltaTime             = p.deltaTime;
    sp.symbolCount           = p.symbolCount;
    sp.coefficientK          = p.coefficientK;
    sp.surplusRate           = p.surplusRate;
    sp.futureTradeCount      = p.futureTradeCount;
    sp.maxRisk               = p.maxRisk;
    sp.minRisk               = p.minRisk;
    sp.exitRisk              = p.exitRisk;
    sp.exitFraction          = p.exitFraction;
    sp.exitSteepness         = p.exitSteepness;
    sp.generateStopLosses    = (p.generateStopLosses != 0);
    sp.stopLossFraction      = p.stopLossFraction;
    sp.stopLossHedgeCount    = p.stopLossHedgeCount;
    sp.downtrendCount        = p.downtrendCount;
    sp.savingsRate           = p.savingsRate;
    return sp;
}

// ============================================================
// API implementation
// ============================================================

extern "C" {

// ---- Lifecycle ----

QEngine* qe_open(const char* dbDir)
{
    return new QEngine(dbDir ? dbDir : "db");
}

void qe_close(QEngine* e)
{
    delete e;
}

// ---- Wallet ----

QWalletInfo qe_wallet_info(QEngine* e)
{
    QWalletInfo w{};
    w.balance  = e->db.loadWalletBalance();
    w.deployed = e->db.deployedCapital();
    w.total    = w.balance + w.deployed;
    return w;
}

void qe_wallet_deposit(QEngine* e, double amount)
{
    e->db.deposit(amount);
}

void qe_wallet_withdraw(QEngine* e, double amount)
{
    e->db.withdraw(amount);
}

// ---- Trades ----

int qe_trade_count(QEngine* e)
{
    return static_cast<int>(e->db.loadTrades().size());
}

int qe_trade_list(QEngine* e, QTrade* out, int maxCount)
{
    auto trades = e->db.loadTrades();
    int n = std::min(static_cast<int>(trades.size()), maxCount);
    for (int i = 0; i < n; ++i)
        out[i] = toC(trades[i]);
    return n;
}

int qe_trade_add(QEngine* e, const QTrade* trade)
{
    Trade t = fromC(*trade);
    t.tradeId = e->db.nextTradeId();
    e->db.addTrade(t);
    return t.tradeId;
}

int qe_trade_delete(QEngine* e, int tradeId)
{
    auto trades = e->db.loadTrades();
    auto it = std::remove_if(trades.begin(), trades.end(),
        [tradeId](const Trade& t) { return t.tradeId == tradeId; });
    if (it == trades.end()) return 0;
    trades.erase(it, trades.end());
    e->db.saveTrades(trades);
    return 1;
}

int qe_trade_get(QEngine* e, int tradeId, QTrade* out)
{
    auto trades = e->db.loadTrades();
    for (const auto& t : trades)
        if (t.tradeId == tradeId) { *out = toC(t); return 1; }
    return 0;
}

int qe_trade_update(QEngine* e, const QTrade* trade)
{
    Trade t = fromC(*trade);
    e->db.updateTrade(t);
    return 1;
}

double qe_trade_sold_qty(QEngine* e, int parentTradeId)
{
    return e->db.soldQuantityForParent(parentTradeId);
}

int qe_execute_buy(QEngine* e, const char* symbol,
                   double price, double qty, double buyFee)
{
    double walBal = e->db.loadWalletBalance();
    double needed = price * qty + buyFee;
    if (needed > walBal) return -1;
    return e->db.executeBuy(std::string(symbol), price, qty, buyFee);
}

int qe_execute_sell(QEngine* e, const char* symbol,
                    double price, double qty, double sellFee)
{
    return e->db.executeSell(std::string(symbol), price, qty, sellFee);
}

// ---- Profit ----

QProfitResult qe_profit_calculate(QEngine* e, int tradeId, double currentPrice)
{
    QProfitResult r{};
    auto trades = e->db.loadTrades();
    for (const auto& t : trades)
        if (t.tradeId == tradeId)
        {
            auto pr = ProfitCalculator::calculate(t, currentPrice,
                                                  t.buyFee, t.sellFee);
            r.grossProfit = pr.grossProfit;
            r.netProfit   = pr.netProfit;
            r.roi         = pr.roi;
            return r;
        }
    return r;
}

// ---- Entry Points ----

int qe_entry_count(QEngine* e)
{
    return static_cast<int>(e->db.loadEntryPoints().size());
}

int qe_entry_list(QEngine* e, QEntryPoint* out, int maxCount)
{
    auto entries = e->db.loadEntryPoints();
    int n = std::min(static_cast<int>(entries.size()), maxCount);
    for (int i = 0; i < n; ++i)
        out[i] = toCEntry(entries[i]);
    return n;
}

int qe_entry_add(QEngine* e, const QEntryPoint* ep)
{
    TradeDatabase::EntryPoint entry = fromCEntry(*ep);
    entry.entryId = e->db.nextEntryId();
    auto all = e->db.loadEntryPoints();
    all.push_back(entry);
    e->db.saveEntryPoints(all);
    return entry.entryId;
}

int qe_entry_update(QEngine* e, const QEntryPoint* ep)
{
    auto all = e->db.loadEntryPoints();
    for (auto& existing : all)
        if (existing.entryId == ep->entryId)
        {
            existing = fromCEntry(*ep);
            e->db.saveEntryPoints(all);
            return 1;
        }
    return 0;
}

int qe_entry_delete(QEngine* e, int entryId)
{
    auto all = e->db.loadEntryPoints();
    auto it = std::remove_if(all.begin(), all.end(),
        [entryId](const TradeDatabase::EntryPoint& ep) { return ep.entryId == entryId; });
    if (it == all.end()) return 0;
    all.erase(it, all.end());
    e->db.saveEntryPoints(all);
    return 1;
}

int qe_entry_get(QEngine* e, int entryId, QEntryPoint* out)
{
    auto entries = e->db.loadEntryPoints();
    for (const auto& ep : entries)
        if (ep.entryId == entryId) { *out = toCEntry(ep); return 1; }
    return 0;
}

// ---- Serial Generator ----

QSerialPlanSummary qe_serial_generate(const QSerialParams* params,
                                      QSerialEntry* out, int maxEntries)
{
    auto sp = fromCParams(*params);
    auto plan = QuantMath::generateSerialPlan(sp);

    QSerialPlanSummary s{};
    s.overhead     = plan.overhead;
    s.effectiveOH  = plan.effectiveOH;
    s.totalFunding = plan.totalFunding;
    s.totalTpGross = plan.totalTpGross;
    s.entryCount   = static_cast<int>(plan.entries.size());

    int n = std::min(s.entryCount, maxEntries);
    for (int i = 0; i < n; ++i)
    {
        const auto& e = plan.entries[i];
        QSerialEntry& q = out[i];
        q.index       = e.index;
        q.entryPrice  = e.entryPrice;
        q.breakEven   = e.breakEven;
        q.discountPct = e.discountPct;
        q.funding     = e.funding;
        q.fundQty     = e.fundQty;
        q.tpUnit      = e.tpUnit;
        q.tpTotal     = e.tpTotal;
        q.tpGross     = e.tpGross;
        q.slUnit      = e.slUnit;
        q.slLoss      = e.slLoss;
    }
    return s;
}

// ---- Cycle ----

QCycleResult qe_cycle_compute(const QSerialParams* params)
{
    auto sp = fromCParams(*params);
    auto plan = QuantMath::generateSerialPlan(sp);
    auto cr = QuantMath::computeCycle(plan, sp);

    QCycleResult r{};
    r.totalCost      = cr.totalCost;
    r.totalRevenue   = cr.totalRevenue;
    r.totalFees      = cr.totalFees;
    r.grossProfit    = cr.grossProfit;
    r.savingsAmount  = cr.savingsAmount;
    r.reinvestAmount = cr.reinvestAmount;
    r.nextCycleFunds = cr.nextCycleFunds;
    return r;
}

// ---- Managed Chains ----

int qe_chain_count(QEngine* e)
{
    return static_cast<int>(e->db.loadManagedChains().size());
}

int qe_chain_list(QEngine* e, QManagedChain* out, int maxCount)
{
    auto chains = e->db.loadManagedChains();
    int n = std::min(static_cast<int>(chains.size()), maxCount);
    for (int i = 0; i < n; ++i)
        out[i] = toCChain(chains[i]);
    return n;
}

int qe_chain_create(QEngine* e, const QManagedChain* chain)
{
    TradeDatabase::ManagedChain c;
    c.chainId      = e->db.nextChainId();
    c.name         = chain->name;
    c.symbol       = chain->symbol;
    c.active       = (chain->active != 0);
    c.theoretical  = (chain->theoretical != 0);
    c.currentCycle = chain->currentCycle;
    c.capital      = chain->capital;
    c.totalSavings = chain->totalSavings;
    c.savingsRate  = chain->savingsRate;

    auto all = e->db.loadManagedChains();
    all.push_back(c);
    e->db.saveManagedChains(all);
    return c.chainId;
}

int qe_chain_update(QEngine* e, const QManagedChain* chain)
{
    auto all = e->db.loadManagedChains();
    for (auto& c : all)
        if (c.chainId == chain->chainId)
        {
            c.name         = chain->name;
            c.symbol       = chain->symbol;
            c.active       = (chain->active != 0);
            c.theoretical  = (chain->theoretical != 0);
            c.currentCycle = chain->currentCycle;
            c.capital      = chain->capital;
            c.totalSavings = chain->totalSavings;
            c.savingsRate  = chain->savingsRate;
            e->db.saveManagedChains(all);
            return 1;
        }
    return 0;
}

int qe_chain_delete(QEngine* e, int chainId)
{
    auto all = e->db.loadManagedChains();
    auto it = std::remove_if(all.begin(), all.end(),
        [chainId](const TradeDatabase::ManagedChain& c) { return c.chainId == chainId; });
    if (it == all.end()) return 0;
    all.erase(it, all.end());
    e->db.saveManagedChains(all);
    return 1;
}

int qe_chain_get(QEngine* e, int chainId, QManagedChain* out)
{
    auto chains = e->db.loadManagedChains();
    for (const auto& c : chains)
        if (c.chainId == chainId) { *out = toCChain(c); return 1; }
    return 0;
}

int qe_chain_activate(QEngine* e, int chainId)
{
    auto all = e->db.loadManagedChains();
    for (auto& c : all)
        if (c.chainId == chainId)
        { c.active = true; c.theoretical = false; e->db.saveManagedChains(all); return 1; }
    return 0;
}

int qe_chain_deactivate(QEngine* e, int chainId)
{
    auto all = e->db.loadManagedChains();
    for (auto& c : all)
        if (c.chainId == chainId)
        { c.active = false; e->db.saveManagedChains(all); return 1; }
    return 0;
}

int qe_chain_add_entries(QEngine* e, int chainId, int cycle,
                         const int* entryIds, int count)
{
    auto all = e->db.loadManagedChains();
    for (auto& c : all)
        if (c.chainId == chainId)
        {
            TradeDatabase::ManagedChain::CycleEntries* target = nullptr;
            for (auto& cy : c.cycles)
                if (cy.cycle == cycle) { target = &cy; break; }
            if (!target)
            {
                c.cycles.push_back({});
                target = &c.cycles.back();
                target->cycle = cycle;
            }
            for (int i = 0; i < count; ++i)
                target->entryIds.push_back(entryIds[i]);
            e->db.saveManagedChains(all);
            return 1;
        }
    return 0;
}

int qe_chain_remove_entry(QEngine* e, int chainId, int cycle, int entryId)
{
    auto all = e->db.loadManagedChains();
    for (auto& c : all)
        if (c.chainId == chainId)
        {
            for (auto& cy : c.cycles)
                if (cy.cycle == cycle)
                {
                    auto& ids = cy.entryIds;
                    ids.erase(std::remove(ids.begin(), ids.end(), entryId), ids.end());
                    e->db.saveManagedChains(all);
                    return 1;
                }
        }
    return 0;
}

// ---- Exit Strategy ----

int qe_exit_generate(double entryPrice, double quantity,
                     double buyFee, double rawOH, double eo,
                     double maxRisk, int horizonCount,
                     double riskCoefficient, double exitFraction,
                     double steepness,
                     QExitLevel* out, int maxLevels)
{
    QuantMath::ExitParams ep;
    ep.entryPrice      = entryPrice;
    ep.quantity         = quantity;
    ep.buyFee           = buyFee;
    ep.rawOH            = rawOH;
    ep.eo               = eo;
    ep.maxRisk          = maxRisk;
    ep.horizonCount     = horizonCount;
    ep.riskCoefficient  = riskCoefficient;
    ep.exitFraction     = exitFraction;
    ep.steepness        = steepness;

    auto plan = QuantMath::generateExitPlan(ep);
    int n = std::min(static_cast<int>(plan.levels.size()), maxLevels);
    for (int i = 0; i < n; ++i)
    {
        const auto& l = plan.levels[i];
        out[i].index        = l.index;
        out[i].tpPrice      = l.tpPrice;
        out[i].sellQty      = l.sellQty;
        out[i].sellFraction = l.sellFraction;
        out[i].sellValue    = l.sellValue;
        out[i].grossProfit  = l.grossProfit;
    }
    return n;
}

// ---- Pure math ----

double qe_overhead(double price, double qty, double feeSpread,
                   double feeHedging, double deltaTime,
                   int symbolCount, double availableFunds,
                   double coefficientK, int futureTradeCount)
{
    return QuantMath::overhead(price, qty, feeSpread, feeHedging,
                               deltaTime, symbolCount, availableFunds,
                               coefficientK, futureTradeCount);
}

double qe_effective_overhead(double overhead, double surplusRate,
                             double feeSpread, double feeHedging,
                             double deltaTime)
{
    return QuantMath::effectiveOverhead(overhead, surplusRate,
                                        feeSpread, feeHedging, deltaTime);
}

// ---- Exit Points ----

static QExitPointData toCExit(const TradeDatabase::ExitPoint& ep)
{
    QExitPointData q{};
    q.exitId       = ep.exitId;
    q.tradeId      = ep.tradeId;
    copyStr(q.symbol, sizeof(q.symbol), ep.symbol);
    q.levelIndex   = ep.levelIndex;
    q.tpPrice      = ep.tpPrice;
    q.slPrice      = ep.slPrice;
    q.sellQty      = ep.sellQty;
    q.sellFraction = ep.sellFraction;
    q.slActive     = ep.slActive ? 1 : 0;
    q.executed     = ep.executed ? 1 : 0;
    q.linkedSellId = ep.linkedSellId;
    return q;
}

static TradeDatabase::ExitPoint fromCExit(const QExitPointData& q)
{
    TradeDatabase::ExitPoint ep;
    ep.exitId       = q.exitId;
    ep.tradeId      = q.tradeId;
    ep.symbol       = q.symbol;
    ep.levelIndex   = q.levelIndex;
    ep.tpPrice      = q.tpPrice;
    ep.slPrice      = q.slPrice;
    ep.sellQty      = q.sellQty;
    ep.sellFraction = q.sellFraction;
    ep.slActive     = (q.slActive != 0);
    ep.executed     = (q.executed != 0);
    ep.linkedSellId = q.linkedSellId;
    return ep;
}

int qe_exitpt_count(QEngine* e)
{
    return static_cast<int>(e->db.loadExitPoints().size());
}

int qe_exitpt_list(QEngine* e, QExitPointData* out, int maxCount)
{
    auto pts = e->db.loadExitPoints();
    int n = std::min(static_cast<int>(pts.size()), maxCount);
    for (int i = 0; i < n; ++i)
        out[i] = toCExit(pts[i]);
    return n;
}

int qe_exitpt_list_for_trade(QEngine* e, int tradeId,
                              QExitPointData* out, int maxCount)
{
    auto pts = e->db.loadExitPointsForTrade(tradeId);
    int n = std::min(static_cast<int>(pts.size()), maxCount);
    for (int i = 0; i < n; ++i)
        out[i] = toCExit(pts[i]);
    return n;
}

int qe_exitpt_add(QEngine* e, QExitPointData* ep)
{
    auto all = e->db.loadExitPoints();
    int maxIdx = -1;
    for (const auto& x : all)
        if (x.tradeId == ep->tradeId && x.levelIndex > maxIdx) maxIdx = x.levelIndex;
    TradeDatabase::ExitPoint xp = fromCExit(*ep);
    xp.exitId     = e->db.nextExitId();
    xp.levelIndex = maxIdx + 1;
    ep->exitId    = xp.exitId;
    ep->levelIndex = xp.levelIndex;
    all.push_back(xp);
    e->db.saveExitPoints(all);
    return xp.exitId;
}

int qe_exitpt_update(QEngine* e, const QExitPointData* ep)
{
    auto all = e->db.loadExitPoints();
    for (auto& x : all)
        if (x.exitId == ep->exitId)
        {
            x.tpPrice      = ep->tpPrice;
            x.slPrice      = ep->slPrice;
            x.sellQty      = ep->sellQty;
            x.sellFraction = ep->sellFraction;
            x.slActive     = (ep->slActive != 0);
            x.executed     = (ep->executed != 0);
            x.linkedSellId = ep->linkedSellId;
            break;
        }
    e->db.saveExitPoints(all);
    return 1;
}

int qe_exitpt_delete(QEngine* e, int exitId)
{
    auto all = e->db.loadExitPoints();
    all.erase(std::remove_if(all.begin(), all.end(),
        [exitId](const TradeDatabase::ExitPoint& x) { return x.exitId == exitId; }),
        all.end());
    e->db.saveExitPoints(all);
    e->db.releaseExitId(exitId);
    return 1;
}

} // extern "C"
