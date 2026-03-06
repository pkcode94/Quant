// ============================================================
// CudaKernels.cu  -  GPU-accelerated batch simulator for BPTT
// ============================================================
//
// Architecture:
//   - Persistent device buffers (allocated once, reused every step)
//   - Grid-stride kernel loop (supports arbitrary N configs)
//   - Coalesced reads via __restrict__ + read-only cache (__ldg)
//   - Configurable GPU throttle (0-100 %) keeps the desktop
//     responsive while long BPTT runs execute
//   - CUDA_CHK macro for per-call error diagnostics
//
// Compile:  nvcc -c CudaKernels.cu  (or CUDA 13.1 Build Customization)
// ============================================================

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <math.h>
#include <float.h>
#include <stdio.h>

// ============================================================
// Error-checking macro
// ============================================================

#define CUDA_CHK(call)                                                   \
    do {                                                                 \
        cudaError_t _e = (call);                                         \
        if (_e != cudaSuccess) {                                         \
            fprintf(stderr, "[CUDA] %s:%d  %s  ->  %s\n",               \
                    __FILE__, __LINE__, #call, cudaGetErrorString(_e));   \
            return false;                                                \
        }                                                                \
    } while (0)

// ============================================================
// Capacity limits  (tuned for register pressure vs. flexibility)
// ============================================================

static constexpr int GPU_MAX_LEVELS    = 32;
static constexpr int GPU_MAX_POSITIONS = 64;
static constexpr int GPU_MAX_EXITS     = 16;
static constexpr int GPU_MAX_CYCLES    = 64;

// ============================================================
// GPU data structures  (POD, no std::)
// ============================================================

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
};

static constexpr int OBJ_MAX_PROFIT = 1;
static constexpr int OBJ_MIN_SPREAD = 2;
static constexpr int OBJ_MAX_ROI    = 3;
static constexpr int OBJ_MAX_CHAIN  = 4;
static constexpr int OBJ_MAX_WEALTH = 5;

// ============================================================
// Device math
// ============================================================

__device__ __forceinline__ double gpu_sigmoid(double x)
{ return 1.0 / (1.0 + exp(-x)); }

__device__ __forceinline__ double gpu_clamp01(double v)
{ return fmin(fmax(v, 0.0), 1.0); }

__device__ __forceinline__ double gpu_sigmoidNorm(double t, double alpha)
{
    double s0 = gpu_sigmoid(-alpha * 0.5);
    double s1 = gpu_sigmoid( alpha * 0.5);
    double S  = s1 - s0;
    if (S < 1e-15) return t;
    return (gpu_sigmoid(alpha * (t - 0.5)) - s0) / S;
}

__device__ __forceinline__ double gpu_overhead(
    double P, double q, double fs, double fh, double dt,
    int ns, double T, double K, int nf)
{
    double F     = fs * fh * dt;
    double denom = (P / q) * T + K;
    if (denom < 1e-15) return 0.0;
    return F * ns * (1.0 + nf) / denom;
}

__device__ __forceinline__ double gpu_effectiveOH(
    double oh, double s, double fs, double fh, double dt)
{ return oh + (s + fs) * fh * dt; }

__device__ __forceinline__ double gpu_horizonFactor(
    double rawOH, double eo, double maxRisk, double steep, int i, int N)
{
    if (maxRisk > 0.0)
    {
        double lo = rawOH;
        double hi = (maxRisk > lo) ? maxRisk : lo;
        double s  = (steep > 0.0) ? steep : 0.01;
        double t  = (N > 1) ? (double)i / (double)(N - 1) : 1.0;
        return lo + gpu_sigmoidNorm(t, s) * (hi - lo);
    }
    return eo * (double)(i + 1);
}

__device__ __forceinline__ double gpu_positionDelta(double P, double q, double T)
{ return (T > 1e-15) ? (P * q) / T : 0.0; }

__device__ __forceinline__ double gpu_sigmoidBuffer(
    double delta, double lower, double upper, int count)
{
    if (count <= 0 || delta <= 0.0) return 1.0;
    double t     = delta / (delta + 1.0);
    double alpha = (delta > 0.1) ? delta : 0.1;
    return 1.0 + count * (lower + gpu_sigmoidNorm(t, alpha) * (upper - lower));
}

// ============================================================
// Device entry-level generator
// ============================================================

struct GpuEntryLevel { double entryPrice, funding, qty; };

