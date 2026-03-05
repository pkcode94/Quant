# Sigmoid-Governed Fee Hedging and Serial Position Management

## A Deterministic Framework for Overhead-Neutral Cyclic Trading

**Abstract.** This paper describes a deterministic mathematical framework for constructing, funding, and exiting multi-level trading positions such that all exchange fees, slippage costs, and timing overhead are embedded into the take-profit targets *a priori*. The system uses normalised logistic sigmoids to distribute entry prices, funding allocation, exit sell quantities, and take-profit targets across a configurable number of levels — then chains completed cycles into a compounding sequence with savings diversion. We present the core equations, their derivations from first principles, their purpose within the trading lifecycle, edge-case behaviour, and worked examples under various market conditions.

---

> **?? READ THIS FIRST — Stop Losses and the Limits of Deterministic Fee Hedging**
>
> **Stop losses are deactivated by default. This is intentional.**
>
> The entire framework is built on one guarantee: *if a take-profit target is hit, all fees are covered and the surplus is captured*. Every equation — overhead, effective overhead, downtrend buffer, chain compounding — flows from that single conditional. The system is deterministic **given that exits happen at TP**.
>
> **Stop losses break this guarantee.** When an SL triggers, the position exits at a loss. The overhead that was embedded into the TP is never recovered. The fees from the buy side are realised but not hedged. The chain's capital shrinks instead of growing. The compounding recurrence in §9 reverses: $T_{c+1} < T_c$, overhead *increases*, and the system enters a death spiral where each cycle needs a wider TP to recover the accumulated losses — but wider TPs are less likely to be hit.
>
> **Specifically, activating stop losses can:**
>
> 1. **Invalidate fee neutrality.** The coverage ratio (§11.4) drops below 1. Fees are no longer fully hedged.
> 2. **Break chain compounding.** A single SL hit in cycle $c$ can wipe out the profit from cycles $0$ through $c-1$, especially if $\phi_{\text{sl}} = 1$ (full exit).
> 3. **Create negative feedback loops.** SL loss ? less capital ? higher overhead ? wider TP ? lower fill probability ? more SL hits.
> 4. **Conflict with the downtrend buffer.** The buffer pre-funds re-entry after a *profitable* exit. If the exit was an SL loss, the buffer's extra TP margin was wasted — it inflated the TP that was never hit.
>
> **When you might use them anyway:**
>
> - With **fractional SL** ($\phi_{\text{sl}} \ll 1$): trimming 10–25% of a position at SL is survivable. The remaining position can still hit its TP and recover.
> - With **SL hedge buffer** ($n_{\text{sl}} > 0$): pre-funds future SL losses via TP inflation. This restores the deterministic guarantee *on average* — if you expect $n_{\text{sl}}$ SL hits per profitable cycle, the extra TP profit covers them.
> - In **highly leveraged or volatile markets** where a 100% drawdown is possible and any exit is better than liquidation.
>
> **The default configuration ($\phi_{\text{sl}} = 1$, SL off, $n_{\text{sl}} = 0$) is the only configuration where every equation in this paper holds unconditionally.** Any other configuration introduces probabilistic assumptions about SL hit rates that the deterministic framework cannot verify. See [failure-modes.md](failure-modes.md) for when the mathematics breaks down and [best-case-use.md](best-case-use.md) for the intended operating envelope.

---

## 1. Introduction

### 1.1 The Problem

A trader entering a position faces costs from multiple sources: exchange fees (maker/taker), bid-ask spread, slippage, and funding rates. These costs are typically treated as a post-hoc drag on returns. The question this framework addresses is:

> *Can we pre-compute take-profit targets that are mathematically guaranteed to cover all overhead costs and deliver a specified surplus, regardless of how many levels we split the position across?*

The answer is yes, provided we can express total overhead as a fraction of the position's notional value. The system presented here does exactly this, then extends the single-position model into a multi-level serial generator, a sigmoid-distributed exit strategy, a chained cycle executor, and a downtrend buffer — all governed by the same normalised sigmoid building block.

### 1.2 Design Principles

1. **Determinism over prediction.** The framework makes no forecast about future prices. It computes *what must happen* for a position to be profitable given known costs, then lets the market decide *if* it happens.

2. **Fee neutrality.** Every take-profit target embeds a computed overhead that covers the round-trip cost of the position. If the TP is hit, fees are fully hedged.

3. **Sigmoid universality.** A single normalised sigmoid function (§2) governs entry price distribution, funding allocation, exit quantity distribution, TP target distribution, and downtrend buffer curvature. This provides a consistent, tunable shape across every dimension of the system.

4. **Composability.** Each component — entry calculator, horizon engine, exit strategy, serial generator — is independently useful but designed to compose into chains and cycles.

### 1.3 Stop Losses — Read This First

> **Stop losses are deactivated by default. This is intentional and critical to the framework's guarantees.**

Every equation in this paper flows from one conditional: *if the TP is hit, fees are covered*. Stop losses violate this conditional by forcing an exit before the TP is reached. When an SL triggers:

1. **Fee neutrality breaks.** The overhead embedded in the TP is never recovered. Buy fees are realised but unhedged. The coverage ratio (§11.4) drops below 1.
2. **Chain compounding reverses.** Capital shrinks ($T_{c+1} < T_c$), overhead increases, TPs widen, fill probability drops, and the system enters a negative feedback spiral (see [failure-modes.md](failure-modes.md) §4).
3. **Downtrend buffer is wasted.** The buffer pre-funds re-entry after a *profitable* exit. An SL exit is not profitable — the buffer's TP inflation was never captured.

**The fractional SL** ($\phi_{\text{sl}} < 1$) and **SL hedge buffer** ($n_{\text{sl}} > 0$) exist as controlled relaxations of these guarantees. They do not restore the unconditional determinism — they convert it into a probabilistic hedge that holds *on average* if the actual SL hit rate matches $n_{\text{sl}}$.

**The default configuration (SL off, $\phi_{\text{sl}} = 1$, $n_{\text{sl}} = 0$) is the only configuration where every equation holds unconditionally.** For a complete analysis of all failure modes, see [failure-modes.md](failure-modes.md). For the intended operating envelope and best-case market conditions, see [best-case-use.md](best-case-use.md).

### 1.4 System Architecture Overview

