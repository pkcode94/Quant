# Quant MCP Server — AI Instruction Manual

This document provides a complete reference for AI agents interacting with the Quant trading system via the Model Context Protocol (MCP).

## System Overview

The Quant MCP Server provides programmatic access to a quantitative trading engine that uses mathematical models to:
- Generate multi-level entry strategies with sigmoid-distributed pricing **or adaptive historical price ranges**
- Calculate optimal take-profit (TP) and stop-loss (SL) levels
- Execute trades with proper accounting and parent-child linkage
- Simulate trading strategies against historical data
- Manage portfolios with fee overhead hedging

**Default Configuration:**
- **Stop losses are DEACTIVATED by default** unless explicitly enabled by the user
- **Adaptive Range is ENABLED by default** - entries span from current price to max historical price
- All prices are per-unit (not total value)
- Fees are automatically hedged into TP calculations
- FIFO (First-In-First-Out) lot allocation for sell executions

---

## Mathematical Foundation

### §1 — Core Variables

| Symbol | Description | Domain |
|--------|-------------|--------|
| $P$ | Current market price per unit | $P > 0$ |
| $P_e$ | Entry price per unit | $P_e > 0$ |
| $q$ | Quantity (units held) | $q > 0$ |
| $N$ | Number of levels (horizons / entries / exits) | $N \geq 1$ |
| $i$ | Level index | $0 \leq i \leq N-1$ |
| $f_s$ | Fee spread / slippage rate | $f_s \geq 0$ |
| $f_h$ | Fee hedging coefficient (safety multiplier) | $f_h \geq 1$ |
| $\Delta t$ | Delta time | $\Delta t > 0$ |
| $n_s$ | Symbol count in portfolio | $n_s \geq 1$ |
| $T$ | Portfolio pump (injected capital for period $t$) | $T \geq 0$ |
| $K$ | Coefficient K (additive denominator offset) | $K \geq 0$ |
| $s$ | Surplus rate (profit margin above break-even) | $s \geq 0$ |
| $r$ | Risk coefficient | $r \in [0, 1]$ |
| $\alpha$ | Steepness (sigmoid sharpness) | $\alpha > 0$ |
| $R_{\max}$ | Max risk (TP ceiling fraction) | $R_{\max} \geq 0$ |
| $R_{\min}$ | Min risk (TP floor fraction above break-even) | $R_{\min} \geq 0$ |
| $\phi$ | Exit fraction (portion of holdings to sell) | $\phi \in [0, 1]$ |
| $\phi_{\text{sl}}$ | SL fraction (portion to sell at SL) | $\phi_{\text{sl}} \in [0, 1]$ |

### §2 — Overhead & Fee Hedging

**Fee Component:**
$$\mathcal{F} = f_s \cdot f_h \cdot \Delta t$$

**Raw Overhead:**
$$\text{OH}(P, q) = \frac{\mathcal{F} \cdot n_s \cdot (1 + n_f)}{\dfrac{P}{q} \cdot T + K}$$

The overhead normalizes fee costs against the per-unit price ratio scaled by pump capital. The factor $(1 + n_f)$ allows this trade's TP to cover fees for $n_f$ future chain trades.

**Effective Overhead:**
$$\text{EO}(P, q) = \text{OH}(P, q) + s \cdot f_h \cdot \Delta t + f_s \cdot f_h \cdot \Delta t$$

**Position Delta:**
$$\delta = \frac{P \cdot q}{T}$$

### §3 — Sigmoid Distribution

All price/funding distributions use the logistic sigmoid:
$$\sigma(x) = \frac{1}{1 + e^{-x}}$$

**Normalized Sigmoid Mapping** (maps $t \in [0,1]$ to normalized $[0,1]$ curve):
$$\hat\sigma(t) = \frac{\sigma\!\bigl(\alpha \cdot (t - 0.5)\bigr) - \sigma_0}{\sigma_1 - \sigma_0}$$

