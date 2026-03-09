# Load trading signals
$signals = Get-Content "D:\q\trade_signals.json" | ConvertFrom-Json

Write-Host "Loaded $($signals.Count) trading signals"
Write-Host ""

$positionOpen = $false
$positionQty = 0
$entryPrice = 0
$executedTrades = @()
$totalPnL = 0
$winningTrades = 0
$losingTrades = 0

foreach ($signal in $signals) {
    if ($signal.Signal -eq "BUY" -and -not $positionOpen) {
        # Open position
        $qty = 50
        $entryPrice = $signal.Price
        $positionOpen = $true
        $positionQty = $qty
        
        $executedTrades += [PSCustomObject]@{
            Type = "BUY"
            Price = $entryPrice
            Quantity = $qty
            Timestamp = $signal.Timestamp
            Cost = $qty * $entryPrice
        }
        
        Write-Host "[BUY] Opened position: $qty @ `$$entryPrice" -ForegroundColor Green
    }
    elseif ($signal.Signal -eq "SELL" -and $positionOpen) {
        # Close position
        $exitPrice = $signal.Price
        $pnl = ($exitPrice - $entryPrice) * $positionQty
        $pnlPct = (($exitPrice / $entryPrice) - 1) * 100
        
        $totalPnL += $pnl
        
        if ($pnl -gt 0) {
            $winningTrades++
            $color = "Green"
        } else {
            $losingTrades++
            $color = "Red"
        }
        
        $executedTrades += [PSCustomObject]@{
            Type = "SELL"
            Price = $exitPrice
            Quantity = $positionQty
            Timestamp = $signal.Timestamp
            PnL = $pnl
            PnLPct = $pnlPct
        }
        
        Write-Host "[SELL] Closed position: $positionQty @ `$$exitPrice | P&L: `$$($pnl.ToString('F2')) ($($pnlPct.ToString('+0.00;-0.00'))%)" -ForegroundColor $color
        
        $positionOpen = $false
        $positionQty = 0
    }
}

# Summary
Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "BACKTEST SUMMARY - Moving Average Crossover Strategy" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

$totalTrades = $winningTrades + $losingTrades
$winRate = if ($totalTrades -gt 0) { ($winningTrades / $totalTrades) * 100 } else { 0 }
$startingCapital = 100000
$endingValue = $startingCapital + $totalPnL
$returnPct = ($totalPnL / $startingCapital) * 100

Write-Host "Strategy: 20/50 Period Moving Average Crossover"
Write-Host "Period: 1833 - 2025 (192 years of gold price data)"
Write-Host ""
Write-Host "Total Signals Generated: $($signals.Count) (alternating BUY/SELL)"
Write-Host "Total Completed Trades: $totalTrades"
Write-Host "  Winning Trades: $winningTrades ($($winRate.ToString('F1'))%)"
Write-Host "  Losing Trades: $losingTrades ($((100-$winRate).ToString('F1'))%)"
Write-Host ""
Write-Host "Financial Results:"
Write-Host "  Starting Capital: `$$($startingCapital.ToString('N2'))"
Write-Host "  Total P&L: `$$($totalPnL.ToString('N2'))"

if ($totalTrades -gt 0) {
    $avgPnL = $totalPnL / $totalTrades
    Write-Host "  Avg P&L per Trade: `$$($avgPnL.ToString('N2'))"
}

Write-Host "  Ending Value: `$$($endingValue.ToString('N2'))"
$returnColor = if ($returnPct -gt 0) { "Green" } else { "Red" }
Write-Host "  Total Return: $($returnPct.ToString('+0.00;-0.00'))%" -ForegroundColor $returnColor
Write-Host ""

# Key trades
Write-Host "Notable Trades:" -ForegroundColor Yellow
$sortedTrades = $executedTrades | Where-Object { $_.Type -eq "SELL" } | Sort-Object PnL
$bestTrade = $sortedTrades | Select-Object -Last 1
$worstTrade = $sortedTrades | Select-Object -First 1

if ($bestTrade) {
    Write-Host "  Best Trade: `$$($bestTrade.PnL.ToString('N2')) ($($bestTrade.PnLPct.ToString('+0.00;-0.00'))%)" -ForegroundColor Green
}
if ($worstTrade) {
    Write-Host "  Worst Trade: `$$($worstTrade.PnL.ToString('N2')) ($($worstTrade.PnLPct.ToString('+0.00;-0.00'))%)" -ForegroundColor Red
}

# Check if position still open
if ($positionOpen) {
    Write-Host ""
    Write-Host "WARNING: Position still open at end of backtest!" -ForegroundColor Yellow
    Write-Host "  Entry: $positionQty @ `$$entryPrice"
    $lastPrice = $signals[-1].Price
    $unrealizedPnL = ($lastPrice - $entryPrice) * $positionQty
    Write-Host "  Last Price: `$$lastPrice"
    Write-Host "  Unrealized P&L: `$$($unrealizedPnL.ToString('N2'))"
}

# Export results
$results = @{
    Strategy = "MA Crossover (20/50)"
    TotalTrades = $totalTrades
    WinningTrades = $winningTrades
    LosingTrades = $losingTrades
    WinRate = $winRate
    TotalPnL = $totalPnL
    StartingCapital = $startingCapital
    EndingValue = $endingValue
    ReturnPct = $returnPct
    Trades = $executedTrades
}

$results | ConvertTo-Json -Depth 10 | Out-File "D:\q\backtest_results.json"
Write-Host ""
Write-Host "Results exported to backtest_results.json" -ForegroundColor Cyan