```
???????????????????????????????????????????????????????????????????????
?                        SYSTEM ARCHITECTURE                         ?
???????????????????????????????????????????????????????????????????????
?                                                                     ?
?  ????????????????    ????????????????    ????????????????????????  ?
?  ?   Sigmoid     ?    ?  Overhead    ?    ?  Market Entry        ?  ?
?  ?   Building    ??????  Calculator  ??????  Calculator (§4)     ?  ?
?  ?   Block (§2)  ?    ?  (§3)        ?    ?  Prices + Funding    ?  ?
?  ????????????????    ????????????????    ????????????????????????  ?
?         ?                    ?                       ?              ?
?         ?                    ?              ??????????????????????  ?
?         ?                    ?              ?  Serial Generator  ?  ?
?         ?                    ????????????????  (§8)              ?  ?
?         ?                                   ?  Entry+TP+SL      ?  ?
?         ?                                   ??????????????????????  ?
?         ?                                            ?              ?
?         ?                                   ??????????????????????  ?
?  ????????????????                           ?  Multi-Horizon     ?  ?
?  ?  Downtrend   ?????????????????????????????  Engine (§5)       ?  ?
?  ?  Buffer (§7) ?                           ?  TP/SL Levels      ?  ?
?  ????????????????                           ??????????????????????  ?
?                                                      ?              ?
?                                             ??????????????????????  ?
?                                             ?  Exit Strategy     ?  ?
?                                             ?  Calculator (§6)   ?  ?
?                                             ?  Sell Distribution ?  ?
?                                             ??????????????????????  ?
?                                                      ?              ?
?                                             ??????????????????????  ?
?                                             ?  Chain Executor    ?  ?
?                                             ?  (§9)              ?  ?
?                                             ?  Cycle ? Compound  ?  ?
?                                             ??????????????????????  ?
???????????????????????????????????????????????????????????????????????
```

---

## 2. The Sigmoid Building Block

Every distribution curve in the system uses the logistic sigmoid:

$$
\sigma(x) = \frac{1}{1 + e^{-x}}
$$

### 2.1 Normalised Sigmoid Mapping

For a parameter $t \in [0, 1]$ and steepness $\alpha > 0$:

$$
\sigma_0 = \sigma\!\left(-\frac{\alpha}{2}\right), \qquad
\sigma_1 = \sigma\!\left(+\frac{\alpha}{2}\right)
$$

$$
\hat\sigma_\alpha(t) = \frac{\sigma\!\bigl(\alpha \cdot (t - 0.5)\bigr) - \sigma_0}{\sigma_1 - \sigma_0}
$$

This maps $t \in [0,1]$ to a normalised $[0,1]$ sigmoid curve with the following properties:

| $\alpha$ | Shape | Use case |
|----------|-------|----------|
| $\to 0$ | Linear ($\hat\sigma \approx t$) | Uniform distribution |
| $\approx 4$ | Smooth S-curve | Balanced distribution |
| $\gg 6$ | Near step function | Concentrated distribution |

**Why sigmoids?** Linear distributions spread resources uniformly. Gaussian distributions are symmetric but fall to zero at the tails. The logistic sigmoid offers a monotonic, bounded, infinitely differentiable curve where the steepness parameter $\alpha$ controls concentration without changing the domain or range. One parameter controls the shape from uniform to binary.

### Sigmoid Shape Visualisation

```
  ??(t)
  1.0 ?                                          ??? ??0 (linear)
      ?                                    ?????
      ?                              ???????
  0.8 ?                         ???????
      ?                    ???????             ??? ??4 (S-curve)
      ?               ???????                 ?
  0.6 ?          ???????                      ?
      ?        ??????                    ??????
      ?      ?????               ????????
  0.4 ?    ????          ????????
      ?   ????    ????????           ??? ??6 (step)
      ?  ??? ??????                  ?
  0.2 ? ?????                  ??????
      ?????
      ?????
  0.0 ????????????????????????????????????????????
      0.0      0.2      0.4      0.6      0.8   1.0  t
```

---

## 3. Core Overhead and Position Sizing

### 3.1 Fee Component

$$
\mathcal{F} = f_s \cdot f_h \cdot \Delta t
$$

where $f_s$ is the fee spread (exchange fee rate plus slippage), $f_h$ is the fee hedging coefficient (a safety multiplier, typically $\geq 1$), and $\Delta t$ is a time scaling factor.

**Purpose.** $\mathcal{F}$ captures the cost of one round-trip fee event, amplified by the hedging safety margin and scaled by time. When $f_h = 1$, fees are hedged at exactly the expected rate. When $f_h > 1$, the system over-provisions — building a buffer against fee spikes or unexpected slippage.

### 3.2 Raw Overhead

$$
\text{OH}(P, q) = \frac{\mathcal{F} \cdot n_s \cdot (1 + n_f)}{\dfrac{P}{q} \cdot T + K}
$$

where $P$ is the price per unit, $q$ is the quantity, $n_s$ is the number of symbols in the portfolio, $T$ is the portfolio pump (injected capital), $K$ is an additive offset, and $n_f$ is the **future trade count** — the number of future chain trades whose fees this position's TP must pre-cover ($n_f = 0$ means self only).

**Derivation.** The numerator represents the total fee cost across all symbols, scaled by $(1 + n_f)$ so that a single profitable exit hedges the fees of $n_f$ subsequent trades in the chain. The denominator is the *effective capital density* — the per-unit price scaled by pump capital, with $K$ as a stabiliser to prevent division by zero when $T = 0$. As $T$ grows, the denominator grows, and overhead shrinks: more capital means fees are a smaller fraction of the position.

**Interpretation.** OH is the fraction of position value consumed by fees. A position at OH = 0.03 needs a 3% price increase just to break even (before surplus). When $n_f > 0$, the overhead inflates proportionally — a trade with $n_f = 2$ needs roughly $3\times$ the overhead of $n_f = 0$ to pre-fund two future trades' fees.

**Chain auto-derivation.** In chain mode (§9), $n_f$ is not a manual parameter — it is derived from the chain structure. Cycle $c$ in a $C$-cycle chain sets $n_f = C - 1 - c$: cycle 0 pre-funds $C-1$ future trades, the last cycle pre-funds 0. This means early cycles have wider TPs (hedging more future fees) and later cycles have tighter TPs (hedging only themselves), producing a natural convergence toward the surplus rate as the chain progresses.

### 3.3 Effective Overhead

$$
\text{EO}(P, q) = \text{OH}(P, q) + (s + f_s) \cdot f_h \cdot \Delta t
$$

where $s$ is the surplus rate (desired profit margin above break-even).

