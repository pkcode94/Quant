# LLM Chat Training System — Architecture Roadmap

## Executive Summary

Integrate a **teachable chat interface** into the Quant trading platform where a
local DeepSeek-derived model can be conversed with, corrected by the trader, and
fine-tuned via CUDA backpropagation — all without leaving the app UI.

---

## Current State Audit

### ✅ What Is Functioning

| Component | Location | Status |
|---|---|---|
| **CUDA infrastructure** | `CudaKernels.cu` | Fully operational — batch simulator, VRAM budget, throttle |
| **GpuLLM forward pass** | `GpuLLM.cu` | Complete — embedding, LayerNorm, causal MHA, LSTM router, MoE FFN, output head |
| **GpuLLM backward pass** | `GpuLLM.cu` | Partial — gradient flows through output head, LN, residual, expert FFN. Attention backward is approximate (skips softmax(QK^T/√d)V backprop). LSTM router backward kernel exists but is not wired into the main `llmBackward()` |
| **Adam optimiser** | `GpuLLM.cu` | Complete — bias-corrected Adam on GPU |
| **VRAM guardrails** | `GpuLLM.cu` | Complete — `llmCheckVram()` pre-flight before any allocation |
| **Weight persistence** | `GpuLLM.cu` | Complete — binary save/load with config header |
| **LSTM state persistence** | `GpuLLM.cu` | Complete — save/load/reset for online inference |
| **Discussion Ledger** | `DiscussionLedger.h` | Complete — trade-linked discussions with post-mortem, JSON persistence |
| **DeepSeek.cpp inference** | `deepseek.cpp-main/` | Complete CPU-only inference — MHA, MLA, MoE, RoPE, tokenizer, sampler, perplexity |
| **Web UI framework** | `Routes_*.h`, `httplib` | Complete — all trade/simulator/chart routes, HTML helpers |

### ⚠️ What Needs Improvement / Is Missing

| Gap | Severity | Notes |
|---|---|---|
| **No text tokenizer in Quant** | Blocker | GpuLLM uses raw `double*` vectors. No char→token→embedding pipeline exists |
| **No chat forward pass** | Blocker | GpuLLM outputs a single scalar (next-price). Chat needs vocab-sized logit output + sampling |
| **Attention backward incomplete** | High | `llmBackward()` skips softmax(QK^T)V gradient — uses approximate pass-through. Fine-tuning quality will suffer |
| **LSTM router backward not wired** | High | `llmLstmGateBwdKernel` exists but is never called in `llmBackward()` |
| **No RLHF / correction pipeline** | High | No mechanism to convert "user corrects response" into a training signal |
| **No chat UI route** | Medium | No `/chat` endpoint or WebSocket for streaming tokens |
| **DeepSeek.cpp is CPU-only** | Medium | All matmuls are CPU (`_matmul` with OpenMP). No CUDA kernels |
| **GELU backward uses post-activation** | Low | `expert_mid` caches post-GELU values; backward needs pre-GELU. Approximation is acceptable for early work |

---

## Architecture: Three-Phase Plan

### Phase 1: Tokenization & Vocabulary Pipeline (Start Here)
**Goal:** Map text ↔ numbers so the GPU model can process language.

```
┌─────────────┐     ┌──────────────┐     ┌──────────────┐
│ Raw text     │────▶│ Tokenizer    │────▶│ Token IDs    │
│ "Buy BTC"   │     │ (BPE/char)   │     │ [42,1337,89] │
└─────────────┘     └──────────────┘     └──────────────┘
                           │
                    ┌──────┴──────┐
                    │ Vocabulary  │
                    │ vocab.json  │
                    │ 32K-64K tok │
                    └─────────────┘
```

**Three encoding levels (incremental):**

| Level | What | Vocab Size | Complexity | When |
|---|---|---|---|---|
| **L0: Character** | Every ASCII/UTF-8 byte is a token | 256 | Trivial | Phase 1a |
| **L1: Word dictionary** | Common trading terms + character fallback | ~2K-5K | Low | Phase 1b |
| **L2: BPE (full)** | Byte-Pair Encoding (DeepSeek-compatible) | 32K-128K | Medium | Phase 1c |

