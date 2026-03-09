#pragma once

#include "QuantMath.h"
#include "MarketEntryCalculator.h"
#include "MultiHorizonEngine.h"
#include "Simulator.h"
#include "CudaAccelerator.h"

#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <functional>
#include <iostream>
#include <iomanip>

// ============================================================
//  Chain Optimizer — BPTT parameter optimization (§15.9, §16)
// ============================================================
//
// Implements Backpropagation Through Time over the chain
// recurrence T_{c+1} = T_c + ?_c(T_c, ?) · (1 - s_save).
//
// Each chain cycle is a "time step":
//   hidden state  = T_c  (capital)
//   recurrence    = T_{c+1} = f(T_c, ?)
//   objective     = J(T_0, ..., T_C, ?)
//
// Forward pass:
//   Analytically compute per-cycle profit ?_c assuming all
//   entries fill and all TPs are hit (the paper's conditional).
//   Record per-cycle state for the backward pass.
//
// Backward pass:
//   Propagate adjoint ?_c = dJ/dT_c backward through cycles.
//   Accumulate parameter gradients dJ/d? at each cycle.
//
// Objectives (§16.2):
//   J1 MaxProfit:  ? ?_c
//   J2 MinSpread:  -? EO_i²  (tightest TPs)
//   J3 MaxROI:     ? ?_c / T_0
//   J4 MaxChain:   ? (1 + ?_c(1-s_save)/T_c)
//   J5 MaxWealth:  T_C + ? ?_c · s_save
// ============================================================

enum class ChainObjective
{
    MaxProfit  = 1,
    MinSpread  = 2,
    MaxROI     = 3,
    MaxChain   = 4,
    MaxWealth  = 5
};

struct ParamBound
{
    double lower  = -1e15;
    double upper  =  1e15;
    bool   frozen = false;

    double clamp(double v) const { return std::max(lower, std::min(upper, v)); }
};

struct ChainParams
{
    double price         = 0.0;
    double capital       = 0.0;
    int    cycles        = 3;
    int    levels        = 4;
    int    exitLevels    = 0;     // TP levels per trade (0 = same as entry levels)

    // Optimizable parameters ?
    double surplus       = 0.02;
    double risk          = 0.5;
    double steepness     = 6.0;
    double feeHedging    = 1.0;
    double maxRisk       = 0.0;
    double minRisk       = 0.0;
    double savingsRate   = 0.05;

    // Per-parameter optimisation bounds (non-negotiable limits + freeze toggle)
    ParamBound bSurplus     {0.0,  0.10, false};
    ParamBound bRisk        {0.0,  1.0, false};
    ParamBound bSteepness   {0.1, 50.0, false};
    ParamBound bFeeHedging  {0.5,  3.0, false};
    ParamBound bMaxRisk     {0.0,  1.0, false};
    ParamBound bSavingsRate {0.0,  1.0, false};

    // Fixed parameters
    double feeSpread     = 0.001;
    double deltaTime     = 1.0;
    int    symbolCount   = 1;
    double coefficientK  = 0.0;
    double buyFeeRate    = 0.001;
    double sellFeeRate   = 0.001;
    double rangeAbove    = 0.0;
    double rangeBelow    = 0.0;
    int    futureTradeCount = 0;
    double stopLossFraction = 1.0;
    int    stopLossHedgeCount = 0;

    // Exit parameters
    double exitRisk         = 0.5;
    double exitFraction     = 1.0;
    double exitSteepness    = 4.0;
    int    downtrendCount   = 1;

    // Trade frequency & capital pump
    int    maxTradesPerMonth   = 0;     // 0 = unlimited
    double capitalPumpPerMonth = 0.0;   // 0 = disabled

    // Entry range mode
    bool   autoRange = false;  // false = [0, price]; true = EO-adaptive

    // Price series mode (empty = analytical mode)
    std::string  symbol;
    PriceSeries  prices;

    // GPU acceleration (requires QUANT_CUDA build + runtime GPU)
    bool useCuda = false;

    bool hasPriceSeries() const
    {
        return !symbol.empty() && prices.hasSymbol(symbol);
    }
};

struct LevelRecord
{
    double entryPrice   = 0.0;
    double tpPrice      = 0.0;
    double quantity     = 0.0;
    double funding      = 0.0;
    double overhead     = 0.0;
    double effectiveOH  = 0.0;
    double grossProfit  = 0.0;
    double buyFee       = 0.0;
    double sellFee      = 0.0;
    double netProfit    = 0.0;
    long long entryTime = 0;     // unix timestamp of buy fill  (0 = analytical mode)
    long long sellTime  = 0;     // unix timestamp of sell fill (0 = still open)
};

struct CycleRecord
{
    double capital             = 0.0;   // T_c at cycle start
    double totalProfit         = 0.0;   // ?_c
    double savings             = 0.0;   // ?_c · s_save
    double nextCapital         = 0.0;   // T_{c+1}
    std::vector<LevelRecord> levels;

    // Analytical gradients (§15.2, §15.3, §15.7)
    double dProfit_dCapital    = 0.0;   // ??_c/?T_c
    double dProfit_dSurplus    = 0.0;   // ??_c/?s
    double dProfit_dFeeH       = 0.0;   // ??_c/?f_h

    // Numerical gradients for complex parameters
    double dProfit_dRisk       = 0.0;   // ??_c/?r
    double dProfit_dAlpha      = 0.0;   // ??_c/??
    double dProfit_dRmax       = 0.0;   // ??_c/?R_max
};

struct ParamGradients
{
    double dJ_dSurplus     = 0.0;
    double dJ_dRisk        = 0.0;
    double dJ_dSteepness   = 0.0;
    double dJ_dFeeHedging  = 0.0;
    double dJ_dMaxRisk     = 0.0;
    double dJ_dSavingsRate = 0.0;
    double objective       = 0.0;

    double gradNorm() const
    {
        return std::sqrt(dJ_dSurplus * dJ_dSurplus
                       + dJ_dRisk * dJ_dRisk
                       + dJ_dSteepness * dJ_dSteepness
                       + dJ_dFeeHedging * dJ_dFeeHedging
                       + dJ_dMaxRisk * dJ_dMaxRisk
                       + dJ_dSavingsRate * dJ_dSavingsRate);
    }
};

