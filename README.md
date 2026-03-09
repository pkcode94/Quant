# Sigmoid-Governed Fee Hedging and Serial Position Management: A Deterministic Framework for Overhead-Neutral Cyclic Trading

**Abstract.** This paper describes a deterministic mathematical framework for constructing, funding, and exiting multi-level trading positions such that all exchange fees, slippage costs, and timing overhead are embedded into the take-profit targets *a priori*. The system uses normalised logistic sigmoids to distribute entry prices, funding allocation, exit sell quantities, and take-profit targets across a configurable number of levels — then chains completed cycles into a compounding sequence with savings diversion. We present the core equations, their derivations from first principles, their purpose within the trading lifecycle, edge-case behaviour, and worked examples under various market conditions.

---

> **⚠️ READ THIS FIRST — Stop Losses and the Limits of Deterministic Fee Hedging**
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
> 3. **Create negative feedback loops.** SL loss → less capital → higher overhead → wider TP → lower fill probability → more SL hits.
> 4. **Conflict with the downtrend buffer.** The buffer pre-funds re-entry after a *profitable* exit. If the exit was an SL loss, the buffer's extra TP margin was wasted — it inflated the TP that was never hit.
>
> **When you might use them anyway:**
>
> - With **fractional SL** ($\phi_{\text{sl}} \ll 1$): trimming 10–25% of a position at SL is survivable. The remaining position can still hit its TP and recover.
> - With **SL hedge buffer** ($n_{\text{sl}} > 0$): pre-funds future SL losses via TP inflation. This restores the deterministic guarantee *on average* — if you expect $n_{\text{sl}}$ SL hits per profitable cycle, the extra TP profit covers them.
> - In **highly leveraged or volatile markets** where a 100% drawdown is possible and any exit is better than liquidation.
>
> **The default configuration ($\phi_{\text{sl}} = 1$, SL off, $n_{\text{sl}} = 0$) is the only configuration where every equation in this paper holds unconditionally.** Any other configuration introduces probabilistic assumptions about SL hit rates that the deterministic framework cannot verify. See [`docs/failure-modes.md`](docs/failure-modes.md) for when the mathematics breaks down and [`docs/best-case-use.md`](docs/best-case-use.md) for the intended operating envelope.

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

**Chain auto-derivation.** In chain mode (§9), $n_f$ is not a manual parameter — it is derived from the chain structure. Cycle $c$ in a $C$-cycle chain sets $n_f = C - 1 - c$: cycle 0 pre-funds $C-1$ future trades, the last cycle pre-funds 0.

### 3.3 Effective Overhead

$$
\text{EO}(P, q) = \text{OH}(P, q) + (s + f_s) \cdot f_h \cdot \Delta t
$$

where $s$ is the surplus rate (desired profit margin above break-even).

**Purpose.** EO is the total TP offset — it includes not just fee recovery (OH) but also the desired profit margin ($s$) and a second fee spread term (to cover the sell-side fee from the exit itself). When a TP is placed at $P_e \cdot (1 + \text{EO})$, the resulting sale covers:

1. The buy-side fee (from OH)
2. The sell-side fee (the second $f_s$ term)
3. The desired surplus ($s$)

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

### 4.4 Edge Cases

- **$N = 1$:** Single entry at $P_{\text{high}}$. The sigmoid normalisation maps $t = 1$ to $\hat\sigma = 1$. All capital is concentrated in one level.

- **$\alpha \to 0$:** Entry prices are linearly distributed. Funding weights become uniform regardless of $r$ (since $\hat\sigma \to t$ and the risk warping balances).

- **$T = 0$:** All $\text{Funding}_i = 0$. No entries are generated. The system gracefully produces empty plans when no capital is available.

- **$R_{\text{above}} > 0$:** Entries extend above current price. This is useful for SHORT positions or for range-bound markets where you want entries on both sides.

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

Each successive level scales the effective overhead linearly. Level 0 is the tightest (just above break-even); level $N-1$ is the widest (most overhead baked in, most profit if hit).

**Note:** These are *total* values (price × quantity), not per-unit. This is intentional — it encodes both the price target and the position size in one number.

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

This bounds TP targets within a known risk envelope. No TP exceeds $P_e \cdot (1 + R_{\max})$ per unit.

### 5.4 Per-Level TP with Risk Warping (`levelTP`)