**Purpose.** EO is the total TP offset — it includes not just fee recovery (OH) but also the desired profit margin ($s$) and a second fee spread term (to cover the sell-side fee from the exit itself). When a TP is placed at $P_e \cdot (1 + \text{EO})$, the resulting sale covers:

1. The buy-side fee (from OH)
2. The sell-side fee (the second $f_s$ term)
3. The desired surplus ($s$)

### Overhead Composition Visualisation

```
  TP Target = Entry × (1 + EO)
  ????????????????????????????????????????????????????????
  ?                                                      ?
  ?  Entry Price                                         ?
  ?  ????????????                                        ?
  ?  ?          ? OH (Raw Overhead)                       ?
  ?  ?          ????????????                              ?
  ?  ?          ?          ? Surplus (s)                   ?
  ?  ?          ?          ?????????                       ?
  ?  ?          ?          ?       ? Sell Fee (fs)         ?
  ?  ?          ?          ?       ?????                   ?
  ?  ???????????????????????????????????                  ?
  ?  ???? Cost Basis ???????? Profit ???                  ?
  ?                                                      ?
  ?  EO = OH + (s + fs) · fh · ?t                         ?
  ????????????????????????????????????????????????????????
```

### 3.4 Position Delta

$$
\delta = \frac{P \cdot q}{T}
$$

The portfolio weight of this position. When $\delta \ll 1$, the position is small relative to capital. When $\delta \gg 1$, the position dominates the portfolio. This ratio governs the downtrend buffer curvature (§7).

### 3.5 The Capital-Overhead Relationship

A key property of the overhead formula:

$$
\lim_{T \to \infty} \text{OH}(P, q) = 0
$$

As pump capital grows, overhead vanishes. The TP target converges to:

$$
\text{TP} \to P_e \cdot (1 + s + f_s \cdot f_h \cdot \Delta t)
$$

The surplus rate $s$ becomes the dominant factor. This is the theoretical minimum profit margin — the cost of doing business approaches zero, leaving only the desired profit.

Conversely, when $T \to 0$ and $K = 0$, overhead diverges. The system correctly signals that a position with no capital behind it has infinite relative fee burden.

### Capital vs Overhead Visualisation

```
  OH(P,q)
  ?
  ? ?
  ?  ?                OH = F·ns·(1+nf) / (P/q·T + K)
  ?   ?
  ?    ?
  ?     ?
  ?      ?
  ?       ?
  ?        ?
  ?         ?
  ?          ????????????????????????? ? 0 as T ? ?
  ?                                    (surplus-only limit)
  ??????????????????????????????????????  T (Capital)
       Low capital:              High capital:
       OH dominates              s dominates
```

---

## 4. Market Entry Calculator

### 4.1 Purpose

Given a current market price $P$, generate $N$ entry price levels distributed below $P$ (for LONG) with sigmoid-shaped pricing and risk-warped funding allocation. Each level specifies: *at what price to buy, how much capital to deploy, and the break-even price*.

### 4.2 Price Distribution

**Default mode** (entries from near-zero to current price):

$$
P_{\text{low}} = 0, \qquad P_{\text{high}} = P
$$

**Range mode** (entries around current price):

$$
P_{\text{low}} = \max(P - R_{\text{below}},\; \varepsilon), \qquad P_{\text{high}} = P + R_{\text{above}}
$$

**Entry price at level $i$:**

$$
t_i = \begin{cases}
i / (N-1) & \text{if } N > 1 \\
1 & \text{if } N = 1
\end{cases}
$$

$$
P_e^{(i)} = P_{\text{low}} + \hat\sigma_\alpha(t_i) \cdot (P_{\text{high}} - P_{\text{low}})
$$

Level 0 is the deepest discount (near $P_{\text{low}}$); level $N-1$ is nearest to market price.

**Break-even:**

$$
P_{\text{BE}}^{(i)} = P_e^{(i)} \cdot (1 + \text{OH})
$$

### 4.3 Funding Allocation

The risk coefficient $r \in [0, 1]$ warps a sigmoid funding curve:

$$
w_i = (1 - r) \cdot \hat\sigma(t_i) + r \cdot (1 - \hat\sigma(t_i))
$$

$$
F_i = \frac{w_i}{\sum_{j=0}^{N-1} w_j}
$$

| $r$ | Capital concentration | Strategy |
|-----|-----------------------|----------|
| 0 | More at higher (safer) entries | Conservative — buys heavily near market |
| 0.5 | Uniform | Neutral |
| 1 | More at lower (deeper) entries | Aggressive — loads up on deep discounts |

**Capital and quantity:**

$$
\text{Funding}_i = T_{\text{avail}} \cdot F_i, \qquad q_i = \frac{\text{Funding}_i}{P_e^{(i)}}
$$

### Entry Level Distribution Visualisation (4 levels, range mode)

```
  Price
  ?
  ?  P_high ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? TP?
  ?                                  L3 ????
  ?                                        ?
  ?                                        ?
  ?                            L2 ?????????????? TP?
  ?                                        ?
  ?  P_market ? ? ? ? ? ? ? ? ? ? ? ? ? ??
  ?                                        ?
  ?                   L1 ??????????????????????? TP?
  ?                                        ?
  ?               L0 ?????????????????????????? TP?
  ?  P_low ? ? ? ? ? ? ? ? ? ? ? ? ? ? ??
  ?
  ??????????????????????????????????????????????
     Entry Prices (sigmoid-distributed)     TP Targets

  ? = entry level     ????? = distance to TP (includes EO)
```

### Risk Coefficient Funding Visualisation

```
  Funding
  per Level
  ?
  ?
  ? r=1.0      r=0.5      r=0.0
  ? (aggressive) (neutral) (conservative)
  ?
  ?  ????                    ????
  ?  ????        ????        ????
  ?  ????        ????        ????
  ?  ????        ????        ????
  ?  ????        ????        ????
  ?  ????        ????        ????
  ?  ????        ????        ????
  ?  ????        ????        ????
  ???L0?L1?L2?L3??L0?L1?L2?L3??L0?L1?L2?L3???
     Deep  Near   Deep  Near   Deep  Near
     disc. mkt    disc. mkt    disc. mkt

  ???? = heavy allocation   ???? = light allocation
```

---

## 5. Multi-Horizon Engine (TP/SL Generation)

### 5.1 Purpose

Given an existing position (entry price $P_e$, quantity $q$), generate $N$ take-profit and stop-loss levels that embed the overhead formula.