where $\sigma_0 = \sigma(-\alpha/2)$ and $\sigma_1 = \sigma(+\alpha/2)$.

### §4 — Entry Price Generation

**Price Range Modes:**

**Adaptive Range (DEFAULT - enabled by default):**
- Range below: Always current price (entries start from market)
- Range above: Maximum historical entry price ever recorded
- Formula: `rangeAbove = max(historicalEntryPrice) - currentPrice`
- Effect: Entries naturally span from current price back up to previous highs
- Benefits: Always captures historical price levels, adapts to new market regimes
- Example: If historical high was $100 and current is $80, entries fill from $80-$100

**Manual Range Mode:**
- Set `adaptiveRange: false` to use manual rangeAbove/rangeBelow
- Range below: `priceLow = currentPrice - rangeBelow`
- Range above: `priceHigh = currentPrice + rangeAbove`
- Useful for: Testing specific price bands, defining hard limits

**Auto Range Mode (deprecated):**
- Set `autoRange: true` for heuristic range: `[price × (1 - 3×EO), price]`
- Computed from effective overhead but less flexible than adaptive

**Level Index Normalization:**
$$t_i = \begin{cases}
\dfrac{i}{N-1} & \text{if } N > 1 \\
1 & \text{if } N = 1
\end{cases}$$

**Entry Price** (sigmoid-distributed):
$$P_e^{(i)} = P_{\text{low}} + \hat\sigma(t_i) \cdot (P_{\text{high}} - P_{\text{low}})$$

**Break-Even Price:**
$$P_{\text{BE}}^{(i)} = P_e^{(i)} \cdot (1 + \text{OH})$$

**Risk-Warped Funding Allocation:**
$$w_i = (1 - r) \cdot \hat\sigma(t_i) + r \cdot (1 - \hat\sigma(t_i))$$

$$F_i = \frac{w_i}{\displaystyle\sum_{j=0}^{N-1} w_j}$$

Where $r$ controls distribution:
- $r = 0$: Conservative (more funds at higher/safer prices)
- $r = 0.5$: Uniform distribution
- $r = 1$: Aggressive (more funds at lower/discount prices)

**Units Purchased:**
$$q_i = \frac{T \cdot F_i}{P_e^{(i)}}$$

### §5 — Take-Profit & Stop-Loss Generation

**Standard Mode** ($R_{\max} = 0$):
$$\text{TP}_i = P_e \cdot q \cdot (1 + \text{EO} \cdot (i + 1))$$
$$\text{SL}_i = P_e \cdot q \cdot (1 - \text{EO} \cdot (i + 1))$$

**Max-Risk Mode** ($R_{\max} > 0$) - sigmoid interpolation between overhead and max risk:
$$\text{factor}_i = f_{\min} + \hat\sigma(t_i) \cdot (f_{\max} - f_{\min})$$
$$\text{TP}_i = P_e \cdot q \cdot (1 + \text{factor}_i)$$

**Per-Unit TP (for serial generation):**

LONG positions:
$$\text{TP}_i = \text{TP}_{\min} + \hat\sigma(t_i) \cdot (\text{TP}_{\max} - \text{TP}_{\min})$$
where $\text{TP}_{\min} = P_e \cdot (1 + \text{EO} + R_{\min})$

SHORT positions:
$$\text{TP}_i = \text{TP}_{\max} - \hat\sigma(t_i) \cdot (\text{TP}_{\max} - \text{TP}_{\text{floor}})$$
where $\text{TP}_{\max} = P_e \cdot (1 - \text{EO} - R_{\min})$

**Stop-Loss (per-unit):**
- LONG: $\text{SL} = P_e \cdot (1 - \text{EO})$
- SHORT: $\text{SL} = P_e \cdot (1 + \text{EO})$

**Fractional Stop-Loss:**
Only $\phi_{\text{sl}}$ fraction of position exits at SL:
$$q_{\text{sl}} = q \cdot \phi_{\text{sl}}$$
$$\text{Loss}_{\text{sl}} = -\text{EO} \cdot P_e \cdot q \cdot \phi_{\text{sl}}$$

