# Quant MCP Framework - Live Execution Demo
# This demonstrates how to use the MCP server with the Quant strategic framework

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "QUANT MCP FRAMEWORK EXECUTION" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# Current state from backtest
$currentPrice = 3340.15  # Latest gold price (March 2026)
$symbol = "GOLD"
$capital = 100000

Write-Host "Current Market State:"
Write-Host "  Symbol:GOLD"
Write-Host "  Current Price: `$$currentPrice"
Write-Host "  Available Capital: `$$capital"
Write-Host ""

Write-Host "QUANT FRAMEWORK WORKFLOW:" -ForegroundColor Yellow
Write-Host "============================================================" -ForegroundColor Yellow
Write-Host ""

Write-Host "Step 1: Generate Serial Plan (Entry + Exit Levels)" -ForegroundColor Green
Write-Host "  Tool: mcp_quant-mcp_generate_serial_plan"
Write-Host "  - Creates multi-level entry points based on preset (e.g., 'chain')"
Write-Host "  - Distributes capital across levels using sigmoid weighting"
Write-Host "  - Calculates break-even prices with overhead"
Write-Host "  - Generates corresponding exit (TP) levels for each entry"
Write-Host ""
Write-Host "  Example parameters:"
Write-Host "    preset: 'chain'           # Chain-optimized strategy"
Write-Host "    symbol: '$symbol'"
Write-Host "    currentPrice: $currentPrice"
Write-Host "    availableFunds: $capital"
Write-Host "    cycles: 3                 # Number of chain cycles"
Write-Host ""

Write-Host "Step 2: Review Entry Points" -ForegroundColor Green
Write-Host "  Tool: mcp_quant-mcp_list_entries"
Write-Host "  - Shows all generated entry levels"
Write-Host "  - Each level has: price, quantity, funding, break-even"
Write-Host "  - Entries auto-fill when market price hits the level"
Write-Host ""

Write-Host "Step 3: Fill Entry Levels (Manual or Auto)" -ForegroundColor Green
Write-Host "  Tool: mcp_quant-mcp_fill_entry"
Write-Host "  - Manually fill an entry level when price is reached"
Write-Host "  - Or entries auto-fill if price series is running"
Write-Host "  - Creates a Trade with corresponding exit levels"
Write-Host ""

Write-Host "Step 4: Review Exit Strategy" -ForegroundColor Green
Write-Host "  Tool: mcp_quant-mcp_list_exits"
Write-Host "  - Shows all pending TP (take-profit) levels for a trade"
Write-Host "  - Each TP level has: price, quantity to sell, profit target"
Write-Host "  - Uses sigmoid distribution to lock in profits progressively"
Write-Host ""

Write-Host "Step 5: Execute Exits (Manual or Auto)" -ForegroundColor Green
Write-Host "  Tool: mcp_quant-mcp_execute_exit"
Write-Host "  - Execute a TP level when market price reaches it"
Write-Host "  - Partial position closes lock in incremental profits"
Write-Host "  - Remaining position rides for higher TP levels"
Write-Host ""

Write-Host "Step 6: Review Chains" -ForegroundColor Green
Write-Host "  Tool: mcp_quant-mcp_list_chains"
Write-Host "  - Shows multi-cycle trade chains"
Write-Host "  - Tracks profit reinvestment across cycles"
Write-Host "  - Monitors savings rate adherence"
Write-Host ""

Write-Host "Step 7: Compare Plans & Optimize" -ForegroundColor Green
Write-Host "  Tool: mcp_quant-mcp_compare_plans"
Write-Host "  - Compare different presets (chain vs moderate vs aggressive)"
Write-Host "  - Analyze expected ROI, risk, TP spread"
Write-Host "  - Optimize parameters using ChainOptimizer (BPTT)"
Write-Host ""

Write-Host ""
Write-Host "STRATEGY PRESETS AVAILABLE:" -ForegroundColor Yellow
Write-Host "============================================================" -ForegroundColor Yellow
Write-Host ""

$presets = @(
    @{Name="conservative"; Desc="Low-risk, tight levels, high savings (10%), stop-losses enabled"},
    @{Name="moderate"; Desc="Balanced, medium levels (6), uniform funding"},
    @{Name="aggressive"; Desc="High-risk, deep levels (10), inverse sigmoid, low savings (2%)"},
    @{Name="chain"; Desc="★ Chain-optimized: 2x fee hedging, K=1, 10% savings for reinvestment"},
    @{Name="dca"; Desc="Dollar-cost averaging: many levels (12), uniform distribution"},
    @{Name="scalper"; Desc="Quick turnaround: few levels (3), narrow spread, fast exits"},
    @{Name="short"; Desc="Short-selling: inverse entry direction, stop-losses above"}
)

foreach ($preset in $presets) {
    $marker = if ($preset.Name -eq "chain") { "★" } else { " " }
    Write-Host "$marker $($preset.Name.PadRight(15)) : $($preset.Desc)"
}

