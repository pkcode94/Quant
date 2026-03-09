# ============================================================
# Full Trading Run: 667 Gold Prices with MCP
# ============================================================

$ErrorActionPreference = 'Stop'

# Load price data
$priceData = Import-Csv "d:\q\docs\goldprices_series.csv" -Header "date", "price"
$prices = [double[]]@($priceData | ForEach-Object { $_.price })
Write-Host "Loaded $($prices.Count) prices (range: $($prices | Measure-Object -Minimum -Maximum | ForEach-Object { "$($_.Minimum) - $($_.Maximum)"}))"

# Configuration
$symbol = "GOLD"
$initialBalance = 200000
$availableFunds = $initialBalance
$levels = 10
$riskCoeff = 0.5
$surplusRate = 0.02

Write-Host "`n=== Phase 1: Initial Setup ===" 

# Login
$loginBody = @{
    username = "ADMIN"
    password = "02AdminA!12"
} | ConvertTo-Json
$session = Invoke-WebRequest -Uri "http://localhost:8080/login" -Method POST -Body $loginBody -SessionVariable sess -UseBasicParsing
$session = $sess

Write-Host "✓ Logged in"

# Deposit initial funds
$rpc = @{
    jsonrpc = "2.0"
    id = "setup_deposit"
    method = "tools/call"
    params = @{
        name = "wallet_deposit"
        arguments = @{ amount = $availableFunds }
    }
} | ConvertTo-Json -Depth 10

$r = Invoke-WebRequest -Uri "http://localhost:8080/mcp" -Method POST -Body $rpc -ContentType "application/json" -WebSession $session -UseBasicParsing
$result = $r.Content | ConvertFrom-Json
Write-Host "✓ Deposited: `$$initialBalance (Balance: $($result.result.newBalance))"

Write-Host "`n=== Phase 2: Generate Trading Plan ===" 

# Generate serial plan for first price
$firstPrice = $prices[0]
$rpc = @{
    jsonrpc = "2.0"
    id = "gen_plan"
    method = "tools/call"
    params = @{
        name = "generate_serial_plan"
        arguments = @{
            currentPrice = $firstPrice
            availableFunds = $availableFunds * 0.8
            levels = $levels
            risk = $riskCoeff
            surplusRate = $surplusRate
        }
    }
} | ConvertTo-Json -Depth 10

$r = Invoke-WebRequest -Uri "http://localhost:8080/mcp" -Method POST -Body $rpc -ContentType "application/json" -WebSession $session -UseBasicParsing
$planResult = $r.Content | ConvertFrom-Json
$plan = $planResult.result

Write-Host "✓ Plan generated:"
Write-Host "  Levels: $($plan.entryCount)"
Write-Host "  Total funding: `$$([math]::Round($plan.totalFunding, 2))"
Write-Host "  Expected profit: `$$([math]::Round($plan.cycle.grossProfit, 2))"

Write-Host "`n=== Phase 3: Execute Trades ===" 

$tradeCount = 0
$totalCost = 0
$tokens = @()

# Execute some buy trades across price range
$sampleIndices = @(0, 50, 100, 150, 200, 250)
foreach ($idx in $sampleIndices) {
    if ($idx -ge $prices.Count) { break }
    
    $price = $prices[$idx]
    $qty = 1
    
    $rpc = @{
        jsonrpc = "2.0"
        id = "buy_$idx"
        method = "tools/call"
        params = @{
            name = "execute_buy"
            arguments = @{
                symbol = $symbol
                price = $price
                quantity = $qty
            }
        }
    } | ConvertTo-Json -Depth 10
    
    $r = Invoke-WebRequest -Uri "http://localhost:8080/mcp" -Method POST -Body $rpc -ContentType "application/json" -WebSession $session -UseBasicParsing
    $buyResult = $r.Content | ConvertFrom-Json
    $token = $buyResult.result.token
    $cost = $price * $qty
    
    Write-Host "  Buy #$($tradeCount+1) @ Price `$$price: Token=$($token.Substring(0,8))..."
    $tokens += $token
    $totalCost += $cost
    $tradeCount++
}

Write-Host "`n✓ Executed $tradeCount buy trades (Total cost: `$$([math]::Round($totalCost, 2)))"

Write-Host "`n=== Phase 4: Confirm Trades ===" 

$confirmedCount = 0
foreach ($token in $tokens) {
    $rpc = @{
        jsonrpc = "2.0"
        id = "confirm_$confirmedCount"
        method = "tools/call"
        params = @{
            name = "confirm_execution"
            arguments = @{ token = $token }
        }
    } | ConvertTo-Json -Depth 10
    
    $r = Invoke-WebRequest -Uri "http://localhost:8080/mcp" -Method POST -Body $rpc -ContentType "application/json" -WebSession $session -UseBasicParsing
    $confirmResult = $r.Content | ConvertFrom-Json
    
    if ($confirmResult.result.success) {
        Write-Host "  ✓ Trade $($confirmResult.result.tradeId) confirmed"
        $confirmedCount++
    }
}

Write-Host "`n✓ Confirmed $confirmedCount trades"

Write-Host "`n=== Phase 5: Portfolio Status ===" 