### 5.2 Standard Mode ($R_{\max} = 0$)

$$
\text{TP}_i = P_e \cdot q \cdot (1 + \text{EO} \cdot (i + 1))
$$

$$
\text{SL}_i = P_e \cdot q \cdot (1 - \text{EO} \cdot (i + 1))
$$

### 5.3 Max-Risk Mode ($R_{\max} > 0$)

When $R_{\max} > 0$, the factor is sigmoid-interpolated between overhead and max risk:

$$
f_{\min} = \text{OH}(P_e, q), \qquad f_{\max} = \max(R_{\max}, f_{\min})
$$

$$
\text{factor}_i = f_{\min} + \hat\sigma_4(t_i) \cdot (f_{\max} - f_{\min})
$$

$$
\text{TP}_i = P_e \cdot q \cdot (1 + \text{factor}_i)
$$

### 5.4 Per-Level TP with Risk Warping (`levelTP`)

$$
t_i = \frac{i + 1}{N + 1}
$$

**Risk warping mirrors funding allocation:**

$$
n_i = (1 - r) \cdot \hat\sigma_{\alpha'}(t_i) + r \cdot (1 - \hat\sigma_{\alpha'}(t_i))
$$

**LONG:**

$$
\text{TP}_{\min} = P_e \cdot (1 + \text{EO} + R_{\min})
$$

$$
\text{TP}_{\max} = P_{\text{ref}} \cdot (1 + R_{\max})
$$

$$
\text{TP}_i = \text{TP}_{\min} + n_i \cdot (\text{TP}_{\max} - \text{TP}_{\min})
$$

### 5.5 Per-Level Stop-Loss (`levelSL`)

$$
\text{SL}^{\text{long}} = P_e \cdot (1 - \text{EO}), \qquad \text{SL}^{\text{short}} = P_e \cdot (1 + \text{EO})
$$

### 5.6 Fractional Stop-Loss Exit

The stop-loss fraction $\phi_{\text{sl}} \in [0, 1]$ controls how much of the position is sold when the SL price is hit:

$$
q_{\text{sl}} = q_i \cdot \phi_{\text{sl}}
$$

| $\phi_{\text{sl}}$ | Behaviour | Use case |
|---------------------|-----------|----------|
| 1.0 | Full exit | Close the entire position at SL (default, maximum loss containment) |
| 0.5 | Half exit | Reduce exposure by 50%, keep the rest open for recovery |
| 0.25 | Quarter exit | Trim position, absorb a small loss, hold for reversal |
| 0.0 | No exit | SL is tracked but does not execute (monitoring only) |

### 5.7 Capital-Loss Cap

$$
\text{TotalSLLoss} = \phi_{\text{sl}} \cdot \sum_{i=0}^{N-1} \text{EO} \cdot \text{Funding}_i \leq T_{\text{avail}}
$$

When violated, $\phi_{\text{sl}}$ is auto-clamped downward:

$$
\phi_{\text{sl}}^{\text{clamped}} = \phi_{\text{sl}} \cdot \frac{T_{\text{avail}}}{\text{TotalSLLoss}}
$$

### TP/SL Level Visualisation

```
  Price
  ?
  ?  TP? ? ? ? ? ? ? ? ? ? ? ? ?  ? widest (most profit if hit)
  ?  TP? ? ? ? ? ? ? ? ? ? ?
  ?  TP? ? ? ? ? ? ? ? ?
  ?  TP? ? ? ? ? ? ? ? tightest (just above break-even)
  ?  ? ? ? ? ? ? ? ? Break-Even
  ?  ?????????????????? ENTRY PRICE (Pe)
  ?  ? ? ? ? ? ? ? ? Break-Even (mirror)
  ?  SL? ? ? ? ? ? ?
  ?  SL? ? ? ? ? ? ? ? ?
  ?  SL? ? ? ? ? ? ? ? ? ? ?
  ?  SL? ? ? ? ? ? ? ? ? ? ? ? ?
  ?
  ???????????????????????????????????? Level index
```

---

## 6. Exit Strategy Calculator

### 6.1 Purpose

Given an open position (from a Buy trade), distribute a sigmoidal fraction of holdings across the TP levels as pending sell orders. Controls *how much* to sell at each level.

### 6.2 Cumulative Sell Distribution

$$
c = r \cdot (N - 1)
$$

$$
C_i = \sigma\!\bigl(\alpha \cdot (i - 0.5 - c)\bigr)
$$

$$
\hat{C}_i = \frac{C_i - C_0}{C_N - C_0}
$$

**Sell quantity per level:**

$$
q_i^{\text{sell}} = q \cdot \phi \cdot (\hat{C}_{i+1} - \hat{C}_i)
$$

| $r$ | Sell concentration | Strategy |
|-----|-------------------|----------|
| 0 | Heavy at early (low) TP levels | Lock in gains quickly |
| 0.5 | Balanced across all levels | Moderate |
| 1 | Heavy at late (high) TP levels | Maximise upside, hold for bigger moves |

### Exit Distribution Visualisation

```
  Sell Qty
  per TP Level
  ?
  ?  r=0 (lock gains)       r=0.5 (balanced)       r=1 (max upside)
  ?
  ?  ????                                                    ????
  ?  ????                                                    ????
  ?  ????  ????                            ????  ????       ????
  ?  ????  ????  ????          ????  ????  ????  ????  ???? ????
  ?  ????  ????  ????  ....    ????  ????  ????  ????  ???? ????
  ???TP????TP????TP????TP?????TP????TP????TP????TP???TP???TP???TP???TP????
     Low   ???????????  High   Low   ???????????  High  Low  ?????????? High
```

### 6.3 Net Profit per Level

$$
\text{Net}_i = (P_{\text{TP}}^{(i)} - P_e) \cdot q_i^{\text{sell}} - \text{BuyFee}_i - \text{SellFee}_i
$$

---

## 7. Downtrend Buffer

### 7.1 Purpose

When a position is opened, the system can optionally inflate TP targets to pre-fund $n_d$ future downturn cycles.

### 7.2 Axis-Dependent Sigmoid Curvature

**Hyperbolic compression** (maps $\delta$ to $[0, 1)$):

$$
t = \frac{\delta}{\delta + 1}
$$

**Axis-dependent steepness:**

$$
\alpha_d = \max(\delta, 0.1)
$$

**Per-cycle buffer:**

$$
\text{pc} = R_{\min} + \hat\sigma_{\alpha_d}(t) \cdot (\text{upper} - R_{\min})
$$