struct StepRecord
{
    int    step        = 0;
    double objective   = 0.0;
    double gradNorm    = 0.0;
    double deltaJ = 0.0;  // J(?) change from previous step
    double surplus     = 0.0;
    double risk        = 0.0;
    double steepness   = 0.0;
    double feeHedging  = 0.0;
    double maxRisk     = 0.0;
    double savingsRate = 0.0;
    double g_surplus   = 0.0;
    double g_risk      = 0.0;
    double g_steepness = 0.0;
    double g_feeHedging = 0.0;
    double g_maxRisk   = 0.0;
    double g_savingsRate = 0.0;
};

struct OptimizationResult
{
    ChainParams      initialParams;
    ChainParams      optimizedParams;
    ParamGradients   initialGradients;
    ParamGradients   finalGradients;
    std::vector<CycleRecord> forwardTrace;
    std::vector<double>      objectiveHistory;
    std::vector<StepRecord>  stepLog;
    int              steps = 0;
    SimResult        simResult;   // populated in sim mode only
};

using StepCallback = std::function<void(const StepRecord&)>;

class ChainOptimizer
{
    static constexpr double EPS       = 1e-15;
    static constexpr double FD_STEP   = 1e-7;
    static constexpr double FD_SIM    = 1e-2;   // must cross discrete trigger boundaries
    static constexpr double GRAD_CLIP = 1e4;    // max gradient magnitude per parameter

    enum class ParamIdx { Risk, Steepness, MaxRisk };

    static const char* objLabel(ChainObjective obj)
    {
        switch (obj) {
            case ChainObjective::MaxProfit: return "MaxProfit";
            case ChainObjective::MinSpread: return "MinSpread";
            case ChainObjective::MaxROI:    return "MaxROI";
            case ChainObjective::MaxChain:  return "MaxChain";
            case ChainObjective::MaxWealth: return "MaxWealth";
        }
        return "?";
    }

    // Record one optimisation step, print to console, and invoke callback
    static void recordStep(OptimizationResult& res,
                           int step,
                           const ChainParams& cur,
                           const ParamGradients& grad,
                           ChainObjective obj,
                           const StepCallback& onStep = nullptr)
    {
        StepRecord sr;
        sr.step        = step;
        sr.objective   = grad.objective;
        sr.gradNorm    = grad.gradNorm();
        sr.surplus     = cur.surplus;
        sr.risk        = cur.risk;
        sr.steepness   = cur.steepness;
        sr.feeHedging  = cur.feeHedging;
        sr.maxRisk     = cur.maxRisk;
        sr.savingsRate = cur.savingsRate;
        sr.g_surplus   = grad.dJ_dSurplus;
        sr.g_risk      = grad.dJ_dRisk;
        sr.g_steepness = grad.dJ_dSteepness;
        sr.g_feeHedging = grad.dJ_dFeeHedging;
        sr.g_maxRisk   = grad.dJ_dMaxRisk;
        sr.g_savingsRate = grad.dJ_dSavingsRate;

        // Compute ?J from previous step
        if (!res.stepLog.empty())
            sr.deltaJ = grad.objective - res.stepLog.back().objective;

        res.stepLog.push_back(sr);

        // Console progress
        if (step == 0)
            std::cout << "  [BPTT] " << objLabel(obj)
                      << " | step   J(?)              ?J               ||?J||           s         r         ?         fh        Rmax      s_save\n";

        std::cout << std::fixed << std::setprecision(8)
                  << "  [BPTT] " << std::setw(4) << step
                  << "   " << std::setw(18) << grad.objective
                  << "  " << std::setw(16) << sr.deltaJ
                  << "  " << std::setw(14) << sr.gradNorm
                  << "  " << std::setw(10) << cur.surplus
                  << " " << std::setw(9) << cur.risk
                  << " " << std::setw(9) << cur.steepness
                  << " " << std::setw(9) << cur.feeHedging
                  << " " << std::setw(9) << cur.maxRisk
                  << " " << std::setw(9) << cur.savingsRate
                  << "\n";

        if (onStep) onStep(sr);
    }

    // ---- Apply non-negotiable bounds to all optimisable parameters ----
    static void applyBounds(ChainParams& cur)
    {
        cur.surplus     = cur.bSurplus.clamp(cur.surplus);
        cur.risk        = cur.bRisk.clamp(cur.risk);
        cur.steepness   = cur.bSteepness.clamp(cur.steepness);
        cur.feeHedging  = cur.bFeeHedging.clamp(cur.feeHedging);
        cur.maxRisk     = cur.bMaxRisk.clamp(cur.maxRisk);
        cur.savingsRate = cur.bSavingsRate.clamp(cur.savingsRate);
    }

    // ---- Zero out gradients for frozen (non-learnable) parameters ----
    static void zeroFrozenGrads(const ChainParams& p, ParamGradients& g)
    {
        if (p.bSurplus.frozen)     g.dJ_dSurplus     = 0;
        if (p.bRisk.frozen)        g.dJ_dRisk        = 0;
        if (p.bSteepness.frozen)   g.dJ_dSteepness   = 0;
        if (p.bFeeHedging.frozen)  g.dJ_dFeeHedging  = 0;
        if (p.bMaxRisk.frozen)     g.dJ_dMaxRisk     = 0;
        if (p.bSavingsRate.frozen) g.dJ_dSavingsRate = 0;
    }

    // ---- Build HorizonParams from ChainParams ----
    static HorizonParams makeHP(const ChainParams& p, double capital)
    {
        HorizonParams hp;
        hp.feeSpread             = p.feeSpread;
        hp.feeHedgingCoefficient = p.feeHedging;
        hp.surplusRate           = p.surplus;
        hp.symbolCount           = p.symbolCount;
        hp.deltaTime             = p.deltaTime;
        hp.coefficientK          = p.coefficientK;
        hp.horizonCount          = (p.exitLevels > 0) ? p.exitLevels : p.levels;
        hp.portfolioPump         = capital;
        hp.maxRisk               = p.maxRisk;
        hp.minRisk               = p.minRisk;
        hp.futureTradeCount      = p.futureTradeCount;
        hp.stopLossFraction      = p.stopLossFraction;
        hp.stopLossHedgeCount    = p.stopLossHedgeCount;
        return hp;
    }

