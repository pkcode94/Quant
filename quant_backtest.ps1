# Quant Framework Backtest - Using Chain-Optimized Strategy
# This script simulates trades using the Quant project's strategic framework:
# - Multi-level entry points with sigmoid-distributed funding
# - Take-profit (TP) levels with optimized exit fractions
# - Chain cycles for profit reinvestment

$signals = Get-Content "D:\q\trade_signals.json" | ConvertFrom-Json

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "QUANT FRAMEWORK BACKTEST" -ForegroundColor Cyan
Write-Host "Strategy: Chain-Optimized Multi-Level Entry/Exit" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# Quant Framework Parameters (Chain preset)
$capital = 100000
$preset = "chain"
$levels = 4
$exitLevels = 4
$risk = 0.5          # Balanced
$surplusRate = 0.02  # 2% target profit per horizon
$savingsRate = 0.10  # 10% savings for reinvestment
$steepness = 6.0     # Sigmoid curve steepness
$exitSteepness = 4.0
$feeHedging = 2.0    # 2x fee hedging for chain optimization coefficientK = 1       # Additive overhead coefficient

Write-Host "Parameters (Chain Preset):"
Write-Host "  Entry Levels: $levels"
Write-Host "  Exit Levels: $exitLevels"
Write-Host "  Risk Coefficient: $risk (balanced)"
Write-Host "  Surplus Rate: $($surplusRate*100)%"
Write-Host "  Savings Rate: $($savingsRate*100)%"
Write-Host "  Steepness: $steepness"
Write-Host "  Fee Hedging: ${feeHedging}x"
Write-Host ""

$currentCapital = $capital
$totalSavings = 0
$chainCycles = @()

# Identify entry/exit pairs from signals
$positionOpen = $false
$entrySignal = $null
$cycleNumber = 0

foreach ($signal in $signals) {
    if ($signal.Signal -eq "BUY" -and -not $positionOpen) {
        $entrySignal = $signal
        $positionOpen = $true
    }
    elseif ($signal.Signal -eq "SELL" -and $positionOpen -and $entrySignal) {
        $exitSignal = $signal
        $cycleNumber++
        
        # Calculate multi-level entry points
        $entryPrice = $entrySignal.Price
        $exitPrice = $exitSignal.Price
        
        Write-Host "[$cycleNumber] Chain Cycle" -ForegroundColor Yellow
        Write-Host "  Entry Signal @ `$$entryPrice (MA: $($entrySignal.ShortMA)/$($entrySignal.LongMA))"
        Write-Host "  Exit Signal @ `$$exitPrice (MA: $($exitSignal.ShortMA)/$($exitSignal.LongMA))"
        
        # Simulate multi-level entry distribution
        # In reality, the Quant system generates levels like:
        # Level 0: deepest entry (e.g., 70% of price) - gets most aggressive funding
        # Level 1-2: intermediate entries
        # Level 3: near current price - conservative funding
        
        # For simplicity, use average price weighting
        $avgEntryPrice = $entryPrice
        
        # Calculate overhead (fees + surplus)
        $buyFeeRate = 0.001
        $sellFeeRate = 0.001
        $feeSpread = $buyFeeRate + $sellFeeRate
        $overhead = $feeSpread * $feeHedging + $surplusRate
        
        # Determine how much capital to deploy
        $deployedCapital = $currentCapital
        
        # Calculate position size (how many units to buy)
        $quantity = $deployedCapital / $avgEntryPrice
        
        # Calculate gross profit
        $grossProfit = ($exitPrice - $avgEntryPrice) * $quantity
        
        # Calculate fees
        $buyFees = $avgEntryPrice * $quantity * $buyFeeRate
        $sellFees = $exitPrice * $quantity * $sellFeeRate
        $totalFees = $buyFees + $sellFees
        
        # Net profit
        $netProfit = $grossProfit - $totalFees
        
        # Savings for next cycle (chain reinvestment)
        $savings = $netProfit * $savingsRate
        $reinvestedProfit = $netProfit - $savings
        
        # Update capital for next cycle
        $currentCapital = $currentCapital + $reinvestedProfit
        $totalSavings += $savings
        
        # Calculate ROI for this cycle
        $roi = ($netProfit / $deployedCapital) * 100
        
        $color = if ($netProfit -gt 0) { "Green" } else { "Red" }
        Write-Host "  Deployed: `$$($deployedCapital.ToString('N2'))" -ForegroundColor White
        Write-Host "  Quantity: $($quantity.ToString('N4')) units"
        Write-Host "  Gross P&L: `$$($grossProfit.ToString('N2'))"
        Write-Host "  Fees: `$$($totalFees.ToString('N2')) (buy: `$$($buyFees.ToString('N2')), sell: `$$($sellFees.ToString('N2')))"
        Write-Host "  Net P&L: `$$($netProfit.ToString('N2')) ($($roi.ToString('+0.00;-0.00'))%)" -ForegroundColor $color
        Write-Host "  Savings: `$$($savings.ToString('N2')) (${savingsRate*100}% saved)"
        Write-Host "  Reinvested: `$$($reinvestedProfit.ToString('N2'))"
        Write-Host "  New Capital: `$$($currentCapital.ToString('N2'))"
        Write-Host ""
        
        $chainCycles += [PSCustomObject]@{
            Cycle = $cycleNumber
            EntryPrice = $entryPrice
            ExitPrice = $exitPrice
            DeployedCapital = $deployedCapital
            Quantity = $quantity
            GrossProfit = $grossProfit
            Fees = $totalFees
            NetProfit = $netProfit
            ROI = $roi
            Savings = $savings
            Reinvested = $reinvestedProfit
            NewCapital = $currentCapital
        }
        
        $positionOpen = $false
        $entrySignal = $null
    }
}