**Capital-Loss Capping:**
Total SL loss across all levels is capped at available capital:
$$\phi_{\text{sl}}^{\text{clamped}} = \min\left(\phi_{\text{sl}}, \phi_{\text{sl}} \cdot \frac{T_{\text{avail}}}{\text{TotalSLLoss}}\right)$$

### §6 — Downtrend & SL Hedging Buffers

**Downtrend Buffer** (pre-funds $n_d$ future downturn cycles):
$$\text{buffer}_{\text{dt}} = 1 + n_d \cdot \text{pc}$$

where $\text{pc}$ is per-cycle cost using axis-dependent sigmoid ($\alpha_d = \max(\delta, 0.1)$):
$$\text{pc} = R_{\min} + \hat\sigma_{\alpha_d}(t) \cdot (\text{upper} - R_{\min})$$

**Stop-Loss Hedge Buffer** (pre-funds $n_{\text{sl}}$ future SL hits):
$$\text{buffer}_{\text{sl}} = 1 + n_{\text{sl}} \cdot \phi_{\text{sl}} \cdot \text{pc}$$

**Combined TP Adjustment:**
$$\text{TP}_{\text{adj}} = \text{TP}_{\text{base}} \cdot \text{buffer}_{\text{dt}} \cdot \text{buffer}_{\text{sl}}$$

### §7 — Exit Strategy Distribution

**Sigmoid Cumulative Sell Distribution:**
Centre position: $c = r \cdot (N - 1)$

$$C_i = \sigma(\alpha \cdot (i - 0.5 - c))$$
$$\hat{C}_i = \frac{C_i - C_0}{C_N - C_0}$$

**Sell Quantity per Level:**
$$q_i^{\text{sell}} = q_{\text{total}} \cdot \phi \cdot (\hat{C}_{i+1} - \hat{C}_i)$$

### §8 — Profit Calculation

**Unrealized Profit (open position):**
- LONG: $\text{Gross} = (P_{\text{now}} - P_e) \cdot q$
- SHORT: $\text{Gross} = (P_e - P_{\text{now}}) \cdot q$

$$\text{Net} = \text{Gross} - F_{\text{buy}} - F_{\text{sell}}$$
$$\text{ROI} = \frac{\text{Net}}{P_e \cdot q + F_{\text{buy}}} \times 100\%$$

**Realized Profit (closed trade):**
$$\text{Gross} = (P_{\text{sell}} - P_{\text{entry}}) \cdot q_{\text{sold}}$$
$$\text{ROI} = \frac{\text{Net}}{P_{\text{entry}} \cdot q_{\text{sold}} + F_{\text{buy}}} \times 100\%$$

---

## MCP Tool Usage Guide

### Portfolio Management

**`portfolio_status`** - Get current wallet and open positions
```json
{
  "wallet": {"liquid": 5000.0, "deployed": 100.0, "total": 5100.0},
  "openPositions": [
    {
      "tradeId": 1,
      "symbol": "BTC",
      "entryPrice": 50000.0,
      "quantity": 0.002,
      "sold": 0.001,
      "remaining": 0.001,
      "cost": 100.0
    }
  ]
}
```

**`list_trades`** - View all executed trades with parent-child linkage

**`wallet_deposit`** / **`wallet_withdraw`** - Manage capital

### Trade Execution

**`execute_buy`** → **`confirm_execution`**
```json
// Request buy
{"symbol": "BTC", "price": 50000, "quantity": 0.002}

// Confirm with token
{"token": "abc123..."}

// Result
{"success": true, "tradeId": 42, "action": "buy"}
```

**`execute_sell`** → **`confirm_execution`**
- Uses FIFO lot allocation automatically
- Links to parent buy trades with proper `parentTradeId`
- Updates `sold` and `remaining` fields correctly

### Strategy Generation