__device__ int gpu_generateEntries(
    const GpuSimParams& cfg, double currentPrice, double capital,
    GpuEntryLevel* out)
{
    int N = cfg.levels;
    if (N < 1) N = 1;
    if (N > GPU_MAX_LEVELS) N = GPU_MAX_LEVELS;

    double steep = fmax(cfg.entrySteepness, 0.1);
    double risk  = gpu_clamp01(cfg.entryRisk);

    double pLow, pHigh;
    if (cfg.rangeAbove > 0 || cfg.rangeBelow > 0)
    { pLow = fmax(currentPrice - cfg.rangeBelow, 1e-10); pHigh = currentPrice + cfg.rangeAbove; }
    else
    { pLow = 0.0; pHigh = currentPrice; }

    double weights[GPU_MAX_LEVELS];
    double totalW = 0.0;
    for (int i = 0; i < N; ++i)
    {
        double t   = (N > 1) ? (double)i / (double)(N - 1) : 1.0;
        double sig = gpu_sigmoidNorm(t, steep);
        out[i].entryPrice = pLow + sig * (pHigh - pLow);
        double w = (1.0 - risk) * sig + risk * (1.0 - sig);
        weights[i] = w;  totalW += w;
    }
    if (totalW < 1e-15) totalW = 1.0;
    for (int i = 0; i < N; ++i)
    {
        out[i].funding = capital * weights[i] / totalW;
        out[i].qty = (out[i].entryPrice > 1e-15) ? out[i].funding / out[i].entryPrice : 0.0;
    }
    double minE = currentPrice * 0.01;
    int kept = 0;
    for (int i = 0; i < N; ++i)
        if (out[i].entryPrice >= minE) { if (kept != i) out[kept] = out[i]; ++kept; }
    return kept;
}

// ============================================================
// Device exit-level generator
// ============================================================

struct GpuExitLevel { double tpPrice, sellQty, grossProfit; };

__device__ int gpu_generateExits(
    const GpuSimParams& cfg, double entryPrice, double qty,
    double buyFee, double entryCost, GpuExitLevel* out)
{
    int N = cfg.levels;
    if (N < 1) N = 1;
    if (N > GPU_MAX_EXITS) N = GPU_MAX_EXITS;

    double frac    = gpu_clamp01(cfg.exitFraction);
    double risk    = gpu_clamp01(cfg.exitRisk);
    double steep   = fmax(cfg.exitSteepness, 0.01);
    double sellable = qty * frac;

    double oh = gpu_overhead(entryPrice, qty, cfg.feeSpread, cfg.feeHedging,
                             cfg.deltaTime, cfg.symbolCount, entryCost,
                             cfg.coefficientK, cfg.futureTradeCount);
    double eo = gpu_effectiveOH(oh, cfg.surplus, cfg.feeSpread,
                                cfg.feeHedging, cfg.deltaTime);

    double cumSigma[GPU_MAX_EXITS + 1];
    double center = risk * (double)(N - 1);
    for (int i = 0; i <= N; ++i)
        cumSigma[i] = gpu_sigmoid(steep * ((double)i - 0.5 - center));
    double lo = cumSigma[0], hi = cumSigma[N];
    for (int i = 0; i <= N; ++i)
        cumSigma[i] = (hi > lo) ? (cumSigma[i] - lo) / (hi - lo)
                                : (double)i / (double)N;

    double delta = gpu_positionDelta(entryPrice, qty, entryCost);
    double lower = cfg.minRisk;
    double upper = (cfg.maxRisk > 0.0) ? cfg.maxRisk : eo;
    if (upper < lower) upper = lower;
    double dtBuf = (cfg.downtrendCount > 0)
                 ? gpu_sigmoidBuffer(delta, lower, upper, cfg.downtrendCount) : 1.0;
    double slFrac = gpu_clamp01(cfg.stopLossFraction);
    double slBuf  = gpu_sigmoidBuffer(delta, lower * slFrac, upper * slFrac,
                                      cfg.stopLossHedgeCount);
    double combinedBuf = dtBuf * slBuf;

    for (int i = 0; i < N; ++i)
    {
        double tp = entryPrice * (1.0 + gpu_horizonFactor(oh, eo, cfg.maxRisk, steep, i, N));
        if (combinedBuf > 1.0) tp *= combinedBuf;
        double sf = cumSigma[i + 1] - cumSigma[i];
        double sq = sellable * sf;
        out[i] = { tp, sq, (tp - entryPrice) * sq };
    }
    return N;
}

