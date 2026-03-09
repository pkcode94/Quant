#pragma once
// ============================================================
// GpuLLM.h - MoE-Transformer with LSTM-gated routing
//
// Architecture:
//   Sparse Mixture-of-Experts Transformer with LSTM-gated
//   expert routing.  Combines GPT-style causal attention,
//   LSTM temporal memory, and MoE conditional compute.
//
// Offline/Online hybrid:
//   Train offline on historical price series (full BPTT).
//   Online inference carries LSTM hidden state forward with
//   each new tick - no re-training required.
//
// QUANT_CUDA defined  -> real API, links to GpuLLM.cu
// QUANT_CUDA absent   -> no-op stubs
//
// NOTE: Enzyme AD (C:\Users\patri\Desktop\q\Enzyme-main)
//   requires Clang as compiler.  This project uses MSVC + nvcc,
//   so backward kernels are implemented manually.  If the
//   toolchain is switched to Clang in future, replace manual
//   backward kernels with __enzyme_autodiff() calls and add
//   -fplugin=ClangEnzyme.dll to the Clang invocation.
// ============================================================

struct LlmConfig
{
    int inputDim   = 8;      // raw feature vector size (OHLCV + indicators)
    int dModel     = 256;    // embedding / hidden dimension
    int nHeads     = 4;      // multi-head attention heads
    int nLayers    = 6;      // transformer decoder blocks
    int nExperts   = 8;      // MoE experts per layer
    int topK       = 2;      // experts active per token
    int lstmHidden = 128;    // LSTM router hidden dim
    int seqLen     = 512;    // max context window
    int ffnDim     = 512;    // expert FFN intermediate dim
    int outputDim  = 1;      // output vector size (e.g. 1 for next-price)
};

struct LlmTrainResult
{
    double loss;
    int    step;
    double gradNorm;
};

#ifdef QUANT_CUDA

extern "C" {

// Lifecycle
bool        llmInit(const LlmConfig* cfg);
void        llmCleanup();
bool        llmIsReady();
const char* llmStatusString();
long long   llmParamCount();
long long   llmVramBytes();
long long   llmEstimateVramBytes(const LlmConfig* cfg);

// Weights
void llmInitWeights();
bool llmSaveWeights(const char* path);
bool llmLoadWeights(const char* path);

// Training  (single step: forward -> loss -> backward -> Adam update)
bool llmTrainStep(const double* inputSeq, const double* targets,
                  int batchSize, int seqLen, double lr,
                  LlmTrainResult* result);

// Inference (forward only, LSTM state carries across calls)
bool llmInfer(const double* inputSeq, int seqLen, double* output);

// LSTM state management for online mode
void llmResetLstmState();
bool llmSaveLstmState(const char* path);
bool llmLoadLstmState(const char* path);

}

#else // !QUANT_CUDA  -- stubs

inline bool        llmInit(const LlmConfig*)        { return false; }
inline void        llmCleanup()                      {}
inline bool        llmIsReady()                      { return false; }
inline const char* llmStatusString()                 { return "no GPU"; }
inline long long   llmParamCount()                   { return 0; }
inline long long   llmVramBytes()                    { return 0; }
inline long long   llmEstimateVramBytes(const LlmConfig*) { return 0; }
inline void        llmInitWeights()                  {}
inline bool        llmSaveWeights(const char*)       { return false; }
inline bool        llmLoadWeights(const char*)       { return false; }
inline bool        llmTrainStep(const double*, const double*,
                                int, int, double, LlmTrainResult*) { return false; }
inline bool        llmInfer(const double*, int, double*)            { return false; }
inline void        llmResetLstmState()               {}
inline bool        llmSaveLstmState(const char*)     { return false; }
inline bool        llmLoadLstmState(const char*)     { return false; }

#endif // QUANT_CUDA