**`generate_serial_plan`** - Create multi-level entry strategy
```json
{
  "symbol": "BTC",
  "currentPrice": 50000,
  "capital": 1000,
  "levels": 5,
  "params": {
    "feeSpread": 0.001,
    "feeHedge": 1.5,
    "deltaTime": 1.0,
    "symbolCount": 1,
    "surplusRate": 0.02,
    "riskCoefficient": 0.5,
    "steepness": 2.0,
    "maxRisk": 0.1,
    "minRisk": 0.01,
    "generateStopLosses": false  // DEFAULT: stop losses disabled
  }
}
```

Returns entry prices, TP/SL levels, funding allocation, and expected profits per level.

**`fill_entry`** - Execute a specific entry level from a generated plan

### Analysis Tools

**`price_check`** - Monitor open positions against current market prices
- Shows TP/SL breach status
- Displays ROI and unrealized P&L
- Lists pending exit levels

**`calculate_profit`** - Compute unrealized profit for open position

**`compare_plans`** - Compare multiple strategy configurations

**`what_if`** - Simulate position outcomes at different price points

### Chain Management

**`list_chains`** - View active multi-level trading chains

**`list_entries`** / **`list_exits`** - Monitor pending entry/exit levels

### Presets

**`save_preset`** / **`load_preset`** / **`list_presets`** - Store and reuse parameter configurations

---

## AI Agent Guidelines

### 1. Default Strategy Parameters & Adaptive Range

Use these conservative defaults unless user specifies otherwise:
```json
{
  "feeSpread": 0.001,           // 0.1% fee
  "feeHedge": 1.5,              // 50% safety margin
  "deltaTime": 1.0,
  "symbolCount": 1,
  "surplusRate": 0.02,          // 2% profit margin
  "riskCoefficient": 0.5,       // Balanced distribution
  "steepness": 2.0,             // Moderate sigmoid curve
  "maxRisk": 0.1,               // 10% max TP
  "minRisk": 0.01,              // 1% min TP floor
  "generateStopLosses": false,  // CRITICAL: Stop losses OFF by default
  "adaptiveRange": true         // ENABLED: Spans from current to max historical price
}
```

**Adaptive Range Behavior:**
- **Enabled by default** (`adaptiveRange: true`)
- Automatically queries all historical trades to find max entry price
- Sets entries from current price up to historical high
- No manual configuration needed - adapts to market conditions
- Disable with `adaptiveRange: false` to use manual rangeAbove/rangeBelow

### 2. Stop-Loss Handling

**⚠️ IMPORTANT: Stop losses are DEACTIVATED by default**

Only enable stop losses if user explicitly requests:
- Set `generateStopLosses: true` in params
- Configure `stopLossFraction` (0-1, default 1.0 = full exit)
- Set `stopLossHedgeCount` for pre-funding future SL hits

### 3. Risk Management

**Conservative approach ($r = 0$):**
- More capital allocated to higher/safer entry prices
- Suitable for volatile markets

**Balanced approach ($r = 0.5$):**
- Uniform funding distribution
- Default recommendation

**Aggressive approach ($r = 1$):**
- More capital at deeper discounts
- Higher risk, higher potential reward

### 4. Multi-Level Strategy Design

**Small portfolios (< $1000):**
- Use 3-5 entry levels
- Lower level count reduces complexity

**Medium portfolios ($1000-$10000):**
- Use 5-10 entry levels
- Balanced granularity

**Large portfolios (> $10000):**
- Use 10-20 entry levels
- Fine-grained price distribution

### 5. Backtesting Workflow

1. Generate serial plan with `generate_serial_plan`
2. Review entry/TP/SL levels and funding allocation
3. Use simulator endpoint to backtest against historical data
4. Analyze fee hedging coverage and profit distribution
5. Adjust parameters and iterate

### 6. Chain Trading

Enable chain mode for automated cycle regeneration:
- After all exits complete, new entries generate at current price
- Capital compounds across cycles (minus optional savings rate)
- Creates superposition of entry levels across time