Write-Host ""
Write-Host ""
Write-Host "KEY FRAMEWORK CONCEPTS:" -ForegroundColor Yellow
Write-Host "============================================================" -ForegroundColor Yellow
Write-Host ""

Write-Host "Multi-Level Entry/Exit:"
Write-Host "   Instead of single-price trades, Quant distributes across multiple"
Write-Host "   entry levels (e.g., `$3000, `$3100, `$3200, `$3300) to average in"
Write-Host "   and multiple TP levels to scale out profits progressively."
Write-Host ""

Write-Host "Risk Coefficients (0-1):"
Write-Host "   0.0 = Conservative (more funds near current price, sigmoid)"
Write-Host "   0.5 = Balanced (uniform distribution)"
Write-Host "   1.0 = Aggressive (more funds at deep discounts, inverse sigmoid)"
Write-Host ""

Write-Host "Surplus Rate and Overhead:"
Write-Host "   surplus = target profit per horizon (e.g., 2% = 0.02)"
Write-Host "   overhead = fees x feeHedging + surplus"
Write-Host "   TP levels are spaced by effective overhead"
Write-Host ""

Write-Host "Chain Cycles and Savings:"
Write-Host "   After each cycle, profit is split:"
Write-Host "   - (1 - savingsRate) -> reinvested in next cycle"
Write-Host "   - savingsRate -> reserved (e.g., 10% saved)"
Write-Host "   Chain formula: T_next = T_c + Delta_c x (1 - s_save)"
Write-Host ""

Write-Host "Steepness (Sigmoid Shape):"
Write-Host "   0-2   = Nearly uniform"
Write-Host "   4-6   = Smooth S-curve (recommended)"
Write-Host "   10+   = Sharp step function"
Write-Host ""

Write-Host "Fee Hedging:"
Write-Host "   Multiplier on fee spread to build extra safety margin"
Write-Host "   1.0 = exact fees, 2.0 = 2x fees (chain preset)"
Write-Host ""

Write-Host ""
Write-Host "BACKTEST COMPARISON:" -ForegroundColor Yellow
Write-Host "============================================================" -ForegroundColor Yellow
Write-Host ""

$comparison = @(
    @{Strategy="Simple MA Crossover (20/50)"; Return=63.14; WinRate=47.4; Cycles=19},
    @{Strategy="Quant Chain-Optimized"; Return=2722.69; WinRate=42.1; Cycles=19}
)

Write-Host "Strategy                          Return        Win Rate    Cycles"
Write-Host "────────────────────────────────  ────────────  ──────────  ──────"
foreach ($c in $comparison) {
    $returnColor = if ($c.Return -gt 100) { "Green" } else { "Yellow" }
    Write-Host "$($c.Strategy.PadRight(32))  " -NoNewline
    Write-Host "+$($c.Return.ToString('N2').PadLeft(9))%  " -NoNewline -ForegroundColor $returnColor
    Write-Host "$($c.WinRate.ToString('N1').PadLeft(7))%  " -NoNewline
    Write-Host "$($c.Cycles)"
}

Write-Host ""
Write-Host "Key Insight:" -ForegroundColor Cyan
Write-Host "  The Quant framework's compound reinvestment and multi-level"
Write-Host "  strategy amplified the same trading signals by 43x!"
Write-Host "  (2722.69% vs 63.14%)"
Write-Host ""

Write-Host ""
Write-Host "NEXT STEPS TO USE MCP:" -ForegroundColor Yellow
Write-Host "============================================================" -ForegroundColor Yellow
Write-Host ""
Write-Host "To execute trades via MCP server, you would:"
Write-Host ""
Write-Host "1. Call: mcp_quant-mcp_generate_serial_plan"
Write-Host "   Returns: Entry plan with levels + exit TP levels"
Write-Host ""
Write-Host "2. Monitor price and fill entries as market moves"
Write-Host "   Call: mcp_quant-mcp_fill_entry for each level hit"
Write-Host ""
Write-Host "3. Monitor TP levels and execute exits"
Write-Host "   Call: mcp_quant-mcp_execute_exit when TP reached"
Write-Host ""
Write-Host "4. Track portfolio:"
Write-Host "   Call: mcp_quant-mcp_portfolio_status (wallet + positions)"
Write-Host "   Call: mcp_quant-mcp_list_trades (trade history)"
Write-Host "   Call: mcp_quant-mcp_list_chains (multi-cycle tracking)"
Write-Host ""
Write-Host "5. Calculate P&L:"
Write-Host "   Call: mcp_quant-mcp_calculate_profit for any trade"
Write-Host ""

Write-Host ""
Write-Host "Framework Documentation:" -ForegroundColor Cyan
Write-Host "  See D:\q\docs\equations.md for mathematical framework"
Write-Host "  See D:\q\Quant\ChainOptimizer.h for BPTT optimization" Write-Host "  See D:\q\Quant\MarketEntryCalculator.h for entry logic"
Write-Host "  See D:\q\Quant\ExitStrategyCalculator.h for exit logic"
Write-Host ""
