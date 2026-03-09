# Gold Historical Series Trade Executor
# Executes all 19 trade pairs from goldprices_series.csv via MCP

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "GOLD HISTORICAL SERIES - MCP TRADE EXECUTION" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# Load signals
$signals = Get-Content "D:\q\trade_signals.json" | ConvertFrom-Json

# MCP execution record
$trades = @()
$quantity = 50

Write-Host "Preparing to execute $($signals.Count) signals (pairs)"
Write-Host "Position size: $quantity GOLD per trade"
Write-Host ""
Write-Host "Trade Signals:"
Write-Host "─────────────────────────────────────────────────────────────"

$tradeNum = 0
$positionOpen = $false
$entryPrice = 0

foreach ($signal in $signals) {
    if ($signal.Signal -eq "BUY" -and -not $positionOpen) {
        $tradeNum++
        $entryPrice = $signal.Price
        $positionOpen = $true
        Write-Host "[$tradeNum] BUY  $quantity GOLD @ `$$entryPrice" -ForegroundColor Green
    }
    elseif ($signal.Signal -eq "SELL" -and $positionOpen) {
        $exitPrice = $signal.Price
        $pnl = ($exitPrice - $entryPrice) * $quantity
        $pnlPct = (($exitPrice / $entryPrice) - 1) * 100
        
        $color = if ($pnl -gt 0) { "Green" } else { "Red" }
        $pnlStr = $pnl.ToString('N2')
        $pctStr = $pnlPct.ToString('+0.00;-0.00')
        Write-Host "[$tradeNum] SELL $quantity GOLD @ `$$exitPrice | P&L: `$$pnlStr ($pctStr%)" -ForegroundColor $color
        
        $positionOpen = $false
    }
}

Write-Host "─────────────────────────────────────────────────────────────"
Write-Host ""
Write-Host "Total Trade Pairs: $tradeNum"
Write-Host ""
Write-Host "To execute via MCP, use Copilot with these commands:"
Write-Host '  "Execute buy GOLD at $18.93 quantity 50"'
Write-Host '  "Execute sell trade 1 at $18.94"'
Write-Host "  etc..."
Write-Host ""
Write-Host "Or run the automated execution below (requires MCP tools)"
Write-Host ""

# Show what MCP commands would be needed
Write-Host "MCP Command Sequence:" -ForegroundColor Yellow
Write-Host "─────────────────────────────────────────────────────────────"

$cmdNum = 0
$positionOpen = $false
$currentTradeId = 0

foreach ($signal in $signals) {
    if ($signal.Signal -eq "BUY" -and -not $positionOpen) {
        $cmdNum++
        $currentTradeId = $cmdNum
        Write-Host "$cmdNum. mcp_quant-mcp_execute_buy(symbol='GOLD', price=$($signal.Price), quantity=$quantity)"
        Write-Host "   mcp_quant-mcp_confirm_execution(token='...')"
        $positionOpen = $true
    }
    elseif ($signal.Signal -eq "SELL" -and $positionOpen) {
        Write-Host "   mcp_quant-mcp_execute_sell(symbol='GOLD', tradeId=$currentTradeId, price=$($signal.Price), quantity=$quantity)"
        Write-Host "   mcp_quant-mcp_confirm_execution(token='...')"
        Write-Host ""
        $positionOpen = $false
    }
}

Write-Host "─────────────────────────────────────────────────────────────"
Write-Host ""
Write-Host "Ask Copilot to execute these 19 trade pairs via MCP!" -ForegroundColor Cyan