**Total buffer:**

$$
\text{buffer} = 1 + n_d \cdot \text{pc}
$$

$$
\text{TP}_{\text{adj}} = \text{TP}_{\text{base}} \cdot \text{buffer}
$$

### 7.3 Behaviour Table

| $\delta$ | Steepness | Per-cycle buffer | Interpretation |
|----------|-----------|-----------------|----------------|
| $\ll 1$ | Near-linear | $\approx R_{\min}$ | Small position, minimal buffer |
| $\approx 1$ | Mild S-curve | Midpoint between bounds | Balanced protection |
| $\gg 1$ | Steep S-curve | Saturates at upper | Position dominates portfolio, maximum protection |
| $= 0$ | — | Buffer $= 1$ | No position deployed, nothing to protect |

### 7.5 Stop-Loss Hedge Buffer

$$
\text{buffer}_{\text{sl}} = 1 + n_{\text{sl}} \cdot \phi_{\text{sl}} \cdot \text{pc}
$$

The combined TP multiplier applies both buffers multiplicatively:

$$
\text{TP}_{\text{adj}} = \text{TP}_{\text{base}} \cdot \text{buffer}_{\text{dt}} \cdot \text{buffer}_{\text{sl}}
$$

### Buffer Effect Visualisation

```
  TP Price
  ?
  ?
  ?  ???????????  TP_adjusted (with buffer × 1.03)
  ?     ? 3%
  ?  ???????????  TP_base (no buffer)
  ?     ? EO
  ?  ···········  Break-Even
  ?     ? OH
  ?  ???????????  Entry Price
  ?
  ?  Buffer = 1 + n_d × pc
  ?  (n_d=2, pc=0.015 ? Buffer=1.03)
  ?
  ??????????????????????????????????????
```

---

## 8. Serial Generator

### 8.1 Purpose

Combines Entry (§4) + TP (§5.4) + SL (§5.5) into a unified tuple per level, producing a complete trading plan from a single set of parameters.

### 8.2 Output per Level

For each level $i \in [0, N-1]$:

| Field | Formula | Meaning |
|-------|---------|---------|
| Entry Price | $P_{\text{low}} + \hat\sigma(t_i) \cdot (P_{\text{high}} - P_{\text{low}})$ | Where to buy |
| Break Even | $P_e^{(i)} \cdot (1 + \text{OH})$ | Price to recover all fees |
| Funding | $T_{\text{avail}} \cdot F_i$ | Capital deployed at this level |
| Quantity | $\text{Funding}_i / P_e^{(i)}$ | Units purchased |
| Take Profit | $\texttt{levelTP}(P_e^{(i)}, \ldots)$ | Where to sell for profit |
| Stop Loss | $P_e^{(i)} \cdot (1 - \text{EO})$ | Where to cut losses |
| TP Gross | $\text{TP} \cdot q_i - P_e^{(i)} \cdot q_i$ | Gross profit if TP is hit |
| Discount | $(P - P_e^{(i)}) / P \times 100\%$ | How far below market |

### 8.3 Properties

1. **Self-consistent.** Every entry's TP embeds enough overhead to cover its own fees plus the surplus.
2. **Level-independent.** Each tuple is independently viable. If only level 3 of 7 triggers, that single trade is still fee-hedged and profitable at its TP.
3. **Risk-symmetric.** The risk coefficient warps both funding and TP in the same direction.

---

## 9. Chain Execution

### 9.1 Concept

A chain is a sequence of cycles where each cycle is a complete serial plan (§8). When all positions from cycle $c$ close (all TPs or SLs hit), the realised profit is partially diverted to savings and the remainder reinvested into cycle $c+1$.

### 9.2 Cycle Transition

$$
\Pi_c = \sum_{\text{sells in cycle } c} \text{Net}_j
$$

$$
\text{Savings}_c = \Pi_c \cdot s_{\text{save}}
$$

$$
T_{c+1} = \text{Capital} - \text{Savings}_c
$$

### 9.3 Compounding Effect

$$
\text{OH}_{c+1} < \text{OH}_c \quad \text{(since } T_{c+1} > T_c \text{)}
$$

### 9.4 Future Fee Pre-Funding

$$
n_f^{(c)} = C - 1 - c
$$

### Chain Compounding Visualisation

```
  Capital
  ?
  ?                                          ??? $1,061
  ?                                    ??????
  ?                              ??????
  ?                         ?????
  ?                    ?????  Cycle 2: +$21.85
  ?               ?????       Savings: $1.09
  ?          ?????
  ?     ?????  Cycle 1: +$21.42
  ??????       Savings: $1.07
  ?
  ? $1,000  Cycle 0: +$21.00
  ?         Savings: $1.05
  ?
  ???????????????????????????????????????????? Cycles
       C?           C?           C?

  ???????????????????????????????????????????
  ?  OH decreases each cycle (more capital) ?
  ?  TP tightens ? higher fill probability  ?
  ?  System converges to surplus-only limit ?
  ???????????????????????????????????????????
```

### 9.5 Savings as Irreversible Extraction

$$
\text{Wealth} = \text{Capital}_{\text{chain}} + \text{Savings}_{\text{total}}
$$

### Two-Compartment Wealth Model

```
  ???????????????????????????????????????
  ?           CHAIN CAPITAL             ?
  ?  ???????  ???????  ???????        ?
  ?  ? C?  ??? C?  ??? C?  ?? ...    ?
  ?  ?$1000?  ?$1020?  ?$1040?         ?
  ?  ???????  ???????  ???????        ?
  ?     ?        ?        ?    (compounds)?
  ??????????????????????????????????????
        ?        ?        ?
        ?        ?        ?
  ???????????????????????????????????????
  ?         SAVINGS (irreversible)      ?
  ?  $1.05  + $1.07  + $1.09  = $3.21  ?
  ?         (extracted permanently)      ?
  ???????????????????????????????????????
```

---

## 10. Profit Calculation & ROI

### 10.1 Unrealised (Open Position)

**LONG:** $\text{Gross} = (P_{\text{now}} - P_e) \cdot q$

**SHORT:** $\text{Gross} = (P_e - P_{\text{now}}) \cdot q$

$$
\text{Net} = \text{Gross} - F_{\text{buy}} - F_{\text{sell}}
$$

$$
\boxed{
\text{ROI} = \frac{\text{Net}}{P_e \cdot q + F_{\text{buy}}} \times 100\%
}
$$

### 10.2 Realised (Closed Position)