**Deliverables:**
- `Tokenizer.h` — header-only, `encode(string) → vector<int>`, `decode(vector<int>) → string`
- `VocabBuilder.h` — build BPE vocabulary from corpus
- Token embedding table integration with `GpuLLM.h` (`LlmConfig::vocabSize` field)
- Modify `llmInit` to allocate embedding table `[vocabSize × dModel]`

**Start with L0 (character-level).** It works immediately, trains faster on small
data, and the architecture is identical — just swap the embedding table size later.

### Phase 2: Chat Forward Pass & CUDA Inference
**Goal:** Make GpuLLM produce text, not just scalars.

```
┌──────────┐    ┌────────────┐    ┌──────────────┐    ┌──────────┐
│ Token IDs│───▶│ GpuLLM     │───▶│ Logits       │───▶│ Sampler  │
│ [42,1337]│    │ Forward    │    │ [vocabSize]  │    │ top-p/T  │
└──────────┘    │ (CUDA)     │    └──────────────┘    └────┬─────┘
                └────────────┘                             │
                                                    ┌─────▼─────┐
                                                    │ Next token│
                                                    │ ID = 89   │
                                                    └───────────┘
```

**Changes to GpuLLM:**

| Change | File | Description |
|---|---|---|
| Config expansion | `GpuLLM.h` | Add `vocabSize`, `useTokenEmbedding` fields to `LlmConfig` |
| Token embedding lookup kernel | `GpuLLM.cu` | `llmTokenEmbedKernel` — index into `[vocabSize × dModel]` table |
| Output head → vocab logits | `GpuLLM.cu` | Change output projection from `[dModel → outputDim=1]` to `[dModel → vocabSize]` |
| Softmax + sampling | `GpuLLM.cu` or host | Top-p / temperature sampling over vocab logits |
| Autoregressive loop | Host C++ | Feed predicted token back as next input, accumulate output |
| New API function | `GpuLLM.h` | `llmChat(const int* tokenIds, int seqLen, int* outputIds, int maxOut)` |

### Phase 3: CUDA Backpropagation & Correction Training
**Goal:** User corrects a response → model learns from the correction.

```
┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│ Model says:  │    │ User says:   │    │ Training     │
│ "sell at 50k"│───▶│ "no, hold"   │───▶│ signal       │
└──────────────┘    └──────────────┘    └──────┬───────┘
                                               │
                    ┌──────────────────────────▼───────┐
                    │  Cross-entropy loss on correct   │
                    │  token sequence vs model output  │
                    │  → backward() → Adam update      │
                    └──────────────────────────────────┘
```

**Sub-phases:**

| Sub-phase | What | Key Work |
|---|---|---|
| **3a: Fix attention backward** | Complete the softmax(QK^T/√d)V gradient | Implement `dQ`, `dK`, `dV` from `dAttnOut` through saved attention scores |
| **3b: Wire LSTM backward** | Connect `llmLstmGateBwdKernel` into `llmBackward()` BPTT loop | Reverse time-step loop, accumulate `dc_prev`, `dh_prev`, backprop through `Wih`, `Whh` |
| **3c: Cross-entropy loss** | Replace MSE with cross-entropy for language modelling | `loss = -sum(target_onehot * log(softmax(logits)))` |
| **3d: Correction pipeline** | Convert user correction into teacher-forced training pair | `input = [prompt + model_output_prefix]`, `target = [correct_continuation]` |
| **3e: Chat training loop** | Wire correction → `llmTrainStep` → weight update | Single-step or mini-batch, save weights after N corrections |

### Phase 4: Chat UI (can run in parallel with Phase 2-3)
**Goal:** In-app chat window accessible from the Quant web UI.