$rpc = @{
    jsonrpc = "2.0"
    id = "portfolio"
    method = "tools/call"
    params = @{
        name = "portfolio_status"
        arguments = @{}
    }
} | ConvertTo-Json -Depth 10

$r = Invoke-WebRequest -Uri "http://localhost:8080/mcp" -Method POST -Body $rpc -ContentType "application/json" -WebSession $session -UseBasicParsing
$portfolio = $r.Content | ConvertFrom-Json

Write-Host "Wallet:"
Write-Host "  Liquid: `$$([math]::Round($portfolio.result.wallet.liquid, 2))"
Write-Host "  Deployed: `$$([math]::Round($portfolio.result.wallet.deployed, 2))"
Write-Host "  Total: `$$([math]::Round($portfolio.result.wallet.total, 2))"

if ($portfolio.result.openPositions) {
    Write-Host "`nOpen Positions:"
    $portfolio.result.openPositions | ForEach-Object {
        Write-Host "  Trade $($_.tradeId): $($_.quantity) $($_.symbol) @ `$$([math]::Round($_.entryPrice, 2))"
    }
}

Write-Host "`n=== Phase 6: Price Check Analysis ===" 

# Check P&L at various price points
$checkPrices = @($prices[0], $prices[100], $prices[200], $prices[300], $prices[-1])
$checkPrices | ForEach-Object {
    $price = $_
    $rpc = @{
        jsonrpc = "2.0"
        id = "check_$price"
        method = "tools/call"
        params = @{
            name = "price_check"
            arguments = @{
                symbol = $symbol
                price = $price
            }
        }
    } | ConvertTo-Json -Depth 10
    
    $r = Invoke-WebRequest -Uri "http://localhost:8080/mcp" -Method POST -Body $rpc -ContentType "application/json" -WebSession $session -UseBasicParsing
    $check = $r.Content | ConvertFrom-Json
    
    $openPnL = 0
    if ($check.result.openPositions) {
        $openPnL = ($check.result.openPositions | Measure-Object -Property unrealizedPnl -Sum).Sum
    }
    
    Write-Host "  @ `$$price: Open P&L = `$$([math]::Round($openPnL, 2))"
}

Write-Host "`n=== Phase 7: Execute Exits ===" 

# Get exit points
$rpc = @{
    jsonrpc = "2.0"
    id = "list_exits"
    method = "tools/call"
    params = @{
        name = "list_exits"
        arguments = @{}
    }
} | ConvertTo-Json -Depth 10

$r = Invoke-WebRequest -Uri "http://localhost:8080/mcp" -Method POST -Body $rpc -ContentType "application/json" -WebSession $session -UseBasicParsing
$exits = $r.Content | ConvertFrom-Json

if ($exits.result -and $exits.result.Count -gt 0) {
    Write-Host "Available exits: $($exits.result.Count)"
    
    # Execute first few exits at favorable prices
    $lastPrice = $prices[-1]
    $exitCount = 0
    
    $exits.result | Select-Object -First 3 | ForEach-Object {
        $exitId = $_.exitId
        $tpPrice = $_.tpPrice
        
        if ($lastPrice -ge $tpPrice) {
            $rpc = @{
                jsonrpc = "2.0"
                id = "exit_$exitId"
                method = "tools/call"
                params = @{
                    name = "execute_exit"
                    arguments = @{ exitId = $exitId }
                }
            } | ConvertTo-Json -Depth 10
            
            $r = Invoke-WebRequest -Uri "http://localhost:8080/mcp" -Method POST -Body $rpc -ContentType "application/json" -WebSession $session -UseBasicParsing
            $exitResult = $r.Content | ConvertFrom-Json
            
            if ($exitResult.result) {
                Write-Host "  ✓ Exit $exitId executed (TP: `$$tpPrice)"
                $exitCount++
            }
        }
    }
    
    Write-Host "`n✓ Executed $exitCount exits"
}

Write-Host "`n=== Phase 8: Final Portfolio ===" 

$rpc = @{
    jsonrpc = "2.0"
    id = "final_portfolio"
    method = "tools/call"
    params = @{
        name = "portfolio_status"
        arguments = @{}
    }
} | ConvertTo-Json -Depth 10

$r = Invoke-WebRequest -Uri "http://localhost:8080/mcp" -Method POST -Body $rpc -ContentType "application/json" -WebSession $session -UseBasicParsing
$final = $r.Content | ConvertFrom-Json

Write-Host "Final Wallet:"
Write-Host "  Liquid: `$$([math]::Round($final.result.wallet.liquid, 2))"
Write-Host "  Deployed: `$$([math]::Round($final.result.wallet.deployed, 2))"
Write-Host "  Total: `$$([math]::Round($final.result.wallet.total, 2))"

$gains = $final.result.wallet.total - $initialBalance
$gainPct = ($gains / $initialBalance) * 100

Write-Host "`n=== RESULTS ===" 
Write-Host "Initial Balance: `$$initialBalance"
Write-Host "Final Balance: `$$([math]::Round($final.result.wallet.total, 2))"
Write-Host "Gain/Loss: `$$([math]::Round($gains, 2)) ($([math]::Round($gainPct, 2))%)"
Write-Host "`n✓ Full trading run completed successfully!"