$$
\text{Gross} = (P_{\text{sell}} - P_e) \cdot q_{\text{sold}}
$$

$$
\text{Net} = \text{Gross} - F_{\text{buy}} - F_{\text{sell}}
$$

The ROI denominator includes buy fees because they are part of the cost basis. Sell fees are subtracted from proceeds.

### ROI Calculation Flow

```
  ???????????????????????????????????????????????????????
  ?                  ROI CALCULATION                     ?
  ?                                                     ?
  ?  ?????????????????                                  ?
  ?  ? Entry Price    ?                                  ?
  ?  ? × Quantity     ???? Cost Basis ???               ?
  ?  ? + Buy Fee      ?                 ?               ?
  ?  ?????????????????                 ?               ?
  ?                                     ?               ?
  ?  ?????????????????         ????????????????        ?
  ?  ? Exit Price     ?         ?              ?        ?
  ?  ? × Quantity     ????      ?  ROI =       ?        ?
  ?  ? ? Buy Fee      ?  Gross  ?  Net / Cost  ?        ?
  ?  ? ? Sell Fee     ???? Net ??  × 100%      ?        ?
  ?  ?????????????????         ?              ?        ?
  ?                             ????????????????        ?
  ?                                                     ?
  ?  Example:                                           ?
  ?  Entry: $100 × 10 qty = $1,000 + $5 fee = $1,005   ?
  ?  Exit:  $105 × 10 qty = $1,050                      ?
  ?  Gross: $50    Net: $50 - $5 - $5 = $40             ?
  ?  ROI: $40 / $1,005 × 100% = 3.98%                  ?
  ???????????????????????????????????????????????????????
```

---

## 11. Simulator

### 11.1 Purpose

Steps through a price series (forward or historical), executing the entry/exit logic mechanically.

### 11.2 Entry Trigger

$$
\text{Buy if } P_t \leq P_e^{(i)} \text{ and } \text{Cost}_i + \text{Fee}_i \leq \text{Capital}
$$

### 11.3 Exit Trigger

$$
\text{Sell if } P_t \geq P_{\text{TP}}^{(j)}
$$

### 11.4 Fee Hedging Verification

$$
\text{Coverage} = \frac{\text{HedgePool}}{\text{TotalFees}}
$$

Coverage $\geq 1$ confirms the overhead formula fully covered all trading fees.

### Simulator Execution Flow

```
  Price
  ?
  ?     ??      ????
  ?    ?  ?    ?      ?                  ??
  ?   ?    ?  ?        ?    ??          ?  ?
  ?????????????????????????????????????????????? Time
  ?  ?  L3?  ?            ?    ?  TP? hit ?  ?
  ?  ?  L2?  ? L2?        ?    ?  TP? hit ?  ?
  ?  ?       ? L1?        ?    ?              ?
  ?  ?       ? L0?        ?    ?              ?
  ?                                           ?
  ?  ?? entries trigger ?? ??? TPs hit ???????
  ?      (price drops)         (price recovers)
  ?
  ?????????????????????????????????????????????? t
```

---

## 12. Worked Examples

### 12.1 Single Entry — Bull Market

**Scenario:** BTC at $100,000. Pump $10,000. Fee spread 0.1%. Surplus 2%. 4 levels. Risk 0.5.

```
Parameters:
  P = 100,000    T = 10,000    f_s = 0.001
  s = 0.02       N = 4         r = 0.5
  f_h = 1        ?t = 1        K = 0

Overhead:
  F = 0.001 × 1 × 1 = 0.001
  OH = (0.001 × 1) / ((100,000/1) × 10,000 + 0) = 1e-12 ? 0
  EO = 0 + (0.02 + 0.001) × 1 × 1 = 0.021

Entry levels (default range, priceLow=0, priceHigh=100,000):
  L0: entry ?  $2,474    discount 97.5%
  L1: entry ? $26,894    discount 73.1%
  L2: entry ? $73,106    discount 26.9%
  L3: entry ? $97,526    discount  2.5%

Funding (r=0.5, uniform):
  Each level gets ? 25% of $10,000 = $2,500

TP at each level ? entry × (1 + 0.021) ? entry × 1.021
```

### 12.2 Chain Cycle — Compounding Growth

**Scenario:** Starting capital $1,000. Surplus 2%. Savings rate 5%. 3 cycles.

```
Cycle 0:  $1,000.00 ? profit $21.00 ? savings $1.05 ? capital $1,019.95
Cycle 1:  $1,019.95 ? profit $21.42 ? savings $1.07 ? capital $1,040.30
Cycle 2:  $1,040.30 ? profit $21.85 ? savings $1.09 ? capital $1,061.06

Total savings extracted: $3.21
Total wealth: $1,064.27
Growth: 6.4% over 3 cycles
```

### 12.3 Range Mode — Sideways Market

```
ETH at $3,500. Range ±$200. 4 levels.

priceLow  = $3,300
priceHigh = $3,700

Entry levels:
  L0: $3,310   (near bottom of range)
  L1: $3,407
  L2: $3,593
  L3: $3,690   (near top of range)
```

---

## 13. Edge Cases and Degeneracies

| Scenario | Behaviour |
|----------|-----------|
| $T = 0$, $K = 0$ | OH clamped to 0, no entries generated |
| $N = 1$ | Single entry at $P_{\text{high}}$, all capital in one level |
| $\alpha \to 0$ | All sigmoids linear, system becomes a uniform grid |
| $\alpha \gg 10$ | Step functions, entries cluster at extremes |
| $r = 0.5$ | All weights uniform, funding equal across levels |
| Short positions | All formulas mirror (TP below entry, SL above) |

---

## 14. Summary of Equations