// ============================================================
// Main simulation kernel  (grid-stride loop)
//
// Each logical thread runs one complete simulation instance.
// The grid-stride pattern means if N > blockDim*gridDim,
// threads loop back to pick up remaining work items.
// ============================================================

__global__ void batchSimKernel(
    const GpuSimParams* __restrict__ configs,
    const double*       __restrict__ prices,
    int                              numPrices,
    int                              objective,
    double*             __restrict__ results,
    int                              numConfigs)
{
    for (int tid = blockIdx.x * blockDim.x + threadIdx.x;
         tid < numConfigs;
         tid += blockDim.x * gridDim.x)
    {
        // Coalesced struct read: copy entire config into registers
        const GpuSimParams cfg = configs[tid];

        double capital = cfg.capital, realized = 0.0, savings = 0.0;
        int    cycle   = 0;

        GpuEntryLevel entries[GPU_MAX_LEVELS];
        bool          entryFilled[GPU_MAX_LEVELS];
        int           numEntries = 0;

        struct Pos {
            double entryPrice, qty, remaining, buyFee;
            int    cycle;
            GpuExitLevel exits[GPU_MAX_EXITS];
            bool         exitFilled[GPU_MAX_EXITS];
            int          numExits;
        };
        Pos positions[GPU_MAX_POSITIONS];
        int numPos = 0;

        double cycCap[GPU_MAX_CYCLES], cycProf[GPU_MAX_CYCLES];
        int maxCyc = 0;
        for (int i = 0; i < GPU_MAX_CYCLES; ++i) { cycCap[i] = 0; cycProf[i] = 0; }
        cycCap[0] = capital;

        // __ldg: read-only data cache path for price series
        double entryRefPrice = __ldg(&prices[0]);
        numEntries = gpu_generateEntries(cfg, entryRefPrice, capital, entries);
        for (int i = 0; i < numEntries; ++i) entryFilled[i] = false;

        // ---- Main price loop ----
        for (int pi = 0; pi < numPrices; ++pi)
        {
            double price = __ldg(&prices[pi]);

            // --- entries: limit buys fill on drop, breakout buys fill on rise ---
            for (int ei = 0; ei < numEntries; ++ei)
            {
                if (entryFilled[ei]) continue;
                bool belowRef = (entries[ei].entryPrice <= entryRefPrice);
                if (belowRef) {
                    if (price >= entries[ei].entryPrice) continue;  // limit: wait for drop
                } else {
                    if (price <= entries[ei].entryPrice) continue;  // breakout: wait for rise
                }
                if (entries[ei].qty < 1e-15 || numPos >= GPU_MAX_POSITIONS) continue;
                double cost = entries[ei].entryPrice * entries[ei].qty;
                double fee  = cost * cfg.buyFeeRate;
                if (cost + fee > capital) continue;
                capital -= (cost + fee);
                Pos& p = positions[numPos];
                p.entryPrice = entries[ei].entryPrice;
                p.qty = entries[ei].qty; p.remaining = p.qty;
                p.buyFee = fee; p.cycle = cycle;
                p.numExits = gpu_generateExits(cfg, p.entryPrice, p.qty, fee, cost, p.exits);
                for (int x = 0; x < p.numExits; ++x) p.exitFilled[x] = false;
                ++numPos;
                entryFilled[ei] = true;
            }

            // --- exits ---
            for (int j = 0; j < numPos; ++j)
            {
                Pos& p = positions[j];
                if (p.remaining < 1e-15) continue;
                for (int li = 0; li < p.numExits; ++li)
                {
                    if (p.exitFilled[li] || p.exits[li].sellQty < 1e-15) continue;
                    if (price < p.exits[li].tpPrice || p.remaining < 1e-15) continue;
                    double sq = fmin(p.exits[li].sellQty, p.remaining);
                    double sf = p.exits[li].tpPrice * sq * cfg.sellFeeRate;
                    double net = (p.exits[li].tpPrice - p.entryPrice) * sq - sf;
                    p.remaining -= sq;
                    capital += p.exits[li].tpPrice * sq - sf;
                    realized += net;
                    if (p.cycle < GPU_MAX_CYCLES) cycProf[p.cycle] += net;
                    p.exitFilled[li] = true;
                }
            }

            // --- chain transition ---
            if (cfg.chainCycles)
            {
                bool allClosed = true, hadPos = false, anyFilled = false;
                for (int i = 0; i < numPos; ++i)
                    if (positions[i].cycle == cycle)
                    { hadPos = true; if (positions[i].remaining > 1e-15) { allClosed = false; break; } }
                for (int i = 0; i < numEntries; ++i)
                    if (entryFilled[i]) { anyFilled = true; break; }
                if (hadPos && allClosed && anyFilled && capital > 1e-15)
                {
                    double cp = cycProf[cycle < GPU_MAX_CYCLES ? cycle : GPU_MAX_CYCLES - 1];
                    if (cp > 0 && cfg.savingsRate > 0)
                    { double sv = cp * cfg.savingsRate; savings += sv; capital -= sv; }
                    ++cycle;
                    if (cycle < GPU_MAX_CYCLES) cycCap[cycle] = capital;
                    if (cycle > maxCyc) maxCyc = cycle;
                    numEntries = gpu_generateEntries(cfg, price, capital, entries);
                    entryRefPrice = price;
                    for (int i = 0; i < numEntries; ++i) entryFilled[i] = false;
                }
            }
        }

        // --- objective ---
        int nc = maxCyc + 1;
        double obj = 0.0;
        switch (objective)
        {
        case OBJ_MAX_PROFIT:
            for (int c = 0; c < nc; ++c) obj += cycProf[c]; break;
        case OBJ_MIN_SPREAD:
            for (int i = 0; i < numPos; ++i)
                for (int li = 0; li < positions[i].numExits; ++li)
                    if (positions[i].exitFilled[li])
                    { double sp = (positions[i].exits[li].tpPrice - positions[i].entryPrice)
                                / fmax(positions[i].entryPrice, 1e-15); obj -= sp * sp; }
            break;
        case OBJ_MAX_ROI:
            for (int c = 0; c < nc; ++c) obj += cycProf[c];
            obj = (cfg.capital > 1e-15) ? obj / cfg.capital : 0; break;
        case OBJ_MAX_CHAIN:
        { double prod = 1.0;
            for (int c = 0; c < nc; ++c)
                prod *= 1.0 + cycProf[c] * (1.0 - cfg.savingsRate) / fmax(cycCap[c], 1e-15);
            obj = prod; } break;
        case OBJ_MAX_WEALTH:
            obj = capital + savings; break;
        default: obj = realized; break;
        }
        results[tid] = obj;
    }
}


