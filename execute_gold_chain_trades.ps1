# Gold Chain Trading Script
# Strategy: Buy on dips, sell on peaks with 5-10% profit targets

$prices = Get-Content "d:\q\docs\goldprices_series.csv" | ForEach-Object {
    $parts = $_ -split ','
    [PSCustomObject]@{
        Timestamp = [long]$parts[0]
        Price = [double]$parts[1]
    }
}

# Get last 50 price points for recent trading
$recentPrices = $prices | Select-Object -Last 50

Write-Host "=== GOLD CHAIN TRADING STRATEGY ===" -ForegroundColor Cyan
Write-Host "Starting Capital: $10,000"
Write-Host "Recent prices range: $($recentPrices[0].Price) to $($recentPrices[-1].Price)"
Write-Host ""

# Initialize wallet with $10,000
$body = @{
    jsonrpc = "2.0"
    method = "quant-mcp/wallet_deposit"
    params = @{
        amount = 10000
    }
    id = 1
} | ConvertTo-Json -Depth 10

$resp = Invoke-RestMethod -Uri "http://localhost:8080/mcp" -Method Post -Body $body -ContentType "application/json"
Write-Host "✓ Deposited $10,000 to wallet" -ForegroundColor Green

# Trading parameters
$tradeQty = 0.5  # Buy 0.5 oz at a time
$profitTarget = 0.08  # 8% profit target
$position = $null
$tradeCount = 0
$maxTrades = 10

for ($i = 0; $i -lt $recentPrices.Count - 1; $i++) {
    $currentPrice = $recentPrices[$i].Price
    $nextPrice = $recentPrices[$i + 1].Price
    $priceChange = ($nextPrice - $currentPrice) / $currentPrice
    
    # BUY SIGNAL: Price is about to go up (next price is higher)
    if ($position -eq $null -and $priceChange -gt 0.02 -and $tradeCount -lt $maxTrades) {
        Write-Host "`n[BUY] Price: $currentPrice | Next: $nextPrice | Change: $([math]::Round($priceChange*100, 2))%" -ForegroundColor Yellow
        
        # Execute buy
        $buyBody = @{
            jsonrpc = "2.0"
            method = "quant-mcp/execute_buy"
            params = @{
                symbol = "GOLD"
                price = $currentPrice
                quantity = $tradeQty
            }
            id = ($tradeCount * 2 + 2)
        } | ConvertTo-Json -Depth 10
        
        $buyResp = Invoke-RestMethod -Uri "http://localhost:8080/mcp" -Method Post -Body $buyBody -ContentType "application/json"
        
        if ($buyResp.result.token) {
            # Confirm execution
            $confirmBody = @{
                jsonrpc = "2.0"
                method = "quant-mcp/confirm_execution"
                params = @{
                    token = $buyResp.result.token
                }
                id = ($tradeCount * 2 + 3)
            } | ConvertTo-Json -Depth 10
            
            $confirmResp = Invoke-RestMethod -Uri "http://localhost:8080/mcp" -Method Post -Body $confirmBody -ContentType "application/json"
            Write-Host "  ✓ BUY EXECUTED: $tradeQty GOLD @ $$currentPrice" -ForegroundColor Green
            
            $position = @{
                BuyPrice = $currentPrice
                Quantity = $tradeQty
                TargetPrice = $currentPrice * (1 + $profitTarget)
            }
            $tradeCount++
        }
    }
    
    # SELL SIGNAL: We have a position and profit target met or price is dropping
    if ($position -ne $null -and ($currentPrice -ge $position.TargetPrice -or $priceChange -lt -0.03)) {
        $profitPercent = (($currentPrice - $position.BuyPrice) / $position.BuyPrice) * 100
        Write-Host "`n[SELL] Price: $currentPrice | Buy was: $($position.BuyPrice) | Profit: $([math]::Round($profitPercent, 2))%" -ForegroundColor Yellow
        
        # Execute sell
        $sellBody = @{
            jsonrpc = "2.0"
            method = "quant-mcp/execute_sell"
            params = @{
                symbol = "GOLD"
                price = $currentPrice
                quantity = $position.Quantity
            }
            id = ($tradeCount * 2 + 4)
        } | ConvertTo-Json -Depth 10
        
        $sellResp = Invoke-RestMethod -Uri "http://localhost:8080/mcp" -Method Post -Body $sellBody -ContentType "application/json"
        
        if ($sellResp.result.token) {
            # Confirm execution
            $confirmBody = @{
                jsonrpc = "2.0"
                method = "quant-mcp/confirm_execution"
                params = @{
                    token = $sellResp.result.token
                }
                id = ($tradeCount * 2 + 5)
            } | ConvertTo-Json -Depth 10
            
            $confirmResp = Invoke-RestMethod -Uri "http://localhost:8080/mcp" -Method Post -Body $confirmBody -ContentType "application/json"
            
            $profit = ($currentPrice - $position.BuyPrice) * $position.Quantity
            Write-Host "  ✓ SELL EXECUTED: $($position.Quantity) GOLD @ $$currentPrice | Profit: $$([math]::Round($profit, 2))" -ForegroundColor $(if ($profit -gt 0) { "Green" } else { "Red" })
            
            $position = $null
            $tradeCount++
        }
    }
    
    if ($tradeCount -ge $maxTrades) {
        Write-Host "`nReached maximum trades limit ($maxTrades)" -ForegroundColor Cyan
        break
    }
}