| Equation | Section | Purpose |
|----------|---------|---------|
| $\hat\sigma_\alpha(t)$ | §2.1 | Normalised sigmoid mapping |
| $\text{OH}(P, q)$ | §3.2 | Raw overhead (fee fraction) |
| $\text{EO}(P, q)$ | §3.3 | Effective overhead (fees + surplus) |
| $P_e^{(i)}$ | §4.2 | Entry price per level |
| $F_i$ | §4.3 | Funding fraction per level |
| $\text{TP}_i$ | §5 | Take-profit target |
| $\text{SL}_i$ | §5.5 | Stop-loss target |
| $q_{\text{sl}}$ | §5.6 | Fractional SL exit quantity |
| $\phi_{\text{sl}}^{\text{clamped}}$ | §5.7 | Capital-loss cap on SL fraction |
| $q_i^{\text{sell}}$ | §6.2 | Exit quantity per level |
| buffer | §7.2 | Downtrend TP multiplier |
| $\text{buffer}_{\text{sl}}$ | §7.5 | Stop-loss hedge TP multiplier |
| $\text{ROI}$ | §10.1 | Return on investment |
| Coverage | §11.4 | Fee hedging verification |
| $\partial\hat\sigma/\partial t$, $\partial\hat\sigma/\partial\alpha$ | §15.1 | Sigmoid gradient primitives |
| $\partial\text{OH}/\partial T$ | §15.2 | Capital-overhead sensitivity |
| $\partial\text{EO}/\partial s$ | §15.3 | Surplus linearity ($= f_h \Delta t$, constant) |
| $\partial\text{TP}/\partial r$ | §15.6 | Risk warps TP distribution |
| $\partial\Pi/\partial s$ | §15.7 | Profit-surplus sensitivity |
| $dT_c/d\theta$ | §15.9 | Chain recurrence (BPTT structure) |
| $J_1 \ldots J_5$ | §16.2 | Optimisation objectives |
| $\Sigma_1$–$\Sigma_8$ | §A.1.2 | Axiomatic foundation of $S$ |
| $G_S$ | §A.2.2 | Gödel sentence (solvency undecidability) |
| $\|\Psi_c\rangle$ | §A.3.2 | Capital state-vector in trajectory space |
| $\hat{O}_\theta$ | §A.3.3 | Parameter choice as observation operator |
| $\mathcal{A}$ | §A.3.4 | Attractor manifold (positive-growth region) |

---

## 15. Partial Derivative Catalog

### 15.1 Sigmoid Primitives

$$
\sigma(x) = \frac{1}{1 + e^{-x}}, \qquad \sigma'(x) = \sigma(x)\bigl(1 - \sigma(x)\bigr)
$$

$$
\frac{\partial \hat\sigma}{\partial t} = \frac{\alpha \cdot v(1-v)}{S}
$$

$$
\frac{\partial \hat\sigma}{\partial \alpha} =
\frac{(t - 0.5)\,v(1-v) + \tfrac{1}{2}\,\sigma_0(1-\sigma_0)}{S}
\;-\;
\hat\sigma \cdot \frac{\sigma_1(1-\sigma_1) + \sigma_0(1-\sigma_0)}{2S}
$$

### 15.2 Overhead Derivatives

$$
\frac{\partial\,\text{OH}}{\partial T} = -\frac{\text{OH}}{D} \cdot \frac{P}{q}
$$

### 15.3 Effective Overhead Derivatives

$$
\frac{\partial\,\text{EO}}{\partial s} = f_h \cdot \Delta t \quad \text{(constant — surplus is linear)}
$$

### 15.10 Gradient Summary Table

| Output | w.r.t. | Sign | Interpretation |
|--------|--------|------|---------------|
| OH | $T$ | $-$ | More capital ? less overhead |
| OH | $f_s$ | $+$ | Higher fees ? more overhead |
| EO | $s$ | $+$ | Constant — surplus is linear |
| $P_e$ | $\alpha$ | $\pm$ | Steepness shifts entries |
| $F_i$ | $r$ | $\pm$ | Risk reallocates capital |
| TP | $s$ | $+$ | Surplus raises TP floor |
| TP | $R_{\max}$ | $+$ | Ceiling scales with reference |
| TP | $r$ | $\pm$ | Warps TP distribution |
| $\Pi_i$ | $s$ | $+$ | More surplus ? more profit |
| Buffer | $R_{\min}$ | $+$ | Floor raises buffer |
| $T_{c+1}$ | $s_{\text{save}}$ | $-$ | Savings drain capital |

### Gradient Flow Visualisation

```
  ??????????????????????????????????????????????????????????
  ?                   GRADIENT FLOW                        ?
  ?                                                        ?
  ?  Parameters ?                                          ?
  ?  ???? ???? ???? ????? ?????? ??????                  ?
  ?  ?s ? ?r ? ?? ? ?fh ? ?Rmax? ?save?                   ?
  ?  ???? ???? ???? ????? ?????? ??????                  ?
  ?   ?    ?    ?     ?     ?      ?                       ?
  ?   ?    ?    ?     ?     ?      ?                       ?
  ?  ????????????????????????????  ?                       ?
  ?  ?    Overhead (OH, EO)     ?  ?                       ?
  ?  ????????????????????????????  ?                       ?
  ?               ?                ?                       ?
  ?   ?????????????????????????    ?                       ?
  ?   ? Serial Plan (per level)?    ?                       ?
  ?   ? Entry, TP, SL, Funding?    ?                       ?
  ?   ?????????????????????????    ?                       ?
  ?               ?                ?                       ?
  ?   ?????????????????????????    ?                       ?
  ?   ?    Per-Level Profit   ?    ?                       ?
  ?   ?    ?? = (TP?-Pe?)·q? ?    ?                       ?
  ?   ?????????????????????????    ?                       ?
  ?               ?                ?                       ?
  ?   ?????????????????????????    ?                       ?
  ?   ?    Chain Recurrence   ??????                       ?
  ?   ?  T_{c+1} = Tc + ?c·  ?                            ?
  ?   ?         (1 - s_save)  ?                            ?
  ?   ?????????????????????????                            ?
  ?               ?                                        ?
  ?               ?                                        ?
  ?   ?????????????????????????                            ?
  ?   ?   Objective J(?)      ?                            ?
  ?   ? MaxProfit / MinSpread ?                            ?
  ?   ? MaxROI / MaxChain     ?                            ?
  ?   ? MaxWealth             ?                            ?
  ?   ?????????????????????????                            ?
  ??????????????????????????????????????????????????????????
```

---

## 16. Gradient-Based Optimisation Framework

### 16.2 Objective Functions

| # | Objective | Formula | Use case |
|---|-----------|---------|----------|
| 1 | MaxProfit | $\sum_i (\text{TP}_i - P_e^{(i)}) \cdot q_i$ | Maximise raw dollar profit |
| 2 | MinSpread | $-\sum_i \left(\frac{\text{TP}_i - P_e^{(i)}}{P_e^{(i)}}\right)^2$ | Tightest TPs, fastest cycles |
| 3 | MaxROI | $\frac{\sum_i \Pi_i}{T_{\text{avail}}}$ | Best return per dollar deployed |
| 4 | MaxChain | $\prod_c\left(1 + \frac{\Pi_c(1-s_{\text{save}})}{T_c}\right)$ | Maximum compounding speed |
| 5 | MaxWealth | $T_C + \sum_c \Pi_c \cdot s_{\text{save}}$ | Total wealth (capital + savings) |