    // ---- Compute total cycle profit for a parameter set ----
    static double cycleProfit(const ChainParams& p, double capital)
    {
        auto hp = makeHP(p, capital);
        auto entries = MarketEntryCalculator::generate(
            p.price, 1.0, hp,
            p.risk, p.steepness,
            p.rangeAbove, p.rangeBelow, p.autoRange);

        double minEntry = p.price * 0.01;
        std::erase_if(entries, [minEntry](const EntryLevel& el) {
            return el.entryPrice < minEntry;
        });

        double total = 0;
        for (const auto& e : entries)
        {
            double oh = QuantMath::overhead(
                e.entryPrice, e.fundingQty,
                p.feeSpread, p.feeHedging, p.deltaTime,
                p.symbolCount, capital, p.coefficientK,
                p.futureTradeCount);
            double eo = QuantMath::effectiveOverhead(
                oh, p.surplus, p.feeSpread,
                p.feeHedging, p.deltaTime);
            double tp     = e.entryPrice * (1.0 + eo);
            double cost   = e.entryPrice * e.fundingQty;
            double buyFee = cost * p.buyFeeRate;
            double slFee  = tp * e.fundingQty * p.sellFeeRate;
            double gross  = (tp - e.entryPrice) * e.fundingQty;
            total += gross - buyFee - slFee;
        }
        return total;
    }

    // ---- Numerical gradient via central finite differences ----
    static double numericalGrad(const ChainParams& p, double capital, ParamIdx idx)
    {
        auto perturb = [&](double delta) {
            ChainParams q = p;
            switch (idx) {
                case ParamIdx::Risk:      q.risk      += delta; break;
                case ParamIdx::Steepness: q.steepness += delta; break;
                case ParamIdx::MaxRisk:   q.maxRisk   += delta; break;
            }
            q.risk      = QuantMath::clamp01(q.risk);
            q.steepness = std::max(0.1, q.steepness);
            q.maxRisk   = std::max(0.0, q.maxRisk);
            return cycleProfit(q, capital);
        };
        return (perturb(+FD_STEP) - perturb(-FD_STEP)) / (2.0 * FD_STEP);
    }

    // ---- Compute objective J from forward trace ----
    static double computeObjective(const std::vector<CycleRecord>& trace,
                                   const ChainParams& p,
                                   ChainObjective obj)
    {
        switch (obj)
        {
            case ChainObjective::MaxProfit:
            {
                double s = 0;
                for (const auto& c : trace) s += c.totalProfit;
                return s;
            }
            case ChainObjective::MinSpread:
            {
                double s = 0;
                for (const auto& c : trace)
                    for (const auto& l : c.levels) {
                        double sp = (l.tpPrice - l.entryPrice) / std::max(l.entryPrice, EPS);
                        s -= sp * sp;
                    }
                return s;
            }
            case ChainObjective::MaxROI:
            {
                double s = 0;
                for (const auto& c : trace) s += c.totalProfit;
                return (p.capital > EPS) ? s / p.capital : 0;
            }
            case ChainObjective::MaxChain:
            {
                double prod = 1.0;
                for (const auto& c : trace) {
                    double Tc = std::max(c.capital, EPS);
                    prod *= (1.0 + c.totalProfit * (1.0 - p.savingsRate) / Tc);
                }
                return prod;
            }
            case ChainObjective::MaxWealth:
            {
                double fc = trace.empty() ? p.capital : trace.back().nextCapital;
                double sv = 0;
                for (const auto& c : trace) sv += c.savings;
                return fc + sv;
            }
        }
        return 0;
    }

    // ---- Terminal adjoint ?_C = dJ/dT_C ----
    static double terminalAdjoint(ChainObjective obj)
    {
        // Only MaxWealth has J depending directly on T_C: J = T_C + ...
        return (obj == ChainObjective::MaxWealth) ? 1.0 : 0.0;
    }

    // ---- Per-cycle objective gradient contributions ----
    static void perCycleGradients(
        const std::vector<CycleRecord>& trace, const ChainParams& p,
        ChainObjective obj, int c,
        double& dJc_dTc,
        double& dJc_ds, double& dJc_dr, double& dJc_dalpha,
        double& dJc_dfh, double& dJc_dRmax, double& dJc_dsave)
    {
        const auto& cs = trace[c];
        dJc_dTc = 0; dJc_ds = 0; dJc_dr = 0; dJc_dalpha = 0;
        dJc_dfh = 0; dJc_dRmax = 0; dJc_dsave = 0;

        switch (obj)
        {
            case ChainObjective::MaxProfit:
                // J = ? ?_c  ?  ?J_c/?? = ??_c/??
                dJc_ds     = cs.dProfit_dSurplus;
                dJc_dr     = cs.dProfit_dRisk;
                dJc_dalpha = cs.dProfit_dAlpha;
                dJc_dfh    = cs.dProfit_dFeeH;
                dJc_dRmax  = cs.dProfit_dRmax;
                dJc_dTc    = cs.dProfit_dCapital;
                break;

            case ChainObjective::MinSpread:
            {
                // J = -? EO_i²  ?  ?J_c/?s = -? 2·EO_i · f_h·?t
                for (const auto& l : cs.levels) {
                    double dEO_ds = p.feeHedging * p.deltaTime;
                    dJc_ds += -2.0 * l.effectiveOH * dEO_ds;
                }
                break;
            }

            case ChainObjective::MaxROI:
            {
                // J = ? ?_c / T_0
                double inv = (p.capital > EPS) ? 1.0 / p.capital : 0;
                dJc_ds     = cs.dProfit_dSurplus * inv;
                dJc_dr     = cs.dProfit_dRisk    * inv;
                dJc_dalpha = cs.dProfit_dAlpha   * inv;
                dJc_dfh    = cs.dProfit_dFeeH    * inv;
                dJc_dRmax  = cs.dProfit_dRmax    * inv;
                break;
            }

            case ChainObjective::MaxChain:
            {
                // J = ? (1 + ?_c·(1-s_save)/T_c)
                double Tc  = std::max(cs.capital, EPS);
                double fac = 1.0 + cs.totalProfit * (1.0 - p.savingsRate) / Tc;
                double J   = computeObjective(trace, p, obj);
                double dJ_dPi = (std::abs(fac) > EPS)
                    ? J / fac * (1.0 - p.savingsRate) / Tc : 0;
                dJc_ds     = dJ_dPi * cs.dProfit_dSurplus;
                dJc_dr     = dJ_dPi * cs.dProfit_dRisk;
                dJc_dalpha = dJ_dPi * cs.dProfit_dAlpha;
                dJc_dfh    = dJ_dPi * cs.dProfit_dFeeH;
                dJc_dRmax  = dJ_dPi * cs.dProfit_dRmax;
                dJc_dTc    = (std::abs(fac) > EPS)
                    ? J / fac * (-cs.totalProfit * (1.0 - p.savingsRate) / (Tc * Tc)) : 0;
                dJc_dsave  = (std::abs(fac) > EPS)
                    ? J / fac * (-cs.totalProfit / Tc) : 0;
                break;
            }

            case ChainObjective::MaxWealth:
            {
                // J = T_C + ? ?_c · s_save  ?  savings term
                dJc_ds     = p.savingsRate * cs.dProfit_dSurplus;
                dJc_dr     = p.savingsRate * cs.dProfit_dRisk;
                dJc_dalpha = p.savingsRate * cs.dProfit_dAlpha;
                dJc_dfh    = p.savingsRate * cs.dProfit_dFeeH;
                dJc_dRmax  = p.savingsRate * cs.dProfit_dRmax;
                dJc_dsave  = cs.totalProfit;
                break;
            }
        }
    }