// ============================================================
// Persistent buffer manager
//
// Pre-allocates device memory once.  Buffers grow monotonically
// and are reused across all cudaGpuBatchSim() calls, so the
// hot loop never touches cudaMalloc / cudaFree.
// ============================================================

struct GpuBuffers
{
    GpuSimParams* d_configs  = nullptr;
    double*       d_prices   = nullptr;
    double*       d_results  = nullptr;
    int           capConfigs = 0;
    int           capPrices  = 0;
    int           capResults = 0;

    // Returns total device bytes currently held by these buffers
    size_t bytesUsed() const
    {
        return (size_t)capConfigs * sizeof(GpuSimParams)
             + (size_t)capPrices  * sizeof(double)
             + (size_t)capResults * sizeof(double);
    }

    bool ensure(int nConfigs, int nPrices, size_t vramBudget)
    {
        // Compute bytes we WOULD need after this grow
        int wantConfigs = (nConfigs > capConfigs) ? nConfigs + 64  : capConfigs;
        int wantPrices  = (nPrices  > capPrices)  ? nPrices + 1024 : capPrices;
        int wantResults = (nConfigs > capResults)  ? nConfigs + 64  : capResults;

        size_t need = (size_t)wantConfigs * sizeof(GpuSimParams)
                    + (size_t)wantPrices  * sizeof(double)
                    + (size_t)wantResults * sizeof(double);

        if (vramBudget > 0 && need > vramBudget)
        {
            fprintf(stderr, "[CUDA] Memory request %zu MB exceeds budget %zu MB - rejecting\n",
                    need >> 20, vramBudget >> 20);
            return false;
        }

        if (nConfigs > capConfigs)
        {
            if (d_configs) cudaFree(d_configs);
            CUDA_CHK(cudaMalloc(&d_configs, wantConfigs * sizeof(GpuSimParams)));
            capConfigs = wantConfigs;
        }
        if (nPrices > capPrices)
        {
            if (d_prices) cudaFree(d_prices);
            CUDA_CHK(cudaMalloc(&d_prices, wantPrices * sizeof(double)));
            capPrices = wantPrices;
        }
        if (nConfigs > capResults)
        {
            if (d_results) cudaFree(d_results);
            CUDA_CHK(cudaMalloc(&d_results, wantResults * sizeof(double)));
            capResults = wantResults;
        }
        return true;
    }