### 16.7 Expected Behaviour by Objective

| Objective | $s$ tends to | $r$ tends to | $\alpha$ tends to | $s_{\text{save}}$ tends to |
|-----------|-------------|-------------|-------------------|--------------------------|
| MaxProfit | Increase | Depends | Moderate | N/A |
| MinSpread | Decrease | 0.5 | Low | N/A |
| MaxROI | Increase (bounded) | Increase | High | N/A |
| MaxChain | Moderate | Moderate | Moderate | Decrease |
| MaxWealth | Moderate | Moderate | Moderate | Interior optimum |

---

## 17. Conclusion

The framework transforms trading from *prediction* into *engineering*. The overhead formula guarantees fee neutrality. The sigmoid distributions provide tunable, consistent shapes across all dimensions. The chain execution compounds capital across cycles with built-in savings extraction.

The system does not predict whether entries will trigger or TPs will be hit. It guarantees that *if* they are hit, the result is deterministic: fees are covered, surplus is captured, and the next cycle is funded. The market provides the randomness; the equations provide the structure.

The surplus rate $s$ is the fundamental parameter — it has a constant, positive gradient on EO, making it the most predictable tuning knob. As capital grows across chain cycles, overhead vanishes, and the system converges to its theoretical limit: **a cycle machine generating $s$% per round**.

---

## Addendum A — Formal Axiomatic Structure

### A.1 The Axiomatic System $S$

The system $S$ is formalised with axioms $\Sigma_1$–$\Sigma_8$ covering sigmoid closure, overhead positivity, fee neutrality conditional, capital monotonicity, overhead convergence, savings irreversibility, level independence, and recurrence.

### A.2 Incompleteness Analysis

The Gödel sentence of $S$:

$$
G_S: \quad \forall\, c \geq 0: \; T_c > 0 \quad \text{("The system will remain solvent forever")}
$$

$S$ cannot prove $G_S$ because solvency depends on market prices — an external oracle not axiomatised within $S$.

### A.3 Capital as State-Vector

Capital exists as a superposition of possible outcomes:

$$
| \Psi_c \rangle = \sum_{b \in \{0,1\}^N} a_b \cdot | b \rangle
$$

The parameter choice $\theta$ acts as an operator that shapes the basis; the market "measures" the state by triggering entries and exits.

### Three-Layer Architecture

```
  ????????????????????????????????????????????????????????
  ?  Layer 3: Environment ? (Market)                     ?
  ?  • Price evolution P_market(t)                       ?
  ?  • Unaxiomatised oracle                              ?
  ?  • Source of all Type II inconsistency               ?
  ????????????????????????????????????????????????????????
  ?  Layer 2: Meta-System M (Architect)                  ?
  ?  • Selects ?, verifies Con(S|?)                      ?
  ?  • Resolves G_S empirically                          ?
  ?  • Operates pre-deployment and inter-cycle           ?
  ?  • NULL OPERATOR during cycle execution              ?
  ????????????????????????????????????????????????????????
  ?  Layer 1: Internal System S                          ?
  ?  • Axioms ??–??                                     ?
  ?  • All equations §§2–16                              ?
  ?  • Conditionally complete                            ?
  ?  • Unconditionally incomplete                        ?
  ????????????????????????????????????????????????????????
```

---

## Best-Case Operating Envelope

The system maximises yield on a **bullish market — with or without dips — using HODL-through-drawdown patience and no stop losses**.

```
  Price: ~~~~~~/\~~~~~/\/\~~~~~~/\~~~~~~~~~?
              ?        ??      ?
          entries    entries  entries
              ?              ?        ?
             TP             TP       TP
```

| Assumption | Why it matters |
|-----------|----------------|
| **Bullish trend** (any timeframe) | TPs are above entry. The market must eventually reach them. |
| **No stop losses** | Every equation flows from "if TP is hit." SLs bypass this. |
| **Sufficient patience** | Dips are survived by holding, not cutting. |
| **Known fee rates** | Overhead is pre-computed from $f_s$. |
| **Liquid assets** | TP sell orders must fill at the target price. |

---

## Failure Modes Summary

| Failure Mode | Root Cause | Consequence |
|-------------|-----------|-------------|
| Sustained bear market | Price never reaches TP | Capital deployed, 0% liquid, chain halted |
| Exchange insolvency | Counter-party risk | Orders not honoured |
| Fee spike | $f_s$ increases beyond $f_h$ | Coverage ratio drops below 1 |
| SL death spiral | SL triggers ? less capital ? wider TP ? more SL | Negative feedback loop |
| Asset delisting | Market ceases to exist | All positions become worthless |

For complete analysis, see [failure-modes.md](failure-modes.md) and [best-case-use.md](best-case-use.md).

---

## Quick Reference: Complete Data Flow

```
  Market Price P ???????????????????????????????????????????
       ?                                                    ?
       ?                                                    ?
  ???????????   ????????????   ????????????               ?
  ? Sigmoid  ????? Overhead ?????  Entry   ?               ?
  ? ??(t,?)  ?   ? OH, EO   ?   ? Calc §4  ?               ?
  ???????????   ????????????   ????????????               ?
       ?                            ?                       ?
       ?                   ???????????????????              ?
       ?                   ? Serial Gen §8   ?              ?
       ?                   ? [Entry,TP,SL]?  ?              ?
       ?                   ???????????????????              ?
       ?                            ?                       ?
       ???????????????????????????????????????              ?
       ?                   ? Exit Strategy   ?              ?
       ?                   ? Sell qtys §6    ?              ?
       ?                   ???????????????????              ?
       ?                            ?                       ?
       ?                   ???????????????????              ?
       ?                   ? Simulator §11   ????????????????
       ?                   ? Price series    ?
       ?                   ???????????????????
       ?                            ?
       ?                   ???????????????????
       ?                   ? Chain §9        ?
       ?                   ? Cycles compound ?
       ?                   ???????????????????
       ?                            ?
       ?                   ???????????????????
       ????????????????????? Profit / ROI    ?
                           ? §10             ?
                           ? ROI = Net/Cost  ?
                           ???????????????????
```

---

*Generated from [paper.md](paper.md) and [README.md](../README.md). For the complete equation reference, see [equations.md](equations.md).*