```
┌─────────────────────────────────────────────┐
│  Quant Trading Platform                     │
│  ┌────────────────────────────────────────┐ │
│  │ 💬 Chat with Model                    │ │
│  │                                        │ │
│  │ You: What should I do with BTC?        │ │
│  │ Bot: Based on the current trend...     │ │
│  │                                        │ │
│  │ [✏️ Correct] [👍 Good] [📊 Context]    │ │
│  │                                        │ │
│  │ ┌──────────────────────┐ [Send]       │ │
│  │ │ Type a message...    │              │ │
│  │ └──────────────────────┘              │ │
│  └────────────────────────────────────────┘ │
└─────────────────────────────────────────────┘
```

**Deliverables:**
- `Routes_Chat.h` — GET `/chat` (UI), POST `/chat/send` (inference), POST `/chat/correct` (training)
- Conversation history stored via `DiscussionLedger` (already built)
- Streaming token output via chunked HTTP or SSE
- Correction button triggers `llmTrainStep` with the corrected sequence

---

## Recommended Implementation Order

```
Week 1-2:  Phase 1a — Character-level tokenizer (encode/decode)
           Phase 4  — Basic chat UI skeleton (GET /chat, POST /chat/send)
                      Hook up to a dummy echo responder first

Week 3-4:  Phase 2  — Token embedding kernel, vocab-sized output head
                      Autoregressive generation loop
                      Wire chat UI to real GpuLLM inference

Week 5-6:  Phase 1b — Word dictionary tokenizer (trading vocabulary)
           Phase 3a — Fix attention backward (dQ, dK, dV)
           Phase 3b — Wire LSTM backward into llmBackward()

Week 7-8:  Phase 3c — Cross-entropy loss kernel
           Phase 3d — Correction pipeline (UI correction → training pair)
           Phase 3e — Chat training loop + weight save

Week 9+:   Phase 1c — BPE tokenizer (DeepSeek vocab compatibility)
           Phase 3f — LoRA / adapter layers (optional, for efficient fine-tuning)
           DeepSeek weight import (convert .yalm → GpuLLM binary format)
```

---

## Key Design Decisions

### 1. Start Character-Level, Not BPE
- BPE requires a pre-trained vocabulary and merge table
- Character-level (256 tokens) works immediately, same architecture
- Upgrade to BPE is just swapping the embedding table + tokenizer

### 2. Keep GpuLLM Architecture (Don't Port DeepSeek Directly)
- GpuLLM already has CUDA forward/backward, VRAM management, Adam
- DeepSeek.cpp is CPU-only inference, no backward pass, no training
- Use DeepSeek as **reference** for architecture details (RoPE, MLA, etc.)
- Long-term: import DeepSeek pre-trained weights into GpuLLM format

### 3. Training Signal: Supervised Correction, Not RLHF
- RLHF requires a reward model — too complex for Phase 1
- Instead: user types the correct response → teacher-forced cross-entropy
- This is equivalent to supervised fine-tuning (SFT) on corrections
- Can add DPO/RLHF later as a Phase 5

### 4. Conversations Map to DiscussionLedger
- Each chat session = a Discussion
- Model responses + user corrections = DiscussionMessages
- Trade context can be linked via `tradeIds`
- Post-mortem analysis already supported

---

## File Plan

| New File | Purpose |
|---|---|
| `Tokenizer.h` | Character/word/BPE tokenizer — `encode()`, `decode()` |
| `VocabBuilder.h` | Build BPE vocabulary from text corpus |
| `Routes_Chat.h` | Chat UI routes — `/chat`, `/chat/send`, `/chat/correct` |
| `ChatSession.h` | Manages conversation state, token history, generation loop |
| `LlmChatConfig.h` | Extended config for chat mode (vocabSize, maxGenLen, temperature) |

| Modified File | Changes |
|---|---|
| `GpuLLM.h` | Add `vocabSize` to `LlmConfig`, add `llmChat()` API |
| `GpuLLM.cu` | Token embedding kernel, cross-entropy loss, fix attention backward, wire LSTM backward |
| `AppContext.h` | Add chat session state |
| `Quant.cpp` | Register chat routes |

---

## Where Do We Begin?

**Phase 1a: Character-Level Tokenizer.** This is the foundation — without it,
nothing else can work. It is also self-contained (one header file), testable
independently, and unblocks all subsequent phases.

Shall I implement `Tokenizer.h` with character-level encode/decode now?
