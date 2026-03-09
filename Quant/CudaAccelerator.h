#pragma once
// ============================================================
// CudaAccelerator.h - C++ integration layer for GPU simulation
//
// QUANT_CUDA defined  -> real API, links to CudaKernels.cu
// QUANT_CUDA absent   -> no-op stubs, CPU path used
// ============================================================

#ifdef QUANT_CUDA

#include "QuantMath.h"
#include <string>
#include <vector>
#include <algorithm>

// Must match the struct in CudaKernels.cu exactly
struct GpuSimParams
{
    double capital, price;
    double surplus, risk, steepness, feeHedging;
    double maxRisk, minRisk, savingsRate;
    double feeSpread, deltaTime;
    int    symbolCount;
    double coefficientK;
    double buyFeeRate, sellFeeRate;
    double rangeAbove, rangeBelow;
    int    futureTradeCount;
    double stopLossFraction;
    int    stopLossHedgeCount;
    int    levels, downtrendCount;
    double exitRisk, exitFraction, exitSteepness;
    double entryRisk, entrySteepness;
    int    chainCycles;
    int    exitLevels;

    int    maxTradesPerMonth;
    double capitalPumpPerMonth;
    int    autoRange;
};

// Host API (defined in CudaKernels.cu)
extern "C" {
    bool        cudaGpuInit();
    bool        cudaGpuIsAvailable();
    const char* cudaGpuDeviceName();
    void        cudaGpuSetThrottle(int pct);
    int         cudaGpuGetThrottle();
    void        cudaGpuSetMemBudget(int pct);
    int         cudaGpuGetMemBudget();
    void        cudaGpuCleanup();
    bool        cudaGpuBatchSim(
                    const GpuSimParams* configs, int numConfigs,
                    const double* timestamps, const double* prices, int numPrices,
                    int objective, double* results);
}

class CudaAccelerator
{
public:
    static bool init()                 { return cudaGpuInit(); }
    static bool isAvailable()          { return cudaGpuIsAvailable(); }
    static std::string deviceName()    { return cudaGpuDeviceName(); }
    static void setThrottle(int pct)   { cudaGpuSetThrottle(pct); }
    static int  getThrottle()          { return cudaGpuGetThrottle(); }
    static void setMemBudget(int pct)  { cudaGpuSetMemBudget(pct); }
    static int  getMemBudget()         { return cudaGpuGetMemBudget(); }
    static void cleanup()              { cudaGpuCleanup(); }
};

#else // !QUANT_CUDA

#include <string>

class CudaAccelerator
{
public:
    static bool init()               { return false; }
    static bool isAvailable()        { return false; }
    static std::string deviceName()  { return ""; }
    static void setThrottle(int)     {}
    static int  getThrottle()        { return 0; }
    static void setMemBudget(int)    {}
    static int  getMemBudget()       { return 0; }
    static void cleanup()            {}
};

#endif // QUANT_CUDA