    // ---- Build SimConfig from ChainParams ----
    static SimConfig toSimConfig(const ChainParams& p)
    {
        SimConfig cfg;
        cfg.symbol          = p.symbol;
        cfg.startingCapital = p.capital;
        cfg.prices          = &p.prices;
        cfg.entryRisk       = p.risk;
        cfg.entrySteepness  = p.steepness;
        cfg.entryRangeBelow = p.rangeBelow;
        cfg.entryRangeAbove = p.rangeAbove;
        cfg.exitRisk        = p.exitRisk;
        cfg.exitFraction    = p.exitFraction;
        cfg.exitSteepness   = p.exitSteepness;
        cfg.buyFeeRate      = p.buyFeeRate;
        cfg.sellFeeRate     = p.sellFeeRate;
        cfg.downtrendCount  = p.downtrendCount;
        cfg.chainCycles     = true;
        cfg.savingsRate     = p.savingsRate;
        cfg.autoRange       = p.autoRange;
        cfg.entryLevels     = p.levels;
        cfg.exitLevels      = p.exitLevels;

        cfg.horizonParams.feeSpread             = p.feeSpread;
        cfg.horizonParams.feeHedgingCoefficient = p.feeHedging;
        cfg.horizonParams.surplusRate           = p.surplus;
        cfg.horizonParams.symbolCount           = p.symbolCount;
        cfg.horizonParams.deltaTime             = p.deltaTime;
        cfg.horizonParams.coefficientK          = p.coefficientK;
        cfg.horizonParams.horizonCount          = p.levels;
        cfg.horizonParams.portfolioPump         = p.capital;
        cfg.horizonParams.maxRisk               = p.maxRisk;
        cfg.horizonParams.minRisk               = p.minRisk;
        cfg.horizonParams.futureTradeCount      = p.futureTradeCount;
        cfg.horizonParams.stopLossFraction      = p.stopLossFraction;
        cfg.horizonParams.stopLossHedgeCount    = p.stopLossHedgeCount;
        return cfg;
    }

    // ---- Forward pass via simulator (price-series mode) ----
    static std::vector<CycleRecord> forwardSim(const ChainParams& p)
    {
        return forwardSimFull(p, nullptr);
    }

    // Full version that optionally returns the SimResult for objective use
    static std::vector<CycleRecord> forwardSimFull(const ChainParams& p,
                                                    SimResult* outResult)
    {
        auto cfg    = toSimConfig(p);
        auto result = Simulator::run(cfg);

        // Group sells by cycle
        std::map<int, double> cycleProfit;
        for (const auto& s : result.sells)
            cycleProfit[s.cycle] += s.netProfit;

        int maxCycle = std::max(result.cyclesCompleted, 1);
        if (!cycleProfit.empty())
            maxCycle = std::max(maxCycle, cycleProfit.rbegin()->first + 1);

        std::vector<CycleRecord> trace;
        double capital = p.capital;

        for (int c = 0; c < maxCycle; ++c)
        {
            CycleRecord cr;
            cr.capital     = capital;
            cr.totalProfit = cycleProfit.count(c) ? cycleProfit[c] : 0;
            cr.savings     = (cr.totalProfit > 0) ? cr.totalProfit * p.savingsRate : 0;
            cr.nextCapital = capital + cr.totalProfit - cr.savings;

            for (const auto& t : result.trades) {
                if (t.cycle != c) continue;
                LevelRecord lr;
                lr.entryPrice = t.entryPrice;
                lr.quantity   = t.quantity;
                lr.funding    = t.entryPrice * t.quantity;
                lr.buyFee     = t.buyFee;
                lr.entryTime  = t.entryTime;

                // Compute overhead and effective overhead (simulator uses entryCost as portfolioPump)
                double entryCost = t.entryPrice * t.quantity;
                lr.overhead = QuantMath::overhead(
                    t.entryPrice, t.quantity,
                    p.feeSpread, p.feeHedging, p.deltaTime,
                    p.symbolCount, entryCost, p.coefficientK,
                    p.futureTradeCount);
                lr.effectiveOH = QuantMath::effectiveOverhead(
                    lr.overhead, p.surplus, p.feeSpread,
                    p.feeHedging, p.deltaTime);

                // Default target TP from overhead formula
                lr.tpPrice = t.entryPrice * (1.0 + lr.effectiveOH);

                // Override with actual sell data if the position was closed
                for (const auto& s : result.sells) {
                    if (s.buyId == t.id) {
                        lr.tpPrice     = s.sellPrice;
                        lr.sellFee     = s.sellFee;
                        lr.grossProfit = s.grossProfit;
                        lr.netProfit   = s.netProfit;
                        lr.sellTime    = s.sellTime;
                    }
                }
                cr.levels.push_back(lr);
            }

            trace.push_back(cr);
            capital = cr.nextCapital;
        }

        if (trace.empty()) {
            CycleRecord cr;
            cr.capital     = p.capital;
            cr.totalProfit = result.totalRealized;
            cr.savings     = 0;
            cr.nextCapital = result.finalCapital;
            trace.push_back(cr);
        }

        // Patch the last cycle's nextCapital with the simulator's true final state
        // (accounts for capital locked in open positions)
        trace.back().nextCapital = result.finalCapital + result.totalSavings;

        if (outResult)
            *outResult = std::move(result);

        return trace;
    }

