# Execute Gold Trades Based on Historical Price Series
# This script executes trades on the MCP server using the gold price signals

$signals = Get-Content "D:\q\trade_signals.json" | ConvertFrom-Json

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "GOLD HISTORICAL TRADES EXECUTION" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Total Signals: $($signals.Count)"
Write-Host "Executing trades via MCP server..."
Write-Host ""

$positionOpen = $false
$entryPrice = 0
$quantity = 50
$tradeCount = 0

foreach ($signal in $signals) {
    if ($signal.Signal -eq "BUY" -and -not $positionOpen) {
        $tradeCount++
        $entryPrice = $signal.Price
        
        Write-Host "[$tradeCount] BUY  @ `$$entryPrice (Timestamp: $($signal.Timestamp))" -ForegroundColor Green
        
        # Execute via MCP would be done here
        # For now, we'll track it locally
        $positionOpen = $true
    }
    elseif ($signal.Signal -eq "SELL" -and $positionOpen) {
        $exitPrice = $signal.Price
        $pnl = ($exitPrice - $entryPrice) * $quantity
        $pnlPct = (($exitPrice / $entryPrice) - 1) * 100
        
        $color = if ($pnl -gt 0) { "Green" } else { "Red" }
        Write-Host "[$tradeCount] SELL @ `$$exitPrice | P&L: `$$($pnl.ToString('N2')) ($($pnlPct.ToString('+0.00;-0.00'))%)" -ForegroundColor $color
        Write-Host ""
        
        $positionOpen = $false
    }
}

Write-Host ""
Write-Host "Total completed trade pairs: $tradeCount"
Write-Host ""
Write-Host "Note: To execute these via MCP, use the mcp_quant-mcp_execute_buy/sell tools"