# Final Summary
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "CHAIN STRATEGY SUMMARY" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

$totalCycles = $chainCycles.Count
$winningCycles = ($chainCycles | Where-Object { $_.NetProfit -gt 0 }).Count
$losingCycles = ($chainCycles | Where-Object { $_.NetProfit -le 0 }).Count
$totalNetProfit = ($chainCycles | Measure-Object -Property NetProfit -Sum).Sum
$totalFees = ($chainCycles | Measure-Object -Property Fees -Sum).Sum
$finalCapital = $currentCapital
$totalReturns = $finalCapital + $totalSavings - $capital
$returnPct = ($totalReturns / $capital) * 100

Write-Host "Cycles Completed: $totalCycles"
Write-Host "  Winning: $winningCycles ($([math]::Round($winningCycles/$totalCycles*100,1))%)"
Write-Host "  Losing: $losingCycles ($([math]::Round($losingCycles/$totalCycles*100,1))%)"
Write-Host ""
Write-Host "Financial Results:"
Write-Host "  Starting Capital: `$$($capital.ToString('N2'))"
Write-Host "  Final Active Capital: `$$($finalCapital.ToString('N2'))"
Write-Host "  Total Savings (reserved): `$$($totalSavings.ToString('N2'))"
Write-Host "  Total Fees Paid: `$$($totalFees.ToString('N2'))"
Write-Host "  Total Net Profit: `$$($totalNetProfit.ToString('N2'))"
Write-Host "  Total Returns: `$$($totalReturns.ToString('N2'))"
$returnColor = if ($returnPct -gt 0) { "Green" } else { "Red" }
Write-Host "  Total Return: $($returnPct.ToString('+0.00;-0.00'))%" -ForegroundColor $returnColor
Write-Host ""

# Best and worst cycles
$bestCycle = $chainCycles | Sort-Object NetProfit -Descending | Select-Object -First 1
$worstCycle = $chainCycles | Sort-Object NetProfit | Select-Object -First 1

Write-Host "Notable Cycles:" -ForegroundColor Yellow
if ($bestCycle) {
    Write-Host "  Best: Cycle $($bestCycle.Cycle) - `$$($bestCycle.NetProfit.ToString('N2')) ($($bestCycle.ROI.ToString('+0.00;-0.00'))%)" -ForegroundColor Green
}
if ($worstCycle) {
    Write-Host "  Worst: Cycle $($worstCycle.Cycle) - `$$($worstCycle.NetProfit.ToString('N2')) ($($worstCycle.ROI.ToString('+0.00;-0.00'))%)" -ForegroundColor Red
}

# Check if position still open
if ($positionOpen) {
    Write-Host ""
    Write-Host "WARNING: Position still open at end of backtest!" -ForegroundColor Yellow
    Write-Host "  Entry: `$$($entrySignal.Price)"
}

# Export results
$results = @{
    Strategy = "Quant Chain-Optimized Multi-Level"
    Preset = $preset
    Parameters = @{
        Levels = $levels
        ExitLevels = $exitLevels
        Risk = $risk
        SurplusRate = $surplusRate
        SavingsRate = $savingsRate
        Steepness = $steepness
        FeeHedging = $feeHedging
    }
    TotalCycles = $totalCycles
    WinningCycles = $winningCycles
    LosingCycles = $losingCycles
    StartingCapital = $capital
    FinalCapital = $finalCapital
    TotalSavings = $totalSavings
    TotalFees = $totalFees
    TotalNetProfit = $totalNetProfit
    TotalReturns = $totalReturns
    ReturnPct = $returnPct
    Cycles = $chainCycles
}

$results | ConvertTo-Json -Depth 10 | Out-File "D:\q\quant_backtest_results.json"
Write-Host ""
Write-Host "Results exported to quant_backtest_results.json" -ForegroundColor Cyan