# Close any remaining position
if ($position -ne $null) {
    $finalPrice = $recentPrices[-1].Price
    Write-Host "`n[FINAL SELL] Closing position at $finalPrice" -ForegroundColor Yellow
    
    $sellBody = @{
        jsonrpc = "2.0"
        method = "quant-mcp/execute_sell"
        params = @{
            symbol = "GOLD"
            price = $finalPrice
            quantity = $position.Quantity
        }
        id = 999
    } | ConvertTo-Json -Depth 10
    
    $sellResp = Invoke-RestMethod -Uri "http://localhost:8080/mcp" -Method Post -Body $sellBody -ContentType "application/json"
    
    if ($sellResp.result.token) {
        $confirmBody = @{
            jsonrpc = "2.0"
            method = "quant-mcp/confirm_execution"
            params = @{
                token = $sellResp.result.token
            }
            id = 1000
        } | ConvertTo-Json -Depth 10
        
        Invoke-RestMethod -Uri "http://localhost:8080/mcp" -Method Post -Body $confirmBody -ContentType "application/json" | Out-Null
        Write-Host "  ✓ Position closed" -ForegroundColor Green
    }
}

Write-Host "`n=== TRADING COMPLETE ===" -ForegroundColor Cyan
Write-Host "Getting portfolio status..." -ForegroundColor Yellow

# Get final portfolio status
$statusBody = @{
    jsonrpc = "2.0"
    method = "quant-mcp/portfolio_status"
    params = @{}
    id = 2000
} | ConvertTo-Json -Depth 10

$statusResp = Invoke-RestMethod -Uri "http://localhost:8080/mcp" -Method Post -Body $statusBody -ContentType "application/json"
$status = $statusResp.result

Write-Host "`n=== FINAL PORTFOLIO ===" -ForegroundColor Cyan
Write-Host "Wallet Balance: $$($status.walletBalance)"
Write-Host "Deployed Capital: $$($status.deployedCapital)"
Write-Host "Total Equity: $$($status.walletBalance + $status.deployedCapital)"
Write-Host "`nInitial Capital: $10,000"
Write-Host "Net Profit: $$($status.walletBalance + $status.deployedCapital - 10000)"
Write-Host "ROI: $([math]::Round((($status.walletBalance + $status.deployedCapital - 10000) / 10000) * 100, 2))%"

Write-Host "`nView trades at: http://localhost:8080/trades" -ForegroundColor Green