Used by the Serial Generator (§8) for per-unit TP targets. The steepness is halved ($\alpha' = \alpha / 2$) to prevent over-compression — the entry prices are already sigmoid-distributed, so applying the full steepness again would double-compress the distribution.

$$
t_i = \frac{i + 1}{N + 1}
$$

This mapping uses $(i+1)/(N+1)$ rather than $i/(N-1)$ to keep $t$ strictly inside $(0, 1)$, ensuring every level produces a visibly distinct TP even at extreme steepness.

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

The TP floor is anchored to the *entry* price (each level's own cost basis), while the ceiling is anchored to the *reference* price (the highest entry in the set). This prevents a bell-curve artifact when entries span a wide range — the ceiling is constant, only the sigmoid norm varies.

**SHORT:** The logic is mirrored. TP targets decrease below entry price.

### 5.5 Per-Level Stop-Loss (`levelSL`)

$$
\text{SL}^{\text{long}} = P_e \cdot (1 - \text{EO}), \qquad \text{SL}^{\text{short}} = P_e \cdot (1 + \text{EO})
$$

The stop-loss is placed at exactly the effective overhead distance *below* entry for LONG (above for SHORT). If the SL is hit, the loss equals the overhead — the maximum loss is bounded by the same formula that governs the TP.

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

The integrated worst-case SL loss across all $N$ levels must never exceed the available capital:

$$
\text{TotalSLLoss} = \phi_{\text{sl}} \cdot \sum_{i=0}^{N-1} \text{EO} \cdot \text{Funding}_i \leq T_{\text{avail}}
$$

When violated, $\phi_{\text{sl}}$ is auto-clamped downward so total exposure equals capital exactly. This guarantees that even if every SL fires simultaneously, the total realised loss cannot exceed what was deployed. The cap is applied per cycle in chain mode.

---

## 6. Exit Strategy Calculator

### 6.1 Purpose

Given an open position (from a Buy trade), distribute a sigmoidal fraction of holdings across the TP levels as pending sell orders. Controls *how much* to sell at each level.

### 6.2 Cumulative Sell Distribution

The cumulative fraction sold follows a logistic curve whose centre is controlled by the risk coefficient:

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

where $\phi$ is the exit fraction (total proportion of holdings to sell across all levels).

| $r$ | Sell concentration | Strategy |
|-----|-------------------|----------|
| 0 | Heavy at early (low) TP levels | Lock in gains quickly |
| 0.5 | Balanced across all levels | Moderate |
| 1 | Heavy at late (high) TP levels | Maximise upside, hold for bigger moves |

### 6.3 Net Profit per Level

$$
\text{Net}_i = (P_{\text{TP}}^{(i)} - P_e) \cdot q_i^{\text{sell}} - \text{BuyFee}_i - \text{SellFee}_i
$$

The buy fee is amortised: each level's buy fee share equals $F_{\text{buy}} \cdot f_i^{\text{sell}}$ (proportional to the sell fraction). This ensures fee attribution is fair regardless of how many levels or how the distribution is shaped.

---

## 7. Downtrend Buffer

### 7.1 Purpose

When a position exits at TP, the system can optionally inflate TP targets to pre-fund re-entry if the market subsequently drops. The downtrend buffer answers a concrete question:

> *If the price drops to $P_{\text{buffer}}$ after my exit, what quantity $q_{\text{next}}$ can I re-enter with using only the extra profit from the buffer inflation?*

The answer is always a **fraction** of the current position. This is not a design choice — it is a mathematical inevitability. The extra profit from inflating a TP by a few percent can never fund a full-size re-entry at a comparable price. The buffer buys *part of* the next trade, not the whole thing:

$$
q_{\text{next}} = \frac{\Delta\Pi}{P_{\text{buffer}} \cdot (1 + \mathcal{F})} < q \quad \text{always, when } P_{\text{buffer}} > 0
$$

This makes the buffer a **fractional re-entry hedge**: it guarantees that you can re-enter with a specific (smaller) quantity at a specific (lower) price, with all fees covered.

### 7.2 The Price-Level Guarantee

**Core formula.** Given a desired re-entry price $P_{\text{buffer}}$ and quantity $q_{\text{next}}$, the TP multiplier required:

$$
\boxed{
\text{buffer}_{\text{dt}} = 1 + \frac{n_d \cdot P_{\text{buffer}} \cdot q_{\text{next}} \cdot (1 + \mathcal{F})}{\text{TP}_{\text{base}} \cdot q}
}
$$

| Symbol | Meaning |
|--------|---------|
| $n_d$ | Number of downturn re-entries to pre-fund |
| $P_{\text{buffer}}$ | Price level at which re-entry is guaranteed ($P_{\text{buffer}} < P_e$) |
| $q_{\text{next}}$ | Re-entry quantity ($q_{\text{next}} \leq q$; see §7.2.1) |
| $\mathcal{F} = f_s \cdot f_h \cdot \Delta t$ | Fee component (§3.1) |
| $\text{TP}_{\text{base}} = P_e \cdot (1 + \text{EO})$ | Un-buffered take-profit price |
| $q$ | Current position quantity |

**Derivation.** The extra profit from buffer inflation:

$$
\Delta\Pi = \text{TP}_{\text{base}} \cdot (\text{buffer} - 1) \cdot q
$$

The cost of one fee-neutral re-entry at $P_{\text{buffer}}$:

$$
C_{\text{re}} = P_{\text{buffer}} \cdot q_{\text{next}} \cdot (1 + \mathcal{F})
$$

Setting $\Delta\Pi \geq n_d \cdot C_{\text{re}}$ and solving yields the formula.

**Adjusted TP:**

$$
\text{TP}_{\text{adj}} = \text{TP}_{\text{base}} \cdot \text{buffer}_{\text{dt}}
$$

#### 7.2.1 The Fractional Bound on $q_{\text{next}}$

For any buffer multiplier, the maximum re-entry quantity at price $P_{\text{buffer}}$ is:

$$
\boxed{
q_{\text{next}}^{\max} = \frac{(\text{buffer} - 1) \cdot \text{TP}_{\text{base}} \cdot q}{n_d \cdot P_{\text{buffer}} \cdot (1 + \mathcal{F})}
}
$$

Since $\text{buffer} - 1 \ll 1$ typically (a few percent), and $P_{\text{buffer}} \sim P_e$, this gives:

$$
\frac{q_{\text{next}}^{\max}}{q} \approx \frac{(\text{buffer} - 1) \cdot (1 + \text{EO})}{1 + \mathcal{F}} \cdot \frac{P_e}{P_{\text{buffer}}}
$$

For realistic parameters ($\text{EO} \approx 3\%$, $\text{buffer} \approx 1.03$, $\mathcal{F} \approx 0.2\%$, $P_{\text{buffer}} \approx P_e$):

$$
\frac{q_{\text{next}}^{\max}}{q} \approx 0.03 \times 1.03 / 1.002 \approx 3.1\%
$$

**The buffer funds roughly 3% of the next trade.** This is the correct interpretation — calling it a "downtrend buffer" was a misnomer insofar as it suggested protection against the full downtrend. It protects a *fraction* of the re-entry, scaled by how much TP inflation the system applies.

### 7.3 The Implied Price Level

Every buffer multiplier implies a concrete $P_{\text{buffer}}$ — the maximum re-entry price the extra profit can fund for a given $q_{\text{next}}$. Equivalently, for a given price the buffer implies a maximum quantity (§7.2.1). The two are linked:

$$
\boxed{
P_{\text{buffer}} = \frac{(\text{buffer} - 1) \cdot \text{TP}_{\text{base}} \cdot q}{n_d \cdot q_{\text{next}} \cdot (1 + \mathcal{F})}
}
$$

When $q_{\text{next}} = q$ (full re-entry), this yields the price at which a full-size re-entry is funded. In practice this price is very close to zero — the buffer can only fund a full position at extreme discounts. The useful form is to fix $P_{\text{buffer}}$ near $P_e$ and read off $q_{\text{next}}^{\max}$, which is always a small fraction of $q$.

This makes every buffer reportable in two equivalent ways:

1. *"Your TP is inflated to fund re-entry of 0.003 BTC at \$48,500."* (fixed price, fractional qty)
2. *"Your TP is inflated to fund re-entry of 0.1 BTC at \$1,455."* (full qty, deep discount price)

### 7.4 Sigmoid-Derived $P_{\text{buffer}}$ (Automatic Mode)

When $P_{\text{buffer}}$ is not specified explicitly, the system derives it from the position's characteristics using the axis-dependent sigmoid (§2.1). The sigmoid determines a per-cycle fractional cost $\text{pc}$, from which $P_{\text{buffer}}$ is recovered via §7.3.

**Position delta** (portfolio weight):

$$
\delta = \frac{P \cdot q}{T}
$$

**Hyperbolic compression** (maps $\delta$ to $[0, 1)$):

$$
t = \frac{\delta}{\delta + 1}
$$

**Axis-dependent steepness:**

$$
\alpha_d = \max(\delta, 0.1)
$$

**Asymptotes:**

$$
\text{upper} = \begin{cases}
R_{\max} & \text{if } R_{\max} > 0 \\
\text{EO} & \text{otherwise}
\end{cases}, \qquad
\text{lower} = R_{\min}
$$

**Per-cycle buffer fraction:**

$$
\text{pc} = R_{\min} + \hat\sigma_{\alpha_d}(t) \cdot (\text{upper} - R_{\min})
$$

**Buffer multiplier (sigmoid mode):**

$$
\text{buffer} = 1 + n_d \cdot \text{pc}
$$

**Implied price level** from the sigmoid buffer:

$$
P_{\text{buffer}}^{\text{implied}} = \frac{\text{pc} \cdot \text{TP}_{\text{base}}}{1 + \mathcal{F}} \qquad (\text{when } q_{\text{next}} = q)
$$

This completes the bridge: the sigmoid computes a buffer fraction → the fraction implies a price level → the price level is the guarantee. The two modes (explicit $P_{\text{buffer}}$ vs. sigmoid-derived) produce the same buffer multiplier when $P_{\text{buffer}} = P_{\text{buffer}}^{\text{implied}}$.

### 7.5 Behaviour Table

| $\delta$ | Sigmoid shape | $P_{\text{buffer}}^{\text{implied}}$ | Interpretation |
|----------|---------------|--------------------------------------|----------------|
| $\ll 1$ | Near-linear | Close to $0$ | Small position → covers re-entry only at deep discounts |
| $\approx 1$ | Mild S-curve | Midpoint of range | Balanced protection |
| $\gg 1$ | Steep S-curve | Near $\text{upper} \cdot \text{TP}_{\text{base}} / (1+\mathcal{F})$ | Position dominates portfolio → maximum protection |
| $= 0$ | — | Buffer $= 1$ (disabled) | No position deployed |

### 7.6 Time Sensitivity

When $R_{\max} = 0$, the upper asymptote falls back to $\text{EO}$, which contains the $\Delta t$ factor. This makes $P_{\text{buffer}}^{\text{implied}}$ *time-sensitive* — longer holding periods inflate the guaranteed re-entry level. When $R_{\max} > 0$, it acts as a hard cap.

### 7.7 Stop-Loss Hedge Buffer

Mirrors the downtrend buffer but pre-funds potential future stop-loss hits. When $n_{\text{sl}} > 0$, TPs are inflated so the extra profit covers $n_{\text{sl}}$ future SL events at the configured fractional loss:

$$
\text{buffer}_{\text{sl}} = 1 + n_{\text{sl}} \cdot \phi_{\text{sl}} \cdot \text{pc}
$$

The combined TP multiplier applies both buffers multiplicatively:

$$
\text{TP}_{\text{adj}} = \text{TP}_{\text{base}} \cdot \text{buffer}_{\text{dt}} \cdot \text{buffer}_{\text{sl}}
$$

### 7.8 Deterministic Guarantee

**Theorem.** *The fee-neutral guarantee (§1.2) is preserved by the buffer. If TP is hit, all fees for both the current trade and the buffered fractional re-entry are covered, provided $P_{\text{buffer}} < P_e$ and $q_{\text{next}} \leq q_{\text{next}}^{\max}$.*

**Proof.** The buffer inflates TP by the factor $(\text{buffer} - 1)$. The extra profit $\Delta\Pi = \text{TP}_{\text{base}} \cdot (\text{buffer} - 1) \cdot q$ is deterministic given TP is hit. The re-entry cost $C_{\text{re}} = P_{\text{buffer}} \cdot q_{\text{next}} \cdot (1 + \mathcal{F})$ is fixed at the time the plan is generated. Since $\Delta\Pi \geq n_d \cdot C_{\text{re}}$ by construction (§7.2), the profit from the current trade funds exactly the re-entry.

The re-entry position at $P_{\text{buffer}} < P_e$ has strictly lower overhead:

$$
\text{OH}(P_{\text{buffer}}, q_{\text{next}}) \leq \text{OH}(P_e, q) \quad \text{when } q_{\text{next}} \leq q
$$

because the denominator $P_{\text{buffer}}/q_{\text{next}} \cdot T + K$ is at least as large (lower price, smaller quantity both increase $P/q$). The re-entry position's TP is therefore tighter (closer to entry), making it strictly *easier* to close profitably. $\square$

**What the buffer does NOT guarantee:** It does not fund a full-size re-entry at the same price. The fractional bound (§7.2.1) shows $q_{\text{next}}^{\max} / q \approx \text{buffer} - 1$, which is typically 1–5%. The buffer is a *partial re-entry hedge*, not a drawdown shield.

**Corollary.** $K$ and $\alpha$ constraints are unchanged — $K$ prevents division by zero, $\alpha$ shapes distributions. Neither depends on the buffer mechanism.

### 7.9 Serial Generator Integration

Each level $i$ in the serial plan (§8) has entry price $P_e^{(i)}$. The global price-level buffer must satisfy:

$$
P_{\text{buffer}} < \min_i P_e^{(i)} = P_e^{(0)}
$$

Since $P_e^{(0)}$ is the deepest entry, this guarantees consistency across all levels. The price-level form is *more conservative* than fractional in falling markets: a fixed $P_{\text{buffer}}$ does not shrink as entry prices fall, while a fractional buffer proportionally narrows.

### 7.10 Validation Logic

```cpp
// Buffer from explicit price level and re-entry quantity
double bufferFromPriceLevel(double pBuffer, double qNext,
                            double tpBase, double q,
                            double feeComponent, int nDowntrend)
{
    if (nDowntrend <= 0 || tpBase <= 0.0 || q <= 0.0) return 1.0;
    return 1.0 + (double)nDowntrend * pBuffer * qNext * (1.0 + feeComponent)
                 / (tpBase * q);
}

// Implied price level from any buffer multiplier
double impliedBufferPrice(double buffer, double tpBase, double q,
                          double qNext, double feeComponent, int nDowntrend)
{
    if (nDowntrend <= 0 || qNext <= 0.0 || (1.0 + feeComponent) <= 0.0)
        return 0.0;
    return (buffer - 1.0) * tpBase * q
           / ((double)nDowntrend * qNext * (1.0 + feeComponent));
}

// Maximum re-entry quantity the buffer can fund at a given price (§7.2.1)
double maxBufferedQuantity(double buffer, double tpBase, double q,
                           double pBuffer, double feeComponent, int nDowntrend)
{
    if (nDowntrend <= 0 || pBuffer <= 0.0 || (1.0 + feeComponent) <= 0.0)
        return 0.0;
    return (buffer - 1.0) * tpBase * q
           / ((double)nDowntrend * pBuffer * (1.0 + feeComponent));
}

// Verify: p_buffer must be below ALL entry prices in the plan
bool validatePriceBuffer(double pBuffer,
                         const std::vector<double>& entryPrices)
{
    for (double pe : entryPrices)
        if (pBuffer >= pe) return false;
    return true;
}
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

1. **Self-consistent.** Every entry's TP embeds enough overhead to cover its own fees plus the surplus. There is no external dependency.

2. **Level-independent.** Each tuple is independently viable. If only level 3 of 7 triggers, that single trade is still fee-hedged and profitable at its TP.

3. **Risk-symmetric.** The risk coefficient warps both funding and TP in the same direction. At $r = 1$, deep discounts get more funding *and* higher TP targets — the system allocates more capital where the potential reward is highest.

---

## 9. Chain Execution

### 9.1 Concept

A chain is a sequence of cycles where each cycle is a complete serial plan (§8). When all positions from cycle $c$ close (all TPs or SLs hit), the realised profit is partially diverted to savings and the remainder reinvested into cycle $c+1$.

### 9.2 Cycle Transition

When all positions from cycle $c$ are closed:

$$
\Pi_c = \sum_{\text{sells in cycle } c} \text{Net}_j
$$

$$
\text{Savings}_c = \Pi_c \cdot s_{\text{save}}
$$

$$
T_{c+1} = \text{Capital} - \text{Savings}_c
$$

New entries are generated at the current market price with pump $T_{c+1}$.

### 9.3 Compounding Effect

Because $T_{c+1} > T_c$ (the profit minus savings is reinvested), the overhead for cycle $c+1$ is *lower* than for cycle $c$:

$$
\text{OH}_{c+1} < \text{OH}_c \quad \text{(since } T_{c+1} > T_c \text{)}
$$

This means TP targets compress toward entry prices with each cycle — the system becomes more capital-efficient over time. The surplus rate $s$ becomes the dominant cost, and the chain's per-cycle profit margin stabilises at approximately $s$.

### 9.4 Future Fee Pre-Funding

Each cycle's overhead is automatically scaled by the number of remaining cycles:

$$
n_f^{(c)} = C - 1 - c
$$

Cycle 0 in a 5-cycle chain has $n_f = 4$ (its TP must pre-cover fees for 4 future trades). The last cycle has $n_f = 0$ (self-only). The two effects — growing capital and shrinking $n_f$ — compound, making the convergence toward the surplus-only limit faster than either effect alone.

### 9.5 Superposition Metaphor

Before the market observes (reaches) an entry level, all levels exist as potential states. The price action "collapses" each level into an open position when it reaches the entry price, then "collapses" the exit when the TP is reached. The chain is a sequence of these collapses — a supercoordinate system where each cycle's levels are defined relative to the previous cycle's outcome.

### 9.6 Savings as Irreversible Extraction

The savings diversion $s_{\text{save}}$ is an irreversible extraction from the system. Capital inside the chain compounds; savings exit the chain permanently. This creates a two-compartment model:

$$
\text{Wealth} = \text{Capital}_{\text{chain}} + \text{Savings}_{\text{total}}
$$

The savings rate controls the trade-off between chain growth speed and realised wealth extraction.

---

## 10. Profit Calculation

### 10.1 Unrealised (Open Position)

**LONG:** $\text{Gross} = (P_{\text{now}} - P_e) \cdot q$

**SHORT:** $\text{Gross} = (P_e - P_{\text{now}}) \cdot q$

$$
\text{Net} = \text{Gross} - F_{\text{buy}} - F_{\text{sell}}
$$

$$
\text{ROI} = \frac{\text{Net}}{P_e \cdot q + F_{\text{buy}}} \times 100\%
$$

### 10.2 Realised (Closed Position)

$$
\text{Gross} = (P_{\text{sell}} - P_e) \cdot q_{\text{sold}}
$$

$$
\text{Net} = \text{Gross} - F_{\text{buy}} - F_{\text{sell}}
$$

The ROI denominator includes buy fees because they are part of the cost basis. Sell fees are subtracted from proceeds.

---

## 11. Simulator

### 11.1 Purpose

Steps through a price series (forward or historical), executing the entry/exit logic mechanically. No human judgment is involved — the simulator answers: *"Given this price history and these parameters, what would the system have done?"*

### 11.2 Entry Trigger

At each timestep with price $P_t$, for each unfilled entry level $i$:

$$
\text{Buy if } P_t \leq P_e^{(i)} \text{ and } \text{Cost}_i + \text{Fee}_i \leq \text{Capital}
$$

### 11.3 Exit Trigger

For each open position, check each unfilled exit level $j$:

$$
\text{Sell if } P_t \geq P_{\text{TP}}^{(j)}
$$

### 11.4 Fee Hedging Verification

After a run, the simulator computes:

$$
\text{HedgePool} = \sum_{\text{all exits}} \text{Gross}_j
$$

$$
\text{Coverage} = \frac{\text{HedgePool}}{\text{TotalFees}}
$$

Coverage $\geq 1$ confirms the overhead formula fully covered all trading fees. This is the empirical validation of the theoretical fee neutrality claim.

### 11.5 Chain Mode

When enabled, the simulator detects cycle completion (all positions closed), diverts savings, regenerates entries at the current price, and continues. This produces multi-cycle statistics: cycles completed, total savings, and per-cycle P&L.

---

## 12. Worked Examples

### 12.1 Single Entry — Bull Market

**Scenario:** BTC at $100,000. Pump $10,000. Fee spread 0.1%. Surplus 2%. 4 levels. Risk 0.5.

```
Parameters:
  P = 100,000    T = 10,000    f_s = 0.001
  s = 0.02       N = 4         r = 0.5
  f_h = 1        Δt = 1        K = 0

Overhead:
  F = 0.001 × 1 × 1 = 0.001
  OH = (0.001 × 1) / ((100,000/1) × 10,000 + 0) = 1e-12 ≈ 0
  EO = 0 + (0.02 + 0.001) × 1 × 1 = 0.021

Entry levels (default range, priceLow=0, priceHigh=100,000):
  L0: entry ≈  $2,474    discount 97.5%
  L1: entry ≈ $26,894    discount 73.1%
  L2: entry ≈ $73,106    discount 26.9%
  L3: entry ≈ $97,526    discount  2.5%

Funding (r=0.5, uniform):
  Each level gets ≈ 25% of $10,000 = $2,500

TP at each level ≈ entry × (1 + 0.021) ≈ entry × 1.021
```

**Market plays out:** BTC drops to $73,000 — levels L0, L1, L2 trigger. BTC recovers to $75,000 — L2's TP ($74,641) is hit. The 2.1% overhead covers the exchange fee and delivers 2% surplus. L0 and L1 are still open with much larger upside potential.

**Key insight:** With $10,000 pump on a $100,000 asset, overhead is negligible. The TP is essentially `entry × 1.021` — the surplus rate dominates.

### 12.2 Low Capital — Overhead Matters

**Scenario:** Same as above but pump $50 instead of $10,000.

```
OH = (0.001 × 1) / ((100,000/1) × 50 + 0)
   = 0.001 / 5,000,000 ≈ 2e-10 ≈ 0

EO = 0 + 0.021 = 0.021
```

Even with very low capital, overhead remains negligible for BTC because the price/quantity ratio is enormous. The denominator $P/q \cdot T = 100,000 \times 50 = 5,000,000$ overwhelms the numerator.

**When overhead matters:** High fee spread, high symbol count, low $P/q$ ratio (e.g., buying 1000 units of a $0.05 token), or non-zero $K$.

### 12.3 Range Mode — Sideways Market

**Scenario:** ETH at $3,500. Range ±$200. 4 levels.

```
priceLow  = 3,500 - 200 = 3,300
priceHigh = 3,500 + 200 = 3,700

Entry levels (sigmoid-distributed within ±$200):
  L0: $3,310   (near bottom of range)
  L1: $3,407
  L2: $3,593
  L3: $3,690   (near top of range)
```

In a sideways market, this creates a buy-at-support / sell-at-resistance grid. The sigmoid clustering near the extremes means more entries near the boundaries where reversals are most likely.

### 12.4 Chain Cycle — Compounding Growth

**Scenario:** Starting capital $1,000. Surplus 2%. Savings rate 5%. 3 cycles.

```
Cycle 0:
  Capital: $1,000
  Deploy 4 levels, TP ≈ entry × 1.021
  All TPs hit → profit ≈ $21
  Savings: $21 × 0.05 = $1.05
  Capital for cycle 1: $1,000 + $21 - $1.05 = $1,019.95

Cycle 1:
  Capital: $1,019.95
  OH slightly lower (more capital)
  All TPs hit → profit ≈ $21.42
  Savings: $1.07
  Capital for cycle 2: $1,040.30

Cycle 2:
  Capital: $1,040.30
  Profit ≈ $21.85
  Savings: $1.09
  Final capital: $1,061.06

Total savings extracted: $3.21
Total wealth: $1,064.27
Growth: 6.4% over 3 cycles
```

Each cycle is slightly more efficient than the last because the overhead shrinks. The savings extraction provides a risk-free return regardless of market conditions.

### 12.5 Downtrend Buffer — Bear Market Protection

**Scenario:** Position at $50,000 with qty 0.1. Pump $5,000. EO = 3%. DT count = 2.

```
δ = (50,000 × 0.1) / 5,000 = 1.0
t = 1.0 / (1.0 + 1.0) = 0.5
α_d = max(1.0, 0.1) = 1.0

With R_min = 0, R_max = 0 (EO fallback):
  upper = EO = 0.03
  per_cycle = 0 + σ̂_1.0(0.5) × 0.03
            = 0.5 × 0.03 = 0.015
  buffer = 1 + 2 × 0.015 = 1.03

TP_adjusted = TP_base × 1.03
```

The TP is inflated by 3% to pre-fund 2 downturn cycles. If the market drops 3% after exit, the extra profit covers re-entry at the lower price.

### 12.6 Extrapolating a Chain from a Standalone Trade

**Scenario:** You bought 0.5 BTC at $95,000 (trade #5). Now you want to build a chain around it.

The system:
1. Creates an entry point for trade #5 at $95,000 × 0.5 = $47,500 cost
2. Tags it as cycle 0 in the chain
3. Computes cycle 0's theoretical TP from Serial Gen params
4. Uses the theoretical profit to calculate cycle 1's capital
5. Generates cycles 1, 2, ... with compounding capital
6. Saves all entries and starts chain tracking

The price check then enforces chain ordering: only cycle 0 entries are executable until cycle 0 completes, then cycle 1 unlocks, and so on.

---

## 13. Edge Cases and Degeneracies

### 13.1 Zero Pump ($T = 0$)

When $T = 0$ and $K = 0$: OH = 0/0, clamped to 0. No entries are generated (no capital). The system is inert.

When $T = 0$ and $K > 0$: OH = $\mathcal{F} \cdot n_s / K$. The overhead is determined entirely by the coefficient K, enabling a "fixed overhead" mode independent of capital.

### 13.2 Single Level ($N = 1$)

All sigmoid normalisations map to $\hat\sigma(1) = 1$. Entry at $P_{\text{high}}$, TP at the ceiling. All capital in one level. The system degrades to a simple entry/exit calculator.

### 13.3 Zero Steepness ($\alpha \to 0$)

All sigmoids become linear. Prices are uniformly distributed. Funding weights are uniform. TP targets are linearly distributed. The system becomes a simple grid.

### 13.4 Extreme Steepness ($\alpha \gg 10$)

Sigmoids become step functions. All entries cluster at the extremes of the range. Funding concentrates in one level (depending on risk). TPs concentrate near the floor or ceiling. Useful for binary "all-or-nothing" strategies.

### 13.5 Risk = 0.5 (Uniform)

The warping formula $(1-r) \cdot n + r \cdot (1-n) = 0.5$ for all $n$. Weights are equal. Funding is uniform. TP targets are at the midpoint between floor and ceiling. This is the "no opinion" setting.

### 13.6 Fee Hedging Coefficient $f_h \gg 1$

Over-provisions fee hedging. TP targets move further from entry. Useful when fee rates are uncertain (e.g., volatile gas fees on-chain). The excess becomes additional surplus.

### 13.7 Short Positions

All formulas mirror. TP targets decrease below entry. SL targets increase above entry. The sigmoid distributions apply in reverse direction. The overhead formula is identical — fees are symmetric.

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
| $\text{buffer}_{\text{dt}}$ | §7.2 | Price-level downtrend TP multiplier |
| $q_{\text{next}}^{\max}$ | §7.2.1 | Maximum buffered re-entry quantity (fractional bound) |
| $P_{\text{buffer}}^{\text{implied}}$ | §7.3 | Implied re-entry price from any buffer |
| $\text{buffer}_{\text{sl}}$ | §7.7 | Stop-loss hedge TP multiplier |
| Coverage | §11.4 | Fee hedging verification |
| $\partial\hat\sigma/\partial t$, $\partial\hat\sigma/\partial\alpha$ | §15.1 | Sigmoid gradient primitives |
| $\partial\text{OH}/\partial T$ | §15.2 | Capital-overhead sensitivity |
| $\partial\text{EO}/\partial s$ | §15.3 | Surplus linearity ($= f_h \Delta t$, constant) |
| $\partial\text{TP}/\partial r$ | §15.6 | Risk warps TP distribution |
| $\partial\Pi/\partial s$ | §15.7 | Profit-surplus sensitivity |
| $\partial\text{buffer}/\partial P_{\text{buffer}}$ | §15.8.1 | Price-level buffer sensitivity (linear in $P_{\text{buffer}}$) |
| $dT_c/d\theta$ | §15.9 | Chain recurrence (BPTT structure) |
| $J_1 \ldots J_5$ | §16.2 | Optimisation objectives |

---

## 15. Partial Derivative Catalog

Every equation in the system is differentiable with respect to its continuous parameters. This section derives the partial derivatives needed for gradient-based optimisation.

### 15.1 Sigmoid Primitives

The logistic sigmoid and its derivative:

$$
\sigma(x) = \frac{1}{1 + e^{-x}}, \qquad \sigma'(x) = \sigma(x)\bigl(1 - \sigma(x)\bigr)
$$

For the normalised sigmoid $\hat\sigma_\alpha(t)$, define intermediate quantities:

$$
u = \alpha(t - 0.5), \qquad v = \sigma(u), \qquad S = \sigma_1 - \sigma_0
$$

where $\sigma_0 = \sigma(-\alpha/2)$ and $\sigma_1 = \sigma(\alpha/2)$.

**Derivative with respect to $t$** (input position):

$$
\boxed{
\frac{\partial \hat\sigma}{\partial t} = \frac{\alpha \cdot v(1-v)}{S}
}
$$

**Derivative with respect to $\alpha$** (steepness):

$$
\frac{\partial v}{\partial \alpha} = (t - 0.5) \cdot v(1-v)
$$

$$
\frac{\partial \sigma_0}{\partial \alpha} = -\tfrac{1}{2}\,\sigma_0(1-\sigma_0), \qquad
\frac{\partial \sigma_1}{\partial \alpha} = \tfrac{1}{2}\,\sigma_1(1-\sigma_1)
$$

$$
\frac{\partial S}{\partial \alpha} = \tfrac{1}{2}\bigl[\sigma_1(1-\sigma_1) + \sigma_0(1-\sigma_0)\bigr]
$$

By the quotient rule on $\hat\sigma = (v - \sigma_0)/S$:

$$
\boxed{
\frac{\partial \hat\sigma}{\partial \alpha} =
\frac{(t - 0.5)\,v(1-v) + \tfrac{1}{2}\,\sigma_0(1-\sigma_0)}{S}
\;-\;
\hat\sigma \cdot \frac{\sigma_1(1-\sigma_1) + \sigma_0(1-\sigma_0)}{2S}
}
$$

**Key property:** At $t = 0.5$, $\partial\hat\sigma/\partial\alpha$ simplifies because $v = \sigma(0) = 0.5$ and $v(1-v) = 0.25$. At the midpoint, steepness changes have minimal effect on the output (the sigmoid crosses 0.5 regardless of $\alpha$).

### 15.2 Overhead Derivatives

Let $D = \frac{P}{q} \cdot T + K$ (the denominator). Then $\text{OH} = \frac{\mathcal{F} \cdot n_s}{D}$ where $\mathcal{F} = f_s \cdot f_h \cdot \Delta t$.

**Numerator parameters** (linear in numerator — derivative is $\text{OH}$ divided by the variable):

$$
\frac{\partial\,\text{OH}}{\partial f_s} = \frac{\text{OH}}{f_s}, \qquad
\frac{\partial\,\text{OH}}{\partial f_h} = \frac{\text{OH}}{f_h}, \qquad
\frac{\partial\,\text{OH}}{\partial \Delta t} = \frac{\text{OH}}{\Delta t}, \qquad
\frac{\partial\,\text{OH}}{\partial n_s} = \frac{\text{OH}}{n_s}
$$

**Denominator parameters** (inverse relationship — overhead *decreases* as these grow):

$$
\boxed{
\frac{\partial\,\text{OH}}{\partial T} = -\frac{\text{OH}}{D} \cdot \frac{P}{q}
}
$$

$$
\frac{\partial\,\text{OH}}{\partial K} = -\frac{\text{OH}}{D}, \qquad
\frac{\partial\,\text{OH}}{\partial P} = -\frac{\text{OH}}{D} \cdot \frac{T}{q}, \qquad
\frac{\partial\,\text{OH}}{\partial q} = \frac{\text{OH}}{D} \cdot \frac{P \cdot T}{q^2}
$$

**Interpretation:** $\partial\text{OH}/\partial T < 0$ always — more capital always reduces overhead. The rate of reduction is $\text{OH} \cdot (P/q) / D$, which itself decreases as $T$ grows (diminishing returns).

### 15.3 Effective Overhead Derivatives

Since $\text{EO} = \text{OH} + (s + f_s) \cdot f_h \cdot \Delta t$:

$$
\boxed{
\frac{\partial\,\text{EO}}{\partial s} = f_h \cdot \Delta t
}
$$

This is **constant** — surplus rate has a linear effect on EO regardless of other parameters. This makes surplus the most predictable tuning knob.

$$
\frac{\partial\,\text{EO}}{\partial f_s} = \frac{\text{OH}}{f_s} + f_h \cdot \Delta t, \qquad
\frac{\partial\,\text{EO}}{\partial f_h} = \frac{\text{OH}}{f_h} + (s + f_s) \cdot \Delta t
$$

$$
\frac{\partial\,\text{EO}}{\partial T} = \frac{\partial\,\text{OH}}{\partial T} = -\frac{\text{OH}}{D} \cdot \frac{P}{q}
$$

$$
\frac{\partial\,\text{EO}}{\partial \Delta t} = \frac{\text{OH}}{\Delta t} + (s + f_s) \cdot f_h
$$

### 15.4 Entry Price Derivatives

$P_e^{(i)} = P_{\text{low}} + \hat\sigma(t_i) \cdot (P_{\text{high}} - P_{\text{low}})$

$$
\frac{\partial P_e}{\partial \alpha} = \frac{\partial\hat\sigma}{\partial\alpha} \cdot (P_{\text{high}} - P_{\text{low}})
$$

In range mode where $P_{\text{low}} = P - R_{\text{below}}$, $P_{\text{high}} = P + R_{\text{above}}$:

$$
\boxed{
\frac{\partial P_e}{\partial R_{\text{below}}} = \hat\sigma_i - 1 \leq 0
}
$$

$$
\frac{\partial P_e}{\partial R_{\text{above}}} = \hat\sigma_i \geq 0
$$

**Interpretation:** Increasing range below *lowers* all entry prices (more discount). Increasing range above *raises* all entry prices. The effect is modulated by $\hat\sigma_i$ — entries near $P_{\text{low}}$ are affected more by range below, entries near $P_{\text{high}}$ more by range above.

### 15.5 Funding Allocation Derivatives

Weight per level: $w_i = (1-r) \cdot n_i + r \cdot (1 - n_i)$ where $n_i = \hat\sigma(t_i)$.

$$
\boxed{
\frac{\partial w_i}{\partial r} = 1 - 2n_i
}
$$

This changes sign at $n_i = 0.5$. For levels above the midpoint ($n_i > 0.5$), increasing risk *decreases* their weight. For levels below ($n_i < 0.5$), increasing risk *increases* their weight. At $r = 0.5$, all weights are 0.5 and all funding is uniform.

$$
\frac{\partial w_i}{\partial \alpha} = (1 - 2r) \cdot \frac{\partial n_i}{\partial \alpha}
$$

For the normalised funding fraction $F_i = w_i / W$ where $W = \sum_j w_j$:

$$
\frac{\partial F_i}{\partial r} = \frac{(1 - 2n_i) \cdot W - w_i \cdot \sum_j (1 - 2n_j)}{W^2}
$$

**Special case** at $r = 0.5$: All $w_j = 0.5$, so $W = N/2$, $F_i = 1/N$, and the gradient simplifies to:

$$
\left.\frac{\partial F_i}{\partial r}\right|_{r=0.5} = \frac{2(1 - 2n_i)}{N} - \frac{2}{N^2}\sum_j(1 - 2n_j)
$$

### 15.6 Take-Profit Derivatives (levelTP, LONG)

$\text{TP}_i = \text{TP}_{\min} + n_i \cdot (\text{TP}_{\max} - \text{TP}_{\min})$

where $\text{TP}_{\min} = P_e \cdot (1 + \text{EO} + R_{\min})$, $\text{TP}_{\max} = P_{\text{ref}} \cdot (1 + R_{\max})$, and $n_i = (1-r)\hat\sigma_{\alpha'}(t_i) + r(1 - \hat\sigma_{\alpha'}(t_i))$.

**With respect to surplus rate:**

$$
\boxed{
\frac{\partial\,\text{TP}_i}{\partial s} = (1 - n_i) \cdot P_e \cdot f_h \cdot \Delta t
}
$$

For levels near the TP floor ($n_i \approx 0$), the full $P_e \cdot f_h \cdot \Delta t$ sensitivity applies. For levels near the ceiling ($n_i \approx 1$), surplus has no effect (the ceiling is determined by $R_{\max}$, not surplus).

**With respect to risk bounds:**

$$
\frac{\partial\,\text{TP}_i}{\partial R_{\max}} = n_i \cdot P_{\text{ref}}, \qquad
\frac{\partial\,\text{TP}_i}{\partial R_{\min}} = (1 - n_i) \cdot P_e
$$

**With respect to risk coefficient:**

$$
\boxed{
\frac{\partial\,\text{TP}_i}{\partial r} = (\text{TP}_{\max} - \text{TP}_{\min}) \cdot (1 - 2\hat\sigma_{\alpha'}(t_i))
}
$$

This has the same sign-change structure as the funding derivatives — the TP sigmoid warps in mirror with funding when risk changes.

**With respect to steepness** (through TP sigmoid, using half-steepness $\alpha' = \alpha/2$):

$$
\frac{\partial\,\text{TP}_i}{\partial \alpha} = (\text{TP}_{\max} - \text{TP}_{\min}) \cdot (1 - 2r) \cdot \frac{1}{2}\frac{\partial\hat\sigma_{\alpha'}}{\partial \alpha'}
$$

The factor of $1/2$ comes from $\alpha' = \alpha/2$.

### 15.7 Per-Level Profit Derivatives

Gross profit at level $i$: $\Pi_i = (\text{TP}_i - P_e^{(i)}) \cdot q_i$

where $q_i = T_{\text{avail}} \cdot F_i \,/\, P_e^{(i)}$.

By the product rule:

$$
\frac{\partial \Pi_i}{\partial \theta} =
\frac{\partial\,\text{TP}_i}{\partial \theta} \cdot q_i
+ (\text{TP}_i - P_e^{(i)}) \cdot \frac{\partial q_i}{\partial \theta}
- \frac{\partial P_e^{(i)}}{\partial \theta} \cdot q_i
$$

**With respect to surplus (TP moves, entry and qty stay fixed):**

$$
\boxed{
\frac{\partial \Pi_i}{\partial s} = q_i \cdot (1 - n_i) \cdot P_e \cdot f_h \cdot \Delta t
= \text{Funding}_i \cdot (1 - n_i) \cdot f_h \cdot \Delta t
}
$$

This is always positive — increasing surplus always increases profit per level. The effect is strongest at the TP floor levels and vanishes at the ceiling.

**With respect to risk (both entry price, funding, and TP change):**

The risk derivative involves three coupled terms — entry redistribution, funding reallocation, and TP warping. This is the most complex gradient and the one where optimisation is most valuable.

### 15.8 Downtrend Buffer Derivatives

#### 15.8.1 Price-Level Buffer (§7.2)

$$
\text{buffer} = 1 + \frac{n_d \cdot P_{\text{buffer}} \cdot q_{\text{next}} \cdot (1 + \mathcal{F})}{\text{TP}_{\text{base}} \cdot q}
$$

Let $\beta = n_d \cdot q_{\text{next}} \cdot (1 + \mathcal{F}) / (\text{TP}_{\text{base}} \cdot q)$ — the coefficient of $P_{\text{buffer}}$ in the buffer formula. Then $\text{buffer} = 1 + \beta \cdot P_{\text{buffer}}$.

**With respect to $P_{\text{buffer}}$:**

$$
\boxed{
\frac{\partial\,\text{buffer}}{\partial P_{\text{buffer}}} = \beta = \frac{n_d \cdot q_{\text{next}} \cdot (1 + \mathcal{F})}{\text{TP}_{\text{base}} \cdot q}
}
$$

This is constant — the buffer increases linearly with $P_{\text{buffer}}$. A higher buffer price (more protection) costs proportionally more TP inflation.

**With respect to $q_{\text{next}}$:**

$$
\frac{\partial\,\text{buffer}}{\partial q_{\text{next}}} = \frac{n_d \cdot P_{\text{buffer}} \cdot (1 + \mathcal{F})}{\text{TP}_{\text{base}} \cdot q}
$$

**With respect to pump capital $T$** (through $\text{TP}_{\text{base}} = P_e(1 + \text{EO})$ where $\text{EO}$ depends on $T$):

$$
\frac{\partial\,\text{buffer}}{\partial T} = -\frac{n_d \cdot P_{\text{buffer}} \cdot q_{\text{next}} \cdot (1 + \mathcal{F})}{(\text{TP}_{\text{base}})^2 \cdot q} \cdot \frac{\partial\,\text{TP}_{\text{base}}}{\partial T}
$$

As pump capital grows, $\text{TP}_{\text{base}}$ falls toward $P_e(1 + s \cdot f_h \Delta t)$, and the buffer cost *increases* slightly — more capital means fees are a smaller fraction, so the buffer must provide a larger absolute dollar amount.

#### 15.8.2 Sigmoid-Derived Buffer (§7.4)

When the buffer is computed via sigmoid, $\text{buffer} = 1 + n_d \cdot \text{pc}$ where $\text{pc} = R_{\min} + \hat\sigma_{\alpha_d}(t) \cdot (\text{upper} - R_{\min})$. These derivatives remain unchanged:

**With respect to $R_{\min}$:**

$$
\frac{\partial\,\text{buffer}}{\partial R_{\min}} = n_d \cdot (1 - \hat\sigma_{\alpha_d}(t))
$$

**With respect to $R_{\max}$** (when upper $= R_{\max}$):

$$
\frac{\partial\,\text{buffer}}{\partial R_{\max}} = n_d \cdot \hat\sigma_{\alpha_d}(t)
$$

**With respect to pump capital $T$** (through $\delta = Pq/T$):

$$
\frac{\partial \delta}{\partial T} = -\frac{\delta}{T}, \qquad
\frac{\partial t}{\partial \delta} = \frac{1}{(\delta + 1)^2}
$$

The full gradient chains through $\delta \to t \to \hat\sigma \to \text{pc} \to \text{buffer}$ and also through $\delta \to \alpha_d \to \hat\sigma$, creating a coupled nonlinear sensitivity.

### 15.8b Stop-Loss Buffer Derivatives

$\text{buffer}_{\text{sl}} = 1 + n_{\text{sl}} \cdot \phi_{\text{sl}} \cdot \text{pc}$

**With respect to SL fraction:**

$$
\boxed{
\frac{\partial\,\text{buffer}_{\text{sl}}}{\partial \phi_{\text{sl}}} = n_{\text{sl}} \cdot \text{pc}
}
$$

**With respect to SL hedge count** (continuous relaxation):

$$
\frac{\partial\,\text{buffer}_{\text{sl}}}{\partial n_{\text{sl}}} = \phi_{\text{sl}} \cdot \text{pc}
$$

### 15.9 Chain Compounding Derivatives

Capital at cycle $c+1$: $T_{c+1} = T_c + \Pi_c - \text{Sav}_c$ where $\text{Sav}_c = \Pi_c \cdot s_{\text{save}}$, so $T_{c+1} = T_c + \Pi_c(1 - s_{\text{save}})$.

**With respect to savings rate:**

$$
\boxed{
\frac{\partial T_{c+1}}{\partial s_{\text{save}}} = -\Pi_c
}
$$

More savings directly reduces next-cycle capital, dollar for dollar.

**Recursive chain rule for multi-cycle profit:**

Total chain profit $J = \sum_{c=0}^{C-1} \Pi_c(\theta, T_c)$ where $T_c$ depends on all previous cycles:

$$
\frac{dJ}{d\theta} = \sum_{c=0}^{C-1} \left[
\frac{\partial \Pi_c}{\partial \theta}
+ \frac{\partial \Pi_c}{\partial T_c} \cdot \frac{dT_c}{d\theta}
\right]
$$

$$
\frac{dT_c}{d\theta} = (1 - s_{\text{save}}) \cdot \left[
\frac{\partial \Pi_{c-1}}{\partial \theta}
+ \frac{\partial \Pi_{c-1}}{\partial T_{c-1}} \cdot \frac{dT_{c-1}}{d\theta}
\right]
$$

This is a **recurrence relation** — the gradient at cycle $c$ depends on the gradient at cycle $c-1$. This is structurally identical to backpropagation through time (BPTT) in recurrent neural networks. The chain's cycle sequence is the "time axis" and each cycle's serial plan is a "layer".

**With respect to surplus (through multiple cycles):**

$$
\frac{dJ}{ds} = \sum_{c=0}^{C-1} \frac{\partial \Pi_c}{\partial s}
+ (1 - s_{\text{save}}) \sum_{c=1}^{C-1} \frac{\partial \Pi_c}{\partial T_c} \cdot \frac{dT_c}{ds}
$$

The first term is the direct effect (higher surplus = higher profit per cycle). The second is the indirect effect (higher surplus in cycle $c$ means more capital in cycle $c+1$, further amplified by compounding).

### 15.10 Gradient Summary Table

| Output | w.r.t. | Gradient | Sign | Interpretation |
|--------|--------|----------|------|---------------|
| OH | $T$ | $-\text{OH} \cdot (P/q) / D$ | $-$ | More capital → less overhead |
| OH | $f_s$ | $\text{OH}/f_s$ | $+$ | Higher fees → more overhead |
| EO | $s$ | $f_h \cdot \Delta t$ | $+$ | Constant — surplus is linear |
| $P_e$ | $\alpha$ | $\partial\hat\sigma/\partial\alpha \cdot \Delta P$ | $\pm$ | Steepness shifts entries |
| $F_i$ | $r$ | $(1-2n_i)/W - F_i \cdot \Sigma(1-2n_j)/W$ | $\pm$ | Risk reallocates capital |
| TP | $s$ | $(1-n_i) \cdot P_e \cdot f_h \cdot \Delta t$ | $+$ | Surplus raises TP floor |
| TP | $R_{\max}$ | $n_i \cdot P_{\text{ref}}$ | $+$ | Ceiling scales with reference |
| TP | $r$ | $(\text{TP}_{\max}-\text{TP}_{\min})(1-2\hat\sigma)$ | $\pm$ | Warps TP distribution |
| $\Pi_i$ | $s$ | $\text{Funding}_i \cdot (1-n_i) \cdot f_h \cdot \Delta t$ | $+$ | More surplus → more profit |

---

## 16. Project Structure

```
quant/
├── Quant/                      # Desktop app (CLI + HTTP/web UI)
│   ├── Quant.cpp               #   main() — CLI menu + HTTP server
│   ├── QuantMath.h             #   Core math (overhead, sigmoid, serial plans)
│   ├── TradeDatabase.h         #   JSON persistence (trades, entries, chains, wallet)
│   ├── ProfitCalculator.h      #   Profit / ROI computation
│   ├── MultiHorizonEngine.h    #   TP/SL horizon generation
│   ├── MarketEntryCalculator.h #   Entry level distribution
│   ├── ExitStrategyCalculator.h#   Exit sell distribution
│   ├── Simulator.h             #   Forward simulation / backtesting
│   ├── ChainOptimizer.h        #   BPTT chain parameter optimization
│   ├── CudaAccelerator.h       #   GPU abstraction (#ifdef QUANT_CUDA)
│   ├── CudaKernels.cu          #   CUDA simulation kernels
│   ├── GpuLLM.h / .cu          #   MoE-Transformer + LSTM (GPU)
│   ├── HttpApi.h               #   HTTP server bootstrap
│   ├── HtmlHelpers.h           #   HTML rendering helpers
│   ├── Routes_*.h              #   Web UI route handlers
│   ├── Trade.h                 #   Trade types
│   ├── IdGenerator.h           #   ID allocation
│   ├── PriceSeries.h           #   Time-series price data
│   ├── SymbolRegistry.h        #   Symbol name ↔ ID mapping
│   ├── UserManager.h           #   Multi-user auth + sessions
│   ├── AdminConfig.h           #   Admin settings
│   ├── DiscussionLedger.h      #   Post-hoc trade analysis ledger
│   ├── json.h                  #   nanojson3 (vendored)
│   └── cpp-httplib-master/     #   cpp-httplib (vendored)
│
├── libquant/                   # Pure math library (header-only)
│   └── include/
│       └── quantmath.h         #   Standalone copy of QuantMath
│
├── libquant-engine/            # Backend engine library (C API)
│   ├── include/
│   │   ├── quant_engine.h      #   C API facade
│   │   └── quant_engine_types.h#   Portable data types
│   └── src/
│       └── quant_engine.cpp    #   Implementation
│
├── quant-ui/                   # GUI provider library (virtual router)
│   ├── include/
│   │   └── quant_ui.h          #   C API: qui_get/qui_post → HTML
│   ├── src/
│   │   └── quant_ui.cpp        #   Page rendering (no HTTP dependency)
│   ├── jni/
│   │   └── quant_ui_jni.cpp    #   JNI bridge for Android
│   ├── java/com/quant/ui/
│   │   ├── QuantUI.java        #   Java virtual router API
│   │   ├── UIResponse.java     #   Response DTO (html or redirect)
│   │   └── QuantWebView.java   #   WebView with form interception
│   └── CMakeLists.txt          #   Build
│
├── quant-android/              # Android JNI bridge (engine)
│   ├── jni/
│   │   └── quant_jni.cpp       #   JNI ↔ C API bridge
│   ├── java/com/quant/engine/
│   │   └── QuantEngine.java    #   Java native API
│   └── CMakeLists.txt          #   NDK build
│
├── quant-app/                  # Android Studio project (ready to open)
│   ├── settings.gradle         #   Gradle project config
│   ├── app/
│   │   ├── build.gradle        #   Module with NDK/CMake
│   │   └── src/main/
│   │       ├── cpp/CMakeLists.txt  # NDK build → libquant-native.so
│   │       ├── AndroidManifest.xml
│   │       └── java/com/quant/
│   │           ├── app/MainActivity.java
│   │           ├── ui/{QuantUI,QuantWebView,UIResponse}.java
│   │           └── engine/QuantEngine.java
│   └── gradle/wrapper/         #   Gradle wrapper config
│
├── CMakeLists.txt              # Root CMake (desktop + Android)
├── Makefile                    # GNU Make (Linux / Termux)
└── README.md                   # This document
```

### Architecture

```
┌─────────────┐   ┌──────────────┐   ┌─────────────────┐
│ Quant (CLI)  │   │ Quant (HTTP) │   │ Android App     │
│ Quant.cpp    │   │ Routes_*.h   │   │ (Java/Kotlin)   │
└──────┬───────┘   └──────┬───────┘   └────────┬────────┘
       │                  │                     │
       │                  │              ┌──────┴────────┐
       │                  │              │ QuantWebView  │
       │                  │              │ (form routing)│
       │                  │              └──────┬────────┘
       │                  │                     │ JNI
       │                  │              ┌──────┴────────┐
       │                  │              │ quant_ui.h    │
       │                  │              │ (virtual      │
       │                  │              │  router)      │
       │                  │              └──────┬────────┘
       ▼                  ▼                     ▼
┌──────────────────────────────┐   ┌────────────────────┐
│ TradeDatabase / ProfitCalc   │   │ quant_engine.h     │
│ Simulator / ChainOptimizer   │◄──│ (C API facade)     │
│ QuantMath (core equations)   │   │ libquant-engine    │
└──────────────────────────────┘   └────────────────────┘
       │
       ▼ (optional)
┌──────────────────┐
│ CudaKernels.cu   │
│ GpuLLM.cu        │
│ (#ifdef QUANT_CUDA)
└──────────────────┘
```

---

## 17. Building

### Windows (Visual Studio)

Open `Quant.sln`.  Build as Debug or Release.  CUDA is optional — controlled by the `QUANT_CUDA` preprocessor define and `.cu` files in the project.

### Linux / Termux (Make)

```bash
# CPU-only (no CUDA)
make

# With GPU support
make CUDA=1

# Termux (Android terminal)
pkg install clang make
make CXX=clang++

# Engine library only
make engine
```

### CMake (cross-platform)

```bash
# Desktop
cmake -B build
cmake --build build

# Android NDK (command-line)
cmake -B build-android \
  -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DQUANT_ANDROID=ON
cmake --build build-android
```

### Android Studio

Open `quant-app/` as an existing project in Android Studio.  The NDK/CMake integration is pre-configured — Gradle builds all C++ into `libquant-native.so` automatically.

Requirements: Android Studio with NDK and CMake installed (SDK Manager → SDK Tools → NDK, CMake).

### CUDA Isolation

All GPU code is behind `#ifdef QUANT_CUDA`:

- `CudaAccelerator.h` — real API when `QUANT_CUDA` defined, no-op stubs otherwise
- `GpuLLM.h` — same pattern
- `CudaKernels.cu` / `GpuLLM.cu` — only compiled when CUDA is enabled
- `ChainOptimizer.h` — uses `CudaAccelerator` stubs transparently; CPU fallback path always available

No CUDA headers or libraries are required for a CPU-only build.

---

## 18. Running

The application starts both a **CLI menu** and an **HTTP web UI** simultaneously:

```
$ ./build/quant
  [HTTP] listening on http://localhost:8080

  QUANT TRADE MANAGER
  ----------------------------------------
   1) List trades
   2) Add trade
   ...
  25) Serial generator
  26) Chain operations
  27) Simulator
  ...
   0) Exit
  ----------------------------------------
```

### CLI Features

| # | Feature | GUI Equivalent |
|---|---------|----------------|
| 1–5 | Trade CRUD | `/trades` |
| 6 | Profit calculation | `/profit` |
| 7–8 | Horizon generation & view | `/generate-horizons` |
| 9 | Toggle stop-loss | `/trades` (edit) |
| 10 | DCA tracker | `/dca` |
| 11 | Portfolio summary | `/portfolio` |
| 12 | Unrealized P&L | `/pnl` |
| 14 | Price check (TP/SL vs market) | `/price-check` |
| 15 | Market entry calculator | `/market-entry` |
| 16 | Exit strategy | `/exit-strategy` |
| 20 | Wallet (deposit/withdraw) | `/wallet` |
| 21 | Pending exit orders | `/pending-exits` |
| 23–24 | Entry points (view/mark traded) | `/entry-points` |
| 25 | Serial generator | `/serial-generator` |
| 26 | Chain operations | `/chains` |
| 27 | Simulator | `/simulator` |
| 28 | P&L ledger | `/pnl` |
| 30 | Execute buy/sell | `/trades` (execute) |
| 31 | Add exit strategy | `/exit-strategy` |
| 33–34 | Chain advance/reset | `/chains/:id` |
| 37 | hledger export | `/export-hledger` |
| 38 | Price series | `/symbols` |

### Web UI

Navigate to `http://localhost:8080` in a browser.  All features from the CLI are available through the web interface plus:

- **Chart** — interactive price + TP/SL visualization
- **BPTT Optimizer** — gradient-based chain parameter optimization (GPU-accelerated when available)
- **Chain Manager** — multi-chain CRUD, cycle management, entry progress tracking
- **Premium / Admin** — multi-user support, payment processing

### Android (future)

The `quant-ui` virtual router provides the same web UI as the desktop HTTP server, but without any network dependency. On Android, `QuantWebView` renders pages locally and intercepts form submissions:

```java
// In your Activity's onCreate:
QuantUI.init(getFilesDir().getAbsolutePath() + "/db", "Quant");

QuantWebView webView = findViewById(R.id.webview);
webView.navigateTo("/");   // renders the dashboard

// That's it — all navigation, forms, and page rendering
// is handled internally by the virtual router.
// No HTTP server, no port listening.
```

For data-only access without UI (e.g. background sync, notifications):

```java
QuantEngine engine = new QuantEngine();
engine.open(getFilesDir().getAbsolutePath() + "/db");
double balance = engine.walletBalance();
int chains = engine.chainCount();
double oh = engine.overhead(price, qty, feeSpread, ...);
```

The Android app builds `libquant-engine` + `libquant-ui` as shared libraries via NDK CMake, then loads them through `QuantEngine.java` and `QuantUI.java`.
| Buffer | $R_{\min}$ | $n_d(1-\hat\sigma)$ | $+$ | Floor raises buffer |
| OH | $n_f$ | $\text{OH}/(1+n_f)$ | $+$ | Each future trade adds marginal overhead |
| $\text{buffer}_{\text{sl}}$ | $\phi_{\text{sl}}$ | $n_{\text{sl}} \cdot \text{pc}$ | $+$ | More SL fraction → more TP inflation |
| $T_{c+1}$ | $s_{\text{save}}$ | $-\Pi_c$ | $-$ | Savings drain capital |

---

## 16. Gradient-Based Optimisation Framework

### 16.1 The Optimisation Problem

The system has a parameter vector $\theta$ of continuous tunable parameters and produces a scalar objective $J(\theta)$ that we want to maximise (or minimise). The question is: *given the current market context, what parameter settings maximise a chosen objective?*

**Tunable parameter vector:**

$$
\theta = \bigl[s,\; r,\; \alpha,\; f_h,\; R_{\max},\; R_{\min},\; s_{\text{save}},\; R_{\text{above}},\; R_{\text{below}},\; \Delta t\bigr]
$$

**Fixed context** (not optimised — determined by market or user):

$$
\text{context} = \bigl[P,\; q,\; T,\; f_s,\; K,\; n_s,\; N,\; n_d\bigr]
$$

$N$ and $n_d$ are discrete and handled separately (grid search or enumeration).

**Box constraints** (valid parameter ranges):

| Parameter | Lower | Upper | Rationale |
|-----------|-------|-------|-----------|
| $s$ | 0 | $\infty$ | Non-negative surplus |
| $r$ | 0 | 1 | Risk coefficient domain |
| $\alpha$ | 0.1 | 20 | Steepness range |
| $f_h$ | 1 | 10 | Hedging multiplier |
| $R_{\max}$ | 0 | 1 | Max TP fraction |
| $R_{\min}$ | 0 | $R_{\max}$ | Floor ≤ ceiling |
| $s_{\text{save}}$ | 0 | 0.99 | Can't save 100% |
| $R_{\text{above}}$, $R_{\text{below}}$ | 0 | $P$ | Range within price |
| $\Delta t$ | 0.01 | 10 | Time scaling |

### 16.2 Objective Functions

Five practical objectives, each suited to a different trading goal:

**Objective 1: Maximum Profit (MaxProfit)**

$$
J_1(\theta) = \sum_{i=0}^{N-1} (\text{TP}_i - P_e^{(i)}) \cdot q_i
$$

Maximises raw dollar profit across all levels if every TP is hit. Gradient-dominant parameter: $s$ (via $\partial\Pi/\partial s > 0$).

*Risk:* Unconstrained maximisation pushes $s \to \infty$, moving TPs far from entry. Must be constrained by a TP distance penalty or hard bound on $s$.

**Objective 2: Minimum TP Spread (MinSpread)**

$$
J_2(\theta) = -\sum_{i=0}^{N-1} \left(\frac{\text{TP}_i - P_e^{(i)}}{P_e^{(i)}}\right)^2
$$

Minimises the *relative* TP distance from entry (tighter targets = higher fill probability). Subject to the hard constraint $\text{TP}_i \geq P_{\text{BE}}^{(i)}$ (must cover fees).

*Use case:* Sideways/range-bound markets where you want the fastest possible cycle turnover. The optimiser will push $s$ toward zero and $R_{\max}$ toward overhead.

**Objective 3: Maximum ROI (MaxROI)**

$$
J_3(\theta) = \frac{\sum_i (\text{TP}_i - P_e^{(i)}) \cdot q_i}{\sum_i \text{Funding}_i}
= \frac{\sum_i (\text{TP}_i - P_e^{(i)}) \cdot q_i}{T_{\text{avail}}}
$$

Maximises return on invested capital. Unlike MaxProfit, this penalises over-deployment. Gradient-dominant parameters: $r$ and $\alpha$ (which control *where* capital goes).

*Use case:* Capital-constrained traders who want the best bang per dollar.

**Objective 4: Maximum Chain Growth (MaxChain)**

$$
J_4(\theta) = \frac{T_C}{T_0} = \prod_{c=0}^{C-1}\left(1 + \frac{\Pi_c(1 - s_{\text{save}})}{T_c}\right)
$$

Maximises the capital growth ratio over $C$ cycles. This naturally handles the savings trade-off: higher $s_{\text{save}}$ extracts wealth but slows growth.

Taking $\log$ for numerical stability:

$$
\log J_4 = \sum_{c=0}^{C-1} \log\!\left(1 + \frac{\Pi_c(1-s_{\text{save}})}{T_c}\right)
$$

Gradient via the chain recurrence (§15.9).

*Use case:* Long-term chain operators who want maximum compounding speed.

**Objective 5: Maximum Wealth Extraction (MaxWealth)**

$$
J_5(\theta) = T_C + \sum_{c=0}^{C-1} \text{Sav}_c
= T_C + \sum_{c=0}^{C-1} \Pi_c \cdot s_{\text{save}}
$$

Maximises total *wealth* (capital remaining + savings extracted). This is the true terminal objective. The gradient with respect to $s_{\text{save}}$ reveals the optimal extraction rate:

$$
\frac{\partial J_5}{\partial s_{\text{save}}} = \sum_{c=0}^{C-1} \Pi_c + \frac{\partial T_C}{\partial s_{\text{save}}}
$$

The first term is positive (more savings = more extracted). The second is negative (more savings = less capital for future cycles). The optimum is where these balance.

### 16.3 Composite Objective with Penalty Terms

In practice, combine objectives with weights and add penalty terms for constraints:

$$
J(\theta) = \lambda_1 J_1 + \lambda_2 J_2 + \lambda_3 J_3 + \lambda_4 J_4 + \lambda_5 J_5
- \mu_{\text{TP}} \sum_i \max(0,\; P_{\text{BE}}^{(i)} - \text{TP}_i)^2
- \mu_{\text{dist}} \sum_i \max\!\left(0,\; \frac{\text{TP}_i - P_e^{(i)}}{P_e^{(i)}} - d_{\max}\right)^2
$$

where $d_{\max}$ is the maximum acceptable TP distance (e.g., 10%).

### 16.4 Gradient Computation

**Analytical gradients** (exact, fast, recommended for single-level parameters):

The derivatives from §15 compose via the chain rule. For the serial plan:

$$
\frac{\partial J}{\partial \theta} = \sum_{i=0}^{N-1}
\frac{\partial J}{\partial \Pi_i} \cdot \frac{\partial \Pi_i}{\partial \theta}
$$

where $\partial\Pi_i/\partial\theta$ expands via §15.7.

**Numerical gradients** (approximate, simpler to implement, required for coupled parameters):

$$
\frac{\partial J}{\partial \theta_k} \approx \frac{J(\theta + \epsilon \mathbf{e}_k) - J(\theta - \epsilon \mathbf{e}_k)}{2\epsilon}
$$

with $\epsilon = 10^{-7} \cdot \max(|\theta_k|, 1)$. Central differences give $O(\epsilon^2)$ accuracy.

**Recommendation:** Use analytical gradients for $s$, $r$, $R_{\max}$, $R_{\min}$, $s_{\text{save}}$ (clean closed-form derivatives). Use numerical gradients for $\alpha$ and range parameters (complex sigmoid chain rule). The mixed approach gives the best speed/accuracy trade-off.

### 16.5 Projected Gradient Descent

Standard gradient descent with projection onto the feasible set (box constraints):

$$
\theta^{(k+1)} = \text{proj}\!\left(\theta^{(k)} + \eta_k \cdot \nabla J(\theta^{(k)}),\; \Theta\right)
$$

where $\text{proj}(\theta, \Theta)$ clamps each component to its valid range.

**Learning rate schedule** (cosine annealing to avoid oscillation near optimum):

$$
\eta_k = \eta_{\min} + \frac{1}{2}(\eta_{\max} - \eta_{\min})\left(1 + \cos\frac{\pi k}{K}\right)
$$

**Convergence criterion:**

$$
\|\nabla J(\theta^{(k)})\|_\infty < \varepsilon_{\text{tol}} \quad \text{or} \quad k \geq K_{\max}
$$

**Typical hyperparameters:**

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| $\eta_{\max}$ | $10^{-3}$ | Avoid overshooting sigmoid transitions |
| $\eta_{\min}$ | $10^{-6}$ | Fine-tuning near optimum |
| $K_{\max}$ | 200 | Convergence is fast for smooth objectives |
| $\varepsilon_{\text{tol}}$ | $10^{-8}$ | Numerical precision limit |

### 16.6 Practical Workflow

For an optimisation tab in the chart UI:

1. **User selects objective** from dropdown (MaxProfit / MinSpread / MaxROI / MaxChain / MaxWealth)
2. **User locks parameters** they don't want changed (e.g., lock $s$ if they have a fixed target margin)
3. **User sets constraint**: maximum acceptable TP distance $d_{\max}$ (e.g., "TP no more than 5% above entry")
4. **System runs optimisation** using current params as initial $\theta^{(0)}$, displaying convergence curve
5. **System shows before/after comparison**: parameter table, chart overlay of old vs. new levels
6. **User accepts or rejects** the optimised parameters

### 16.7 Expected Behaviour by Objective

| Objective | $s$ tends to | $r$ tends to | $\alpha$ tends to | $s_{\text{save}}$ tends to |
|-----------|-------------|-------------|-------------------|--------------------------|
| MaxProfit | Increase (higher TP) | Depends on price range | Moderate (spread entries) | N/A (single cycle) |
| MinSpread | Decrease (tighter TP) | 0.5 (uniform — minimises variance) | Low (linear distribution) | N/A |
| MaxROI | Increase but bounded | Increase (aggressive — more at discounts) | High (concentrate at extremes) | N/A |
| MaxChain | Moderate (balance growth/fill) | Moderate | Moderate | Decrease (reinvest more) |
| MaxWealth | Moderate | Moderate | Moderate | Interior optimum (balance extraction/growth) |

### 16.8 Sensitivity Analysis

Before running optimisation, compute the gradient magnitude for each parameter to identify which ones matter most:

$$
\text{sensitivity}_k = \left|\frac{\partial J}{\partial \theta_k}\right| \cdot \frac{\theta_k}{J}
$$

This is the *elasticity* — the percentage change in objective per percentage change in parameter. Parameters with elasticity near zero can be locked (they don't affect the objective). Parameters with high elasticity are the levers worth optimising.

**Typical sensitivity ordering** (highest to lowest):

1. $s$ (surplus rate) — almost always the dominant parameter
2. $R_{\max}$ (max risk) — controls TP ceiling
3. $r$ (risk coefficient) — reallocates capital and TP
4. $s_{\text{save}}$ (savings rate) — only matters for chain objectives
5. $\alpha$ (steepness) — second-order shape effect
6. $f_h$ (fee hedging) — matters only when fees are significant
7. $R_{\min}$, $R_{\text{above}}$, $R_{\text{below}}$, $\Delta t$ — typically low sensitivity

---

## 17. Conclusion

The framework presented here transforms the problem of profitable trading from *prediction* (guessing where the market goes) into *engineering* (computing what must happen for a known outcome). The overhead formula guarantees fee neutrality. The sigmoid distributions provide tunable, consistent shapes across all dimensions. The chain execution compounds capital across cycles with built-in savings extraction.

The system does not predict whether entries will trigger or TPs will be hit. It guarantees that *if* they are hit, the result is deterministic: fees are covered, surplus is captured, and the next cycle is funded. The market provides the randomness; the equations provide the structure.

The partial derivatives (§15) reveal that the surplus rate $s$ is the fundamental parameter — it has a constant, positive gradient on EO, making it the most predictable tuning knob. All other costs (fees, hedging, spread) are engineering details that the overhead formula absorbs. As capital grows across chain cycles, overhead vanishes, and the system converges to its theoretical limit: a cycle machine generating $s$% per round.

The gradient-based optimisation framework (§16) enables automatic parameter tuning. The chain compounding gradient has the structure of backpropagation through time — each cycle is a "layer" and the gradient flows backward through the recurrence relation. This means standard deep learning optimisation techniques (gradient clipping, learning rate scheduling, momentum) apply directly. The five objective functions cover the full spectrum from conservative (MinSpread) to aggressive (MaxChain), with MaxWealth as the true terminal objective that reveals the optimal savings extraction rate as a balance point between two opposing gradients.

---

## 18. CUDA Backpropagation Architecture (GpuLLM)

The system includes a GPU-accelerated Sparse MoE-Transformer with LSTM-gated expert routing, implemented in CUDA with manual backward kernels. This section documents the architecture and gradient flow.

### 18.1 Model Architecture

Each decoder block follows the pattern:

$$
\text{LayerNorm} \to \text{Causal MHA} \to \text{Residual} \to \text{LayerNorm} \to \text{LSTM Router} \to \text{Top-K MoE FFN} \to \text{Residual}
$$

**Configuration** (`LlmConfig`):

| Parameter | Default | Meaning |
|-----------|---------|---------|
| `inputDim` | 8 | Raw feature vector (OHLCV + indicators) |
| `dModel` | 256 | Embedding / hidden dimension |
| `nHeads` | 4 | Multi-head attention heads |
| `nLayers` | 6 | Decoder blocks |
| `nExperts` | 8 | MoE experts per layer |
| `topK` | 2 | Active experts per token |
| `lstmHidden` | 128 | LSTM router hidden state |
| `seqLen` | 512 | Max context window |
| `ffnDim` | 512 | Expert FFN intermediate dim |
| `outputDim` | 1 | Output size (scalar for price prediction) |

**Total parameters:**

$$
|\theta| = \underbrace{d_{\text{in}} \cdot D + D}_{\text{embedding}}
+ L \cdot \left(
\underbrace{2D}_{\text{LN1}}
+ \underbrace{D \cdot 3D + 3D}_{\text{QKV}}
+ \underbrace{D^2 + D}_{\text{Wo}}
+ \underbrace{2D}_{\text{LN2}}
+ \underbrace{D \cdot 4H + H \cdot 4H + 4H}_{\text{LSTM}}
+ \underbrace{H \cdot E + E}_{\text{Router}}
+ \underbrace{E(DF + F + FD + D)}_{\text{Experts}}
\right)
+ \underbrace{D \cdot d_{\text{out}} + d_{\text{out}}}_{\text{Output head}}
$$

### 18.2 Forward Pass

The forward pass (`llmForward`) is fully GPU-resident. All matrix operations use cuBLAS `cublasDgemm` in row-major layout.

**Embedding:**

$$
\mathbf{E} = \mathbf{X} \mathbf{W}_e + \mathbf{b}_e \qquad [\text{seqLen} \times D]
$$

**Per-layer ($l = 0, \ldots, L-1$):**

1. **LayerNorm 1** (per-row):

$$
\hat{x}_r = \frac{x_r - \mu_r}{\sqrt{\sigma_r^2 + \varepsilon}} \cdot \gamma + \beta
$$

2. **QKV Projection:**

$$
[\mathbf{Q}, \mathbf{K}, \mathbf{V}] = \text{LN}_1(\mathbf{h}) \cdot \mathbf{W}_{qkv} + \mathbf{b}_{qkv} \qquad [S \times 3D]
$$

3. **Causal Multi-Head Attention** (per head $h$):

$$
\mathbf{A}_h = \text{softmax}\!\left(\frac{\mathbf{Q}_h \mathbf{K}_h^\top}{\sqrt{d_h}} + \mathbf{M}_{\text{causal}}\right) \cdot \mathbf{V}_h
$$

4. **Output Projection + Residual:**

$$
\mathbf{h}' = \mathbf{h} + \mathbf{A} \cdot \mathbf{W}_o + \mathbf{b}_o
$$

5. **LayerNorm 2** on $\mathbf{h}'$.

6. **LSTM Router** (sequential over time):

$$
\mathbf{g}_t = \sigma(\mathbf{W}_{ih} \mathbf{x}_t + \mathbf{W}_{hh} \mathbf{h}_{t-1} + \mathbf{b})
$$

$$
\mathbf{c}_t = \mathbf{f}_t \odot \mathbf{c}_{t-1} + \mathbf{i}_t \odot \tilde{\mathbf{c}}_t, \qquad
\mathbf{h}_t = \mathbf{o}_t \odot \tanh(\mathbf{c}_t)
$$

7. **Top-K Expert Routing:**

$$
\text{logits} = \mathbf{h}_{\text{lstm}} \cdot \mathbf{W}_r + \mathbf{b}_r, \qquad
\text{weights} = \text{softmax}(\text{logits}), \qquad
\text{select top-}K
$$

8. **Expert FFN** (per selected expert):

$$
\mathbf{m} = \text{GELU}(\mathbf{x} \cdot \mathbf{W}_1^{(e)} + \mathbf{b}_1^{(e)}), \qquad
\mathbf{y} = \mathbf{m} \cdot \mathbf{W}_2^{(e)} + \mathbf{b}_2^{(e)}
$$

9. **MoE Combine + Residual:**

$$
\mathbf{h}_{\text{out}} = \mathbf{h}' + \sum_{k} w_k \cdot \mathbf{y}_{e_k}
$$

**Output Head:**

$$
\hat{\mathbf{y}} = \mathbf{h}_{\text{final}} \cdot \mathbf{W}_{\text{out}} + \mathbf{b}_{\text{out}} \qquad [S \times d_{\text{out}}]
$$

### 18.3 Backward Pass (Manual Gradient Kernels)

The backward pass (`llmBackward`) is implemented entirely in CUDA with manual gradient computation — no automatic differentiation engine. All GEMM backward operations use `cublasDgemm` with transposed operands. Element-wise backward operations use custom CUDA kernels.

**Loss:** MSE on last sequence position:

$$
\mathcal{L} = \frac{1}{d_{\text{out}}} \sum_{j=1}^{d_{\text{out}}} (\hat{y}_{S,j} - y_j^*)^2
$$

$$
\frac{\partial \mathcal{L}}{\partial \hat{y}_{S,j}} = \frac{2(\hat{y}_{S,j} - y_j^*)}{d_{\text{out}}}
$$

**Gradient flow (reverse order):**

```
dLoss
  │
  ├── Output head backward:
  │     dW_out += h_final^T · dFinalOut     (llmGemmAT)
  │     db_out += sum(dFinalOut)             (llmBiasGradKernel)
  │     dCurInput = dFinalOut · W_out^T      (llmGemmBT)
  │
  ├── Per-layer (L-1 → 0):
  │     │
  │     ├── MoE Combine backward:
  │     │     dExpertOut[t,k] = w[t,k] · dMoeOut[t]   (copy + llmScaleKernel)
  │     │
  │     ├── Expert FFN backward (per expert e):
  │     │     dMid = dExpertOut · W2_e^T               (llmGemmBT)
  │     │     dW2_e += mid^T · dExpertOut              (llmGemmAT)
  │     │     db2_e += sum(dExpertOut)                  (llmBiasGradKernel)
  │     │     dMid *= GELU'(mid)                       (llmGeluBwdKernel)
  │     │     dW1_e += in^T · dMid                     (llmGemmAT)
  │     │     db1_e += sum(dMid)                        (llmBiasGradKernel)
  │     │
  │     ├── Residual: dPostAttn += dCurInput
  │     │
  │     ├── LayerNorm 2 backward:                      (llmLayerNormBwdKernel)
  │     │     dLn2In, dGamma2, dBeta2
  │     │
  │     ├── Attention output projection backward:
  │     │     dW_o += attn_out^T · dAttnProj           (llmGemmAT)
  │     │     db_o += sum(dAttnProj)                    (llmBiasGradKernel)
  │     │     dAttnOut = dAttnProj · W_o^T             (llmGemmBT)
  │     │
  │     ├── LayerNorm 1 backward:                      (llmLayerNormBwdKernel)
  │     │     dInput, dGamma1, dBeta1
  │     │
  │     └── Residual: dInput += dLn2In
  │
  └── Embedding backward:
        dW_e += X^T · dCurInput                        (llmGemmAT)
        db_e += sum(dCurInput)                          (llmBiasGradKernel)
```

### 18.4 Adam Optimiser

All parameter updates use bias-corrected Adam on GPU (`llmAdamKernel`):

$$
m_i \leftarrow \beta_1 m_i + (1 - \beta_1) g_i
$$

$$
v_i \leftarrow \beta_2 v_i + (1 - \beta_2) g_i^2
$$

$$
\hat{m}_i = \frac{m_i}{1 - \beta_1^t}, \qquad \hat{v}_i = \frac{v_i}{1 - \beta_2^t}
$$

$$
\theta_i \leftarrow \theta_i - \eta \cdot \frac{\hat{m}_i}{\sqrt{\hat{v}_i} + \varepsilon}
$$

with $\beta_1 = 0.9$, $\beta_2 = 0.999$, $\varepsilon = 10^{-8}$.

### 18.5 VRAM Guardrails

Before any `cudaMalloc`, the system pre-calculates the total VRAM required (`llmEstimateVram`) and checks against free VRAM via `cudaMemGetInfo`:

$$
\text{VRAM}_{\text{total}} = 4 \cdot |\theta| \cdot 8 + \text{cache}_{\text{per-layer}} \cdot L + \text{workspace}
$$

The factor of 4 covers: parameters + gradients + Adam first moment + Adam second moment. A 64 MB safety buffer is added. If $\text{VRAM}_{\text{required}} + \text{safety} > \text{VRAM}_{\text{free}}$, initialisation aborts with a diagnostic error message before any allocation occurs.

### 18.6 Backward Pass — Known Limitations

The current backward pass has two documented approximations:

1. **Attention backward is approximate.** The gradient through $\text{softmax}(\mathbf{Q}\mathbf{K}^\top / \sqrt{d})\mathbf{V}$ skips the full $d\mathbf{Q}$, $d\mathbf{K}$, $d\mathbf{V}$ computation through the softmax Jacobian. Instead, the gradient from $\mathbf{W}_o$ is passed directly to the LayerNorm 1 backward. This means $\mathbf{W}_{qkv}$ and $\mathbf{b}_{qkv}$ receive attenuated gradients. The LSTM router backward kernel (`llmLstmGateBwdKernel`) exists but is not yet wired into the main `llmBackward()` loop — the BPTT unrolling over sequence positions is pending.

2. **GELU backward uses post-activation values.** The expert FFN caches $\text{GELU}(\mathbf{m})$ (post-activation) rather than $\mathbf{m}$ (pre-activation). The backward pass evaluates $\text{GELU}'$ at the post-activation value, which introduces a small approximation error. This is acceptable for early training but will be corrected by caching pre-GELU activations.

### 18.7 Weight Persistence

Model weights are serialised as a binary file:

```
[LlmConfig struct][Adam step counter (int)][flat double array of all parameters]
```

The flat parameter array follows the same layout as `llmAssignWeightPtrs`: embedding → per-layer (LN1, QKV, Wo, LN2, LSTM, Router, Experts) → output head. LSTM hidden state can be saved/loaded separately for online inference continuity.

### 18.8 Relationship to the Trading Framework

The GpuLLM model operates *alongside* the deterministic framework (§1–§16), not *within* it. The deterministic equations guarantee fee neutrality regardless of any model prediction. The LLM's role is advisory — it processes price sequences through the MoE-Transformer and produces a scalar forecast. The trading decisions (entries, exits, TP levels) remain governed by the sigmoid-based overhead equations.

The LSTM router provides temporal memory that carries across inference calls — each new price tick updates the hidden state without re-training. This is analogous to the chain compounding recurrence (§15.9) where each cycle carries forward capital state. The parallel between the two systems is structural:

| Trading Framework | GpuLLM |
|---|---|
| Chain cycle $c$ | Transformer layer $l$ |
| Capital $T_c$ | Hidden state $\mathbf{h}_l$ |
| Overhead gradient $dT_c/d\theta$ | Loss gradient $d\mathcal{L}/d\theta_l$ |
| BPTT through chain recurrence | BPTT through LSTM time steps |
| Adam on $\theta_{\text{trading}}$ | Adam on $\theta_{\text{model}}$ |

---

## 19. Probabilistic Antithesis — Why Breaking Determinism Fails

This section formally derives the failure modes of a hypothetical regime that intentionally sacrifices the deterministic guarantee (§1.2) for higher trade throughput. It serves as a rigorous benchmark for *why* the overhead-neutral framework exists.

### 19.1 The Regime

Consider a strategy ("the Chef's model") that:

1. **Ignores fee hedging.** TP targets are set at a fixed percentage above entry without embedding overhead. Fees are treated as a post-hoc drag.
2. **Uses leveraged entries.** Capital is borrowed at interest rate $r_{\text{int}}$ to amplify position size.
3. **Targets high throughput.** Entries trigger aggressively on small price dips, with tight TPs and no buffer.

The operating assumption: *if enough TPs hit, the aggregate profit exceeds the aggregate fees and interest costs.* This is a probabilistic bet, not a deterministic guarantee.

### 19.2 The Insolvency Boundary

Define:

- $P_e$ — entry price
- $\phi$ — fractional TP distance above entry (e.g., $\phi = 0.02$ for 2%)
- $\sigma$ — annualised market volatility
- $\mathcal{F}$ — round-trip fee fraction (buy + sell)
- $r_{\text{int}}$ — interest rate on borrowed capital (per holding period)
- $q$ — position quantity
- $T_{\text{hold}}$ — expected holding time (periods)

**Profit per successful TP hit:**

$$
\Pi_{\text{hit}} = P_e \cdot q \cdot (\phi - \mathcal{F})
$$

**Cost per holding period (interest):**

$$
C_{\text{int}} = P_e \cdot q \cdot r_{\text{int}} \cdot T_{\text{hold}}
$$

**Net per trade:**

$$
\Pi_{\text{net}} = P_e \cdot q \cdot (\phi - \mathcal{F} - r_{\text{int}} \cdot T_{\text{hold}})
$$

**Insolvency condition:** $\Pi_{\text{net}} \leq 0$, i.e.:

$$
\boxed{
\phi \leq \mathcal{F} + r_{\text{int}} \cdot T_{\text{hold}}
}
$$

When the TP distance is less than or equal to fees plus interest, every trade loses money. This is the **insolvency boundary**: a hard wall in $(\phi, r_{\text{int}}, T_{\text{hold}})$ space below which the strategy is provably unprofitable regardless of hit rate.

### 19.3 Probability of Insolvency

Assume price follows geometric Brownian motion: $dP = \mu P \, dt + \sigma P \, dW$. The probability that the TP at $P_e(1 + \phi)$ is hit before a drawdown of $\psi$ (margin call threshold):

$$
P(\text{TP hit first}) = \frac{1 - e^{-2\mu\psi/\sigma^2}}{e^{2\mu\phi/\sigma^2} - e^{-2\mu\psi/\sigma^2}}
$$

For $\mu \approx 0$ (no drift), this simplifies to:

$$
P(\text{TP hit first}) \approx \frac{\psi}{\phi + \psi}
$$

The expected holding time conditional on TP hit:

$$
\mathbb{E}[T_{\text{hold}} \mid \text{TP hit}] \approx \frac{\phi \cdot \psi}{\sigma^2}
$$

**Probability of insolvency over $N$ trades:**

$$
P(\text{insolvency after } N) = 1 - \left(\frac{\psi}{\phi + \psi}\right)^N \cdot \left(1 - \frac{\mathcal{F} + r_{\text{int}} \cdot \phi\psi/\sigma^2}{\phi}\right)^N
$$

As $N \to \infty$, insolvency is almost certain unless $\phi \gg \mathcal{F} + r_{\text{int}} \cdot \phi\psi/\sigma^2$. The volatility $\sigma$ controls how fast the boundary is reached.

### 19.4 Expected Surplus Comparison

**Deterministic surplus** (our framework, §3.3):

$$
s_{\text{det}} = s \cdot f_h \cdot \Delta t \quad \text{(per trade, guaranteed if TP is hit)}
$$

This is a *conditional guarantee*: if TP is hit, the surplus is exactly $s_{\text{det}}$, with probability 1 conditional on the event.

**Expected surplus** (Chef's model):

$$
s_{\text{exp}} = P(\text{TP hit}) \cdot (\phi - \mathcal{F} - r_{\text{int}} \cdot \mathbb{E}[T_{\text{hold}}]) - P(\text{SL hit}) \cdot \psi
$$

Substituting the GBM probabilities:

$$
s_{\text{exp}} = \frac{\psi}{\phi + \psi} \cdot \left(\phi - \mathcal{F} - \frac{r_{\text{int}} \cdot \phi\psi}{\sigma^2}\right) - \frac{\phi}{\phi + \psi} \cdot \psi
$$

$$
= \frac{1}{\phi + \psi}\left[\psi\phi - \psi\mathcal{F} - \frac{r_{\text{int}} \cdot \phi\psi^2}{\sigma^2} - \phi\psi\right]
$$

$$
\boxed{
s_{\text{exp}} = \frac{-\psi}{\phi + \psi}\left(\mathcal{F} + \frac{r_{\text{int}} \cdot \phi\psi}{\sigma^2}\right)
}
$$

**This is always negative.** Under zero-drift GBM with fees and interest, the expected surplus of the leveraged probabilistic strategy is *unconditionally* negative. The fee and interest terms cannot be recovered probabilistically — they are a deterministic drag that the strategy has no mechanism to hedge.

Contrast with the deterministic framework where $s_{\text{det}} \geq 0$ always (guaranteed by construction whenever TP is hit).

### 19.5 The Zero-Sum Trap

**Theorem.** *The Chef's strategy becomes a zero-sum trap when interest costs exceed TP revenue — specifically when:*

$$
\boxed{
r_{\text{int}} \geq \frac{(\phi - \mathcal{F}) \cdot \sigma^2}{\phi \cdot \psi}
}
$$

**Proof.** Revenue per cycle at TP hit: $R = P_e q (\phi - \mathcal{F})$. Interest cost per cycle: $I = P_e q \cdot r_{\text{int}} \cdot \mathbb{E}[T_{\text{hold}}] = P_e q \cdot r_{\text{int}} \cdot \phi\psi / \sigma^2$.

Setting $I \geq R$:

$$
r_{\text{int}} \cdot \frac{\phi\psi}{\sigma^2} \geq \phi - \mathcal{F}
$$

$$
r_{\text{int}} \geq \frac{(\phi - \mathcal{F}) \cdot \sigma^2}{\phi \cdot \psi}
$$

Above this threshold, even trades that hit their TP lose money to interest. The strategy has negative expected return on *every path* — TP hit or SL hit. $\square$

**Numerical example:** $\phi = 2\%$, $\mathcal{F} = 0.2\%$, $\sigma = 50\%$ annualised, $\psi = 5\%$:

$$
r_{\text{int}} \geq \frac{(0.02 - 0.002) \cdot 0.25}{0.02 \cdot 0.05} = \frac{0.0045}{0.001} = 4.5\%
$$

At 4.5% interest per holding period, every trade is a loss. On perpetual futures with typical 0.01%/8h funding rates (≈0.03%/day), $T_{\text{hold}} > 150$ days reaches this threshold — but leveraged strategies that get stuck in drawdowns routinely exceed this.

### 19.6 Why Our Framework Is Immune

The deterministic framework (§1–§16) avoids every failure mode of the probabilistic regime:

| Property | Deterministic (Ours) | Probabilistic (Chef's) |
|----------|---------------------|----------------------|
| Fee coverage | Embedded in TP *a priori* | Post-hoc drag |
| Interest exposure | None (no leverage required) | Proportional to holding time |
| Surplus sign | $s_{\text{det}} \geq 0$ (guaranteed) | $s_{\text{exp}} < 0$ (always, under GBM) |
| Insolvency boundary | Does not exist (no leverage) | $\phi \leq \mathcal{F} + r_{\text{int}} T_{\text{hold}}$ |
| Downtrend buffer | Funds fractional re-entry (§7.2.1) | None — drawdown = loss |
| TP hit → profit | **Unconditionally true** | True only if $r_{\text{int}} < (\phi - \mathcal{F})\sigma^2/(\phi\psi)$ |
| $N \to \infty$ | Wealth grows (§9.3, chain compounding) | Insolvency approaches certainty |

The framework does not predict whether TPs will be hit. But it guarantees that *when they are*, every fee is covered, the surplus is captured, and the next cycle is funded. The Chef's model makes the opposite trade-off: it predicts that TPs will hit *often enough* to overcome fees and interest — and as §19.4 proves, this prediction fails under zero-drift conditions.

### 19.7 The Fundamental Asymmetry

The deterministic framework exploits a structural asymmetry:

$$
P(\text{surplus} \mid \text{TP hit}) = 1 \quad \text{vs.} \quad \mathbb{E}[\text{surplus}] = P(\text{TP hit}) \cdot s_{\text{det}}
$$

The surplus is *conditional* on TP being hit, but within that condition it is certain. The expected surplus across all paths is $P(\text{TP hit}) \cdot s_{\text{det}} > 0$ — always positive because $s_{\text{det}} > 0$ and $P(\text{TP hit}) > 0$ for any non-degenerate market.

The Chef's model has no such asymmetry to exploit. Its surplus is probabilistic in both the occurrence *and* the magnitude, with a negative-definite expectation under realistic fee and interest assumptions. The attempt to increase throughput by removing the deterministic constraint removes the only source of guaranteed positive expectation.