    // ---- Compute objective from a simulator run ----
    static double simObjective(const ChainParams& p, ChainObjective obj)
    {
        auto trace = forwardSim(p);
        return computeObjective(trace, p, obj);
    }

    // ---- End-to-end numerical gradients for simulator mode ----
    static ParamGradients simGradients(const ChainParams& p, ChainObjective obj)
    {
#ifdef QUANT_CUDA
        // Try GPU path first: all 13 probes run in parallel on the GPU
        if (p.useCuda && CudaAccelerator::isAvailable())
        {
            ParamGradients g;
            if (cudaSimGradients(p, obj, g))
                return g;
            // GPU dispatch failed — fall through to CPU path
        }
#endif

        auto trace = forwardSim(p);
        ParamGradients g;
        g.objective = computeObjective(trace, p, obj);

        // Create two scratch copies ONCE to avoid repeated PriceSeries deep-copies.
        // Each probe only mutates a single scalar field then restores it.
        ChainParams pp = p, pm = p;

        auto fd = [&](auto getter, auto setter) -> double {
            double base = getter(pp);
            setter(pp, std::max(0.0, base + FD_SIM));
            setter(pm, std::max(0.0, base - FD_SIM));
            double jp = simObjective(pp, obj);
            double jm = simObjective(pm, obj);
            setter(pp, base);  // restore
            setter(pm, base);
            return (jp - jm) / (2.0 * FD_SIM);
        };

        g.dJ_dSurplus = fd(
            [](const ChainParams& q) { return q.surplus; },
            [](ChainParams& q, double v) { q.surplus = std::max(0.0, v); });
        g.dJ_dRisk = fd(
            [](const ChainParams& q) { return q.risk; },
            [](ChainParams& q, double v) { q.risk = QuantMath::clamp01(v); });
        g.dJ_dSteepness = fd(
            [](const ChainParams& q) { return q.steepness; },
            [](ChainParams& q, double v) { q.steepness = std::max(0.1, v); });
        g.dJ_dFeeHedging = fd(
            [](const ChainParams& q) { return q.feeHedging; },
            [](ChainParams& q, double v) { q.feeHedging = std::max(0.1, v); });
        g.dJ_dMaxRisk = fd(
            [](const ChainParams& q) { return q.maxRisk; },
            [](ChainParams& q, double v) { q.maxRisk = std::max(0.0, v); });
        g.dJ_dSavingsRate = fd(
            [](const ChainParams& q) { return q.savingsRate; },
            [](ChainParams& q, double v) { q.savingsRate = QuantMath::clamp01(v); });

        // Clip gradients to prevent catastrophic overshoot
        auto clip = [](double v) {
            return std::max(-GRAD_CLIP, std::min(GRAD_CLIP, v));
        };
        g.dJ_dSurplus     = clip(g.dJ_dSurplus);
        g.dJ_dRisk        = clip(g.dJ_dRisk);
        g.dJ_dSteepness   = clip(g.dJ_dSteepness);
        g.dJ_dFeeHedging  = clip(g.dJ_dFeeHedging);
        g.dJ_dMaxRisk     = clip(g.dJ_dMaxRisk);
        g.dJ_dSavingsRate = clip(g.dJ_dSavingsRate);

        return g;
    }

#ifdef QUANT_CUDA
    // GPU-accelerated gradient computation: runs all 13 probes in parallel
    static bool cudaSimGradients(const ChainParams& p, ChainObjective obj,
                                  ParamGradients& outGrad)
    {
        if (!p.hasPriceSeries()) return false;

        const auto& pts = p.prices.data().at(p.symbol);
        int numPrices = static_cast<int>(pts.size());
        if (numPrices == 0) return false;

        // Flatten price series to contiguous arrays
        std::vector<double> timestamps(numPrices);
        std::vector<double> priceVals(numPrices);
        for (int i = 0; i < numPrices; ++i)
        {
            timestamps[i] = static_cast<double>(pts[i].timestamp);
            priceVals[i]  = pts[i].price;
        }

        // Convert ChainParams ? GpuSimParams
        auto toGpu = [](const ChainParams& cp) -> GpuSimParams {
            GpuSimParams g{};
            g.capital = cp.capital; g.price = cp.price;
            g.surplus = cp.surplus; g.risk = cp.risk;
            g.steepness = cp.steepness; g.feeHedging = cp.feeHedging;
            g.maxRisk = cp.maxRisk; g.minRisk = cp.minRisk;
            g.savingsRate = cp.savingsRate; g.feeSpread = cp.feeSpread;
            g.deltaTime = cp.deltaTime; g.symbolCount = cp.symbolCount;
            g.coefficientK = cp.coefficientK;
            g.buyFeeRate = cp.buyFeeRate; g.sellFeeRate = cp.sellFeeRate;
            g.rangeAbove = cp.rangeAbove; g.rangeBelow = cp.rangeBelow;
            g.futureTradeCount = cp.futureTradeCount;
            g.stopLossFraction = cp.stopLossFraction;
            g.stopLossHedgeCount = cp.stopLossHedgeCount;
            g.levels = cp.levels; g.downtrendCount = cp.downtrendCount;
            g.exitRisk = cp.exitRisk; g.exitFraction = cp.exitFraction;
            g.exitSteepness = cp.exitSteepness;
            g.entryRisk = cp.risk; g.entrySteepness = cp.steepness;
            g.chainCycles = 1;
            g.exitLevels = cp.exitLevels;
            g.maxTradesPerMonth = cp.maxTradesPerMonth;
            g.capitalPumpPerMonth = cp.capitalPumpPerMonth;
            g.autoRange = cp.autoRange ? 1 : 0;
            return g;
        };

        // Build 13 configs: [base, +s, -s, +r, -r, +?, -?, +fh, -fh, +Rm, -Rm, +ss, -ss]
        GpuSimParams configs[13];
        configs[0] = toGpu(p);

        auto mkp = [&](auto mutator) -> GpuSimParams {
            ChainParams t = p;
            t.prices = PriceSeries();  // don't deep-copy the price series
            mutator(t);
            return toGpu(t);
        };

        configs[1]  = mkp([](ChainParams& q) { q.surplus = std::max(0.0, q.surplus + FD_SIM); });
        configs[2]  = mkp([](ChainParams& q) { q.surplus = std::max(0.0, q.surplus - FD_SIM); });
        configs[3]  = mkp([](ChainParams& q) { q.risk = QuantMath::clamp01(q.risk + FD_SIM); });
        configs[4]  = mkp([](ChainParams& q) { q.risk = QuantMath::clamp01(q.risk - FD_SIM); });
        configs[5]  = mkp([](ChainParams& q) { q.steepness = std::max(0.1, q.steepness + FD_SIM); });
        configs[6]  = mkp([](ChainParams& q) { q.steepness = std::max(0.1, q.steepness - FD_SIM); });
        configs[7]  = mkp([](ChainParams& q) { q.feeHedging = std::max(0.1, q.feeHedging + FD_SIM); });
        configs[8]  = mkp([](ChainParams& q) { q.feeHedging = std::max(0.1, q.feeHedging - FD_SIM); });
        configs[9]  = mkp([](ChainParams& q) { q.maxRisk = std::max(0.0, q.maxRisk + FD_SIM); });
        configs[10] = mkp([](ChainParams& q) { q.maxRisk = std::max(0.0, q.maxRisk - FD_SIM); });
        configs[11] = mkp([](ChainParams& q) { q.savingsRate = QuantMath::clamp01(q.savingsRate + FD_SIM); });
        configs[12] = mkp([](ChainParams& q) { q.savingsRate = QuantMath::clamp01(q.savingsRate - FD_SIM); });

        double results[13] = {};
        bool ok = cudaGpuBatchSim(configs, 13,
                                   timestamps.data(), priceVals.data(), numPrices,
                                   static_cast<int>(obj), results);
        if (!ok) return false;

        outGrad.objective     = results[0];
        double denom = 2.0 * FD_SIM;
        outGrad.dJ_dSurplus     = (results[1]  - results[2])  / denom;
        outGrad.dJ_dRisk        = (results[3]  - results[4])  / denom;
        outGrad.dJ_dSteepness   = (results[5]  - results[6])  / denom;
        outGrad.dJ_dFeeHedging  = (results[7]  - results[8])  / denom;
        outGrad.dJ_dMaxRisk     = (results[9]  - results[10]) / denom;
        outGrad.dJ_dSavingsRate = (results[11] - results[12]) / denom;

        auto clip = [](double v) {
            return std::max(-GRAD_CLIP, std::min(GRAD_CLIP, v));
        };
        outGrad.dJ_dSurplus     = clip(outGrad.dJ_dSurplus);
        outGrad.dJ_dRisk        = clip(outGrad.dJ_dRisk);
        outGrad.dJ_dSteepness   = clip(outGrad.dJ_dSteepness);
        outGrad.dJ_dFeeHedging  = clip(outGrad.dJ_dFeeHedging);
        outGrad.dJ_dMaxRisk     = clip(outGrad.dJ_dMaxRisk);
        outGrad.dJ_dSavingsRate = clip(outGrad.dJ_dSavingsRate);
        return true;
    }
#endif

public:

    // ---- Forward pass: analytical chain simulation ----
    static std::vector<CycleRecord> forward(const ChainParams& p)
    {
        std::vector<CycleRecord> trace;
        trace.reserve(p.cycles);
        double capital = p.capital;

        for (int c = 0; c < p.cycles; ++c)
        {
            CycleRecord cr;
            cr.capital = capital;

            auto hp = makeHP(p, capital);
            auto entries = MarketEntryCalculator::generate(
                p.price, 1.0, hp,
                p.risk, p.steepness,
                p.rangeAbove, p.rangeBelow, p.autoRange);

            double minEntry = p.price * 0.01;
            std::erase_if(entries, [minEntry](const EntryLevel& el) {
                return el.entryPrice < minEntry;
            });

            if (entries.empty()) {
                cr.totalProfit = 0;
                cr.savings     = 0;
                cr.nextCapital = capital;
                trace.push_back(cr);
                continue;
            }

            double totalProfit     = 0;
            double dProfit_dCap    = 0;
            double dProfit_dSurp   = 0;
            double dProfit_dFH     = 0;

            for (const auto& e : entries)
            {
                LevelRecord lr;
                lr.entryPrice = e.entryPrice;
                lr.funding    = e.funding;
                lr.quantity   = e.fundingQty;

                lr.overhead = QuantMath::overhead(
                    lr.entryPrice, lr.quantity,
                    p.feeSpread, p.feeHedging, p.deltaTime,
                    p.symbolCount, capital, p.coefficientK,
                    p.futureTradeCount);

                lr.effectiveOH = QuantMath::effectiveOverhead(
                    lr.overhead, p.surplus, p.feeSpread,
                    p.feeHedging, p.deltaTime);

                lr.tpPrice = lr.entryPrice * (1.0 + lr.effectiveOH);

                double cost = lr.entryPrice * lr.quantity;
                lr.buyFee     = cost * p.buyFeeRate;
                lr.sellFee    = lr.tpPrice * lr.quantity * p.sellFeeRate;
                lr.grossProfit = (lr.tpPrice - lr.entryPrice) * lr.quantity;
                lr.netProfit   = lr.grossProfit - lr.buyFee - lr.sellFee;

                totalProfit += lr.netProfit;

                // ---- Analytical gradients (§15) ----

                // ?EO/?s = f_h · ?t  (§15.3, constant)
                double dEO_ds = p.feeHedging * p.deltaTime;
                // ?TP/?s = entry · dEO_ds
                // ?net/?s = entry · dEO_ds · qty · (1 - sellFeeRate)
                dProfit_dSurp += lr.entryPrice * dEO_ds * lr.quantity
                                 * (1.0 - p.sellFeeRate);

                // ?OH/?T = -OH · (P/q) / D  (§15.2)
                double pq = lr.entryPrice / std::max(lr.quantity, EPS);
                double D  = pq * capital + p.coefficientK;
                double dOH_dT = (D > EPS) ? -lr.overhead * pq / D : 0.0;
                double dTP_dT = lr.entryPrice * dOH_dT;
                dProfit_dCap += dTP_dT * lr.quantity * (1.0 - p.sellFeeRate);

                // ?OH/?f_h = OH / f_h  (OH ? f_h)
                // ?EO/?f_h = OH/f_h + (s + f_s) · ?t
                double dOH_dfh = (p.feeHedging > EPS)
                    ? lr.overhead / p.feeHedging : 0.0;
                double dEO_dfh = dOH_dfh
                    + (p.surplus + p.feeSpread) * p.deltaTime;
                double dTP_dfh = lr.entryPrice * dEO_dfh;
                dProfit_dFH += dTP_dfh * lr.quantity * (1.0 - p.sellFeeRate);

                cr.levels.push_back(lr);
            }

            cr.totalProfit      = totalProfit;
            cr.savings          = totalProfit * p.savingsRate;
            cr.nextCapital      = capital + totalProfit * (1.0 - p.savingsRate);
            cr.dProfit_dCapital = dProfit_dCap;
            cr.dProfit_dSurplus = dProfit_dSurp;
            cr.dProfit_dFeeH    = dProfit_dFH;

            // Numerical gradients for parameters with complex dependencies
            cr.dProfit_dRisk  = numericalGrad(p, capital, ParamIdx::Risk);
            cr.dProfit_dAlpha = numericalGrad(p, capital, ParamIdx::Steepness);
            cr.dProfit_dRmax  = numericalGrad(p, capital, ParamIdx::MaxRisk);

            trace.push_back(cr);
            capital = cr.nextCapital;
        }

        return trace;
    }