    void release()
    {
        if (d_configs) { cudaFree(d_configs); d_configs = nullptr; capConfigs = 0; }
        if (d_prices)  { cudaFree(d_prices);  d_prices  = nullptr; capPrices  = 0; }
        if (d_results) { cudaFree(d_results); d_results = nullptr; capResults = 0; }
    }
};


// ============================================================
// Host API
// ============================================================

extern "C" {

static bool       g_cudaReady      = false;
static int        g_deviceId       = -1;
static char       g_deviceName[256] = {};
static int        g_smCount        = 0;
static size_t     g_totalVRAM      = 0;
static GpuBuffers g_buf;

// Throttle: 0-100%.  100 = full GPU.  50 = half SMs.  0 = GPU disabled at runtime.
static int        g_throttlePct    = 100;
// Memory budget: 0-100% of total VRAM.  Default 50% to leave room for desktop/display.
static int        g_memBudgetPct   = 50;

bool cudaGpuInit()
{
    int count = 0;
    cudaError_t err = cudaGetDeviceCount(&count);
    if (err != cudaSuccess || count == 0) { g_cudaReady = false; return false; }

    g_deviceId = 0;
    cudaSetDevice(g_deviceId);

    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, g_deviceId);
    g_smCount  = prop.multiProcessorCount;
    g_totalVRAM = prop.totalGlobalMem;
    snprintf(g_deviceName, sizeof(g_deviceName), "%s (SM %d.%d, %d SMs, %d MB)",
             prop.name, prop.major, prop.minor,
             g_smCount, (int)(g_totalVRAM >> 20));

    g_cudaReady = true;
    return true;
}

bool cudaGpuIsAvailable()   { return g_cudaReady; }
const char* cudaGpuDeviceName() { return g_deviceName; }

void cudaGpuSetThrottle(int pct)
{ g_throttlePct = (pct < 0) ? 0 : (pct > 100) ? 100 : pct; }

int cudaGpuGetThrottle() { return g_throttlePct; }

void cudaGpuSetMemBudget(int pct)
{ g_memBudgetPct = (pct < 1) ? 1 : (pct > 100) ? 100 : pct; }

int cudaGpuGetMemBudget() { return g_memBudgetPct; }

void cudaGpuCleanup()
{
    g_buf.release();
    if (g_cudaReady) { cudaDeviceReset(); g_cudaReady = false; }
}

// Batch-simulate N parameter configs in parallel.
// Persistent buffers: no cudaMalloc/Free in the hot path.
// Timestamps parameter kept for API compat but unused by kernel.
bool cudaGpuBatchSim(
    const GpuSimParams* hostConfigs, int numConfigs,
    const double* /*hostTimestamps*/, const double* hostPrices, int numPrices,
    int objective, double* hostResults)
{
    if (!g_cudaReady || numConfigs <= 0 || numPrices <= 0) return false;
    if (g_throttlePct <= 0) return false;

    size_t vramBudget = (g_totalVRAM * (size_t)g_memBudgetPct) / 100;
    if (!g_buf.ensure(numConfigs, numPrices, vramBudget)) return false;

    CUDA_CHK(cudaMemcpy(g_buf.d_configs, hostConfigs,
                        numConfigs * sizeof(GpuSimParams), cudaMemcpyHostToDevice));
    CUDA_CHK(cudaMemcpy(g_buf.d_prices, hostPrices,
                        numPrices * sizeof(double), cudaMemcpyHostToDevice));

    // Launch geometry: 64 threads/block (each thread is register-heavy).
    // Throttle caps the number of blocks to limit SM occupancy.
    int tpb         = 64;
    if (numConfigs < tpb) tpb = numConfigs;
    int maxBlocks   = (numConfigs + tpb - 1) / tpb;
    int smBudget    = (g_smCount * g_throttlePct + 99) / 100;
    if (smBudget < 1) smBudget = 1;
    int blocks      = (maxBlocks < smBudget) ? maxBlocks : smBudget;

    batchSimKernel<<<blocks, tpb>>>(
        g_buf.d_configs, g_buf.d_prices, numPrices,
        objective, g_buf.d_results, numConfigs);

    CUDA_CHK(cudaDeviceSynchronize());
    CUDA_CHK(cudaGetLastError());

    CUDA_CHK(cudaMemcpy(hostResults, g_buf.d_results,
                        numConfigs * sizeof(double), cudaMemcpyDeviceToHost));
    return true;
}

}  // extern "C"