### 7. Price Monitoring

Regularly call `price_check` to:
- Detect TP breaches (trigger sells)
- Monitor SL status (if enabled)
- Track unrealized P&L
- Identify filled entry levels

### 8. Position Accounting

The system uses FIFO lot allocation:
- Sells automatically link to oldest buy trades first
- `parentTradeId` properly links child sells to parent buys
- `sold` and `remaining` fields update correctly
- Portfolio status reflects accurate deployment

---

## Common Workflows

### Workflow 1: Generate Entry Strategy with Adaptive Range (Recommended)

```
1. price_check(symbol="BTC") 
   → Get current market price: $67,000

2. generate_serial_plan(
     symbol="BTC", 
     currentPrice=67000, 
     capital=1000, 
     levels=5,
     adaptiveRange=true  // or omit (defaults to true)
   )
   → System queries all historical BTC trades
   → Finds max entry price: $72,000
   → Generates 5 entry levels from $67,000 to $72,000
   → Each level has proper TP/SL based on price

3. Review entries:
   - Level 0: $67,500 (near current, conservative)
   - Level 2: $69,500 (mid-range)
   - Level 4: $71,500 (near historical high, aggressive)

4. Save and monitor entry points
```

**How Adaptive Range Helps:**
- No manual calculation of price bands needed
- Automatically captures support/resistance from all past trades
- Adapts when new all-time highs achieved
- Intelligent spacing without guesswork

### Workflow 2: Manual Buy & Exit Levels

```
1. execute_buy(symbol="BTC", price=50000, quantity=0.01)
   → Get confirmation token

2. confirm_execution(token="...")
   → Trade executed, get tradeId

3. Generate exit levels for position:
   [Use internal horizon engine with price=50000, quantity=0.01]

4. Monitor price, execute sells at TP levels:
   execute_sell(symbol="BTC", price=51000, quantity=0.005)
```

### Workflow 3: Backtest Strategy

```
1. generate_serial_plan(...)
   → Get strategy configuration

2. Run simulator with historical price series
   → Analyze profit, drawdown, fee coverage

3. Adjust parameters (risk, surplus, levels)
   
4. Repeat until optimal configuration found

5. save_preset(name="btc_conservative", params=...)
   → Store for future use
```

---

## Error Handling

- **Insufficient capital**: Check `wallet.liquid` before execute_buy
- **Invalid symbol**: Verify symbol exists in registry
- **Negative prices**: All prices must be > 0
- **Invalid quantity**: Quantity must be > 0
- **Sell exceeds position**: Check `remaining` field before execute_sell
- **Token expired**: Confirmation tokens expire after short period

---

## Performance Optimization

1. **Batch operations**: Generate full serial plan instead of individual entries
2. **Preset reuse**: Save tested configurations as presets
3. **Price monitoring**: Use `price_check` efficiently (don't poll every second)
4. **Capital efficiency**: Use fee hedging to minimize overhead impact
5. **Chain trading**: Enable for automatic cycle compounding

---

## Mathematical Validation

The overhead formula ensures:
- Total TP gross profit ≥ Total fees paid (when coverage ≥ 1.0)
- Break-even prices account for all embedded costs
- Surplus rate provides explicit profit margin
- Fee hedging pre-funds future chain trades
- SL capital capping prevents over-leverage

All equations are validated in the simulator against real fee accounting.

---

## Version & Compatibility

- MCP Protocol Version: 2024-11-05
- Engine Version: Multi-Horizon with FIFO Accounting
- Database: JSON-based TradeDatabase with parent-child linkage
- Transport: HTTP POST to `/mcp` endpoint (adaptive TCP fallback)

---

## Support & References

- Full mathematical derivation: `docs/equations.md`
- Architecture documentation: `docs/sync-architecture.md`
- Failure mode analysis: `docs/failure-modes.md`
- Best practices: `docs/best-case-use.md`

---

**Last Updated:** March 8, 2026
**Author:** Quant Development Team