    // ---- Backward pass: BPTT gradient computation (§15.9) ----
    static ParamGradients backward(const ChainParams& p,
                                   const std::vector<CycleRecord>& trace,
                                   ChainObjective obj)
    {
        int C = static_cast<int>(trace.size());
        if (C == 0) return {};

        ParamGradients g;
        g.objective = computeObjective(trace, p, obj);

        // ?_C = dJ/dT_C  (terminal adjoint)
        double lambda = terminalAdjoint(obj);

        double dJ_ds = 0, dJ_dr = 0, dJ_da = 0;
        double dJ_dfh = 0, dJ_dRm = 0, dJ_dSv = 0;

        double oneMinusSave = 1.0 - p.savingsRate;

        // Backward sweep: c = C-1 down to 0
        for (int c = C - 1; c >= 0; --c)
        {
            const auto& cr = trace[c];

            double dJc_dTc = 0, dJc_ds = 0, dJc_dr = 0, dJc_da = 0;
            double dJc_dfh = 0, dJc_dRm = 0, dJc_dSv = 0;
            perCycleGradients(trace, p, obj, c,
                dJc_dTc, dJc_ds, dJc_dr, dJc_da,
                dJc_dfh, dJc_dRm, dJc_dSv);

            // Accumulate: dJ/d? += ?_{c+1} · ?T_{c+1}/?? + ?J_c/??
            // ?T_{c+1}/?? = ??_c/?? · (1 - s_save)
            dJ_ds  += lambda * cr.dProfit_dSurplus * oneMinusSave + dJc_ds;
            dJ_dr  += lambda * cr.dProfit_dRisk    * oneMinusSave + dJc_dr;
            dJ_da  += lambda * cr.dProfit_dAlpha   * oneMinusSave + dJc_da;
            dJ_dfh += lambda * cr.dProfit_dFeeH    * oneMinusSave + dJc_dfh;
            dJ_dRm += lambda * cr.dProfit_dRmax    * oneMinusSave + dJc_dRm;

            // ?T_{c+1}/?s_save = -?_c
            dJ_dSv += lambda * (-cr.totalProfit) + dJc_dSv;

            // Propagate adjoint:
            // ?_c = ?_{c+1} · ?T_{c+1}/?T_c + ?J_c/?T_c
            // ?T_{c+1}/?T_c = 1 + ??_c/?T_c · (1 - s_save)
            double dTnext_dTc = 1.0 + cr.dProfit_dCapital * oneMinusSave;
            lambda = lambda * dTnext_dTc + dJc_dTc;
        }

        g.dJ_dSurplus     = dJ_ds;
        g.dJ_dRisk        = dJ_dr;
        g.dJ_dSteepness   = dJ_da;
        g.dJ_dFeeHedging  = dJ_dfh;
        g.dJ_dMaxRisk     = dJ_dRm;
        g.dJ_dSavingsRate = dJ_dSv;
        return g;
    }

    // ---- Optimize: Adam gradient ascent/descent (§16) ----
    static OptimizationResult optimize(const ChainParams& initial,
                                       ChainObjective obj,
                                       int maxSteps   = 50,
                                       double lr      = 0.001,
                                       StepCallback onStep = nullptr)
    {
        // Dispatch: price series ? simulator mode, otherwise analytical
        if (initial.hasPriceSeries())
            return optimizeSim(initial, obj, maxSteps, lr, onStep);
        return optimizeAnalytical(initial, obj, maxSteps, lr, onStep);
    }

private:

    // ---- Simulator-driven optimisation (price series mode) ----
    static OptimizationResult optimizeSim(const ChainParams& initial,
                                          ChainObjective obj,
                                          int maxSteps, double lr,
                                          const StepCallback& onStep = nullptr)
    {
        OptimizationResult res;
        ChainParams init = initial;
        applyBounds(init);
        res.initialParams = init;

        auto grad = simGradients(init, obj);
        zeroFrozenGrads(init, grad);
        res.initialGradients = grad;
        res.objectiveHistory.push_back(grad.objective);
        recordStep(res, 0, init, grad, obj, onStep);

        ChainParams cur = init;
        double sign = (obj == ChainObjective::MinSpread) ? -1.0 : 1.0;

        struct Adam { double m = 0, v = 0; };
        Adam a_s, a_r, a_a, a_fh, a_rm, a_sv;
        double beta1 = 0.9, beta2 = 0.999, eps = 1e-8;

        auto adamStep = [&](Adam& st, double g, double scale) -> double {
            st.m = beta1 * st.m + (1.0 - beta1) * g;
            st.v = beta2 * st.v + (1.0 - beta2) * g * g;
            return scale * st.m / (std::sqrt(st.v) + eps);
        };

        for (int step = 0; step < maxSteps; ++step)
        {
            double t = static_cast<double>(step + 1);
            double bc1 = 1.0 / (1.0 - std::pow(beta1, t));
            double bc2 = 1.0 / (1.0 - std::pow(beta2, t));
            double adjLR = lr * std::sqrt(bc2) * bc1;

            if (!cur.bSurplus.frozen)
                cur.surplus    += sign * adamStep(a_s,  grad.dJ_dSurplus,     adjLR);
            if (!cur.bRisk.frozen)
                cur.risk       += sign * adamStep(a_r,  grad.dJ_dRisk,        adjLR * 0.1);
            if (!cur.bSteepness.frozen)
                cur.steepness  += sign * adamStep(a_a,  grad.dJ_dSteepness,   adjLR * 0.01);
            if (!cur.bFeeHedging.frozen)
                cur.feeHedging += sign * adamStep(a_fh, grad.dJ_dFeeHedging,  adjLR * 0.1);
            if (!cur.bMaxRisk.frozen)
                cur.maxRisk    += sign * adamStep(a_rm, grad.dJ_dMaxRisk,     adjLR);
            if (!cur.bSavingsRate.frozen)
                cur.savingsRate += sign * adamStep(a_sv, grad.dJ_dSavingsRate, adjLR * 0.1);

            applyBounds(cur);

            grad = simGradients(cur, obj);
            zeroFrozenGrads(cur, grad);
            res.objectiveHistory.push_back(grad.objective);
            res.steps = step + 1;
            recordStep(res, step + 1, cur, grad, obj, onStep);

            if (res.objectiveHistory.size() >= 2) {
                double prev = res.objectiveHistory[res.objectiveHistory.size() - 2];
                double curr = res.objectiveHistory.back();
                if (std::abs(curr - prev) < EPS * std::max(1.0, std::abs(curr)))
                    break;
            }
        }

        std::cout << "  [BPTT] converged after " << res.steps << " steps\n";

        res.optimizedParams = cur;
        res.finalGradients  = grad;
        res.forwardTrace    = forwardSimFull(cur, &res.simResult);
        return res;
    }

    // ---- Analytical optimisation (no price series) ----
    static OptimizationResult optimizeAnalytical(const ChainParams& initial,
                                                  ChainObjective obj,
                                                  int maxSteps, double lr,
                                                  const StepCallback& onStep = nullptr)
    {
        OptimizationResult res;
        ChainParams init = initial;
        applyBounds(init);
        res.initialParams = init;

        // Initial forward + backward
        auto trace = forward(init);
        auto grad  = backward(init, trace, obj);
        zeroFrozenGrads(init, grad);
        res.initialGradients = grad;
        res.objectiveHistory.push_back(grad.objective);
        recordStep(res, 0, init, grad, obj, onStep);

        ChainParams cur = init;

        // Ascent for Max objectives, descent for Min
        double sign = (obj == ChainObjective::MinSpread) ? -1.0 : 1.0;

        // Adam state (per parameter)
        struct Adam { double m = 0, v = 0; };
        Adam a_s, a_r, a_a, a_fh, a_rm, a_sv;
        double beta1 = 0.9, beta2 = 0.999, eps = 1e-8;

        auto adamStep = [&](Adam& st, double g, double scale) -> double {
            st.m = beta1 * st.m + (1.0 - beta1) * g;
            st.v = beta2 * st.v + (1.0 - beta2) * g * g;
            return scale * st.m / (std::sqrt(st.v) + eps);
        };

        for (int step = 0; step < maxSteps; ++step)
        {
            double t = static_cast<double>(step + 1);
            double bc1 = 1.0 / (1.0 - std::pow(beta1, t));
            double bc2 = 1.0 / (1.0 - std::pow(beta2, t));
            double adjLR = lr * std::sqrt(bc2) * bc1;

            if (!cur.bSurplus.frozen)
                cur.surplus    += sign * adamStep(a_s,  grad.dJ_dSurplus,     adjLR);
            if (!cur.bRisk.frozen)
                cur.risk       += sign * adamStep(a_r,  grad.dJ_dRisk,        adjLR * 0.1);
            if (!cur.bSteepness.frozen)
                cur.steepness  += sign * adamStep(a_a,  grad.dJ_dSteepness,   adjLR * 0.01);
            if (!cur.bFeeHedging.frozen)
                cur.feeHedging += sign * adamStep(a_fh, grad.dJ_dFeeHedging,  adjLR * 0.1);
            if (!cur.bMaxRisk.frozen)
                cur.maxRisk    += sign * adamStep(a_rm, grad.dJ_dMaxRisk,     adjLR);
            if (!cur.bSavingsRate.frozen)
                cur.savingsRate += sign * adamStep(a_sv, grad.dJ_dSavingsRate, adjLR * 0.1);

            // Clamp to non-negotiable bounds
            applyBounds(cur);

            trace = forward(cur);
            grad  = backward(cur, trace, obj);
            zeroFrozenGrads(cur, grad);
            res.objectiveHistory.push_back(grad.objective);

            res.steps = step + 1;
            recordStep(res, step + 1, cur, grad, obj, onStep);

            // Early stop on convergence
            if (res.objectiveHistory.size() >= 2) {
                double prev = res.objectiveHistory[res.objectiveHistory.size() - 2];
                double curr = res.objectiveHistory.back();
                if (std::abs(curr - prev) < EPS * std::max(1.0, std::abs(curr)))
                    break;
            }
        }

        std::cout << "  [BPTT] converged after " << res.steps << " steps\n";

        res.optimizedParams = cur;
        res.finalGradients  = grad;
        res.forwardTrace    = trace;
        return res;
    }
};
