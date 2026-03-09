# Complete Trading Workflow - 667 Gold Prices
# Executes: login -> deposit -> plan generation -> 5 trades -> confirmations -> portfolio check

Write-Host "`n=== QUANT MCP TRADING WORKFLOW ===" -ForegroundColor Cyan
Write-Host "Executing comprehensive trading with 667-price gold series`n" -ForegroundColor Yellow

# ───────── STEP 1: Load Prices ─────────
Write-Host "[1/7] Loading price data..." -ForegroundColor Yellow
$csvFile = "d:\q\docs\goldprices_series.csv"
$allPrices = @()
$lines = @(Get-Content $csvFile)
foreach ($line in $lines) {
    $parts = $line -split ','
    if ($parts.Length -eq 2 -and $parts[1] -match '^\d+\.?\d*$') {
        $allPrices += [double]$parts[1]
    }
}
Write-Host "✓ Loaded $($allPrices.Count) prices" -ForegroundColor Green
Write-Host "  Range: `$$([Math]::Round([Math]::Min($allPrices), 2)) - `$$([Math]::Round([Math]::Max($allPrices), 2))" -ForegroundColor Cyan

# Select 6 prices across the range
$tradeIndices = @(
    [int]($allPrices.Count * 0.05)   # 5%
    [int]($allPrices.Count * 0.20)   # 20%
    [int]($allPrices.Count * 0.40)   # 40%
    [int]($allPrices.Count * 0.60)   # 60%
    [int]($allPrices.Count * 0.80)   # 80%
    [int]($allPrices.Count * 0.95)   # 95%
)
$tradePrices = @($tradeIndices | ForEach-Object { $allPrices[$_] })
Write-Host "✓ Selected 6 trading prices: $($tradePrices | ForEach-Object { "`$$([Math]::Round($_, 2))" } | Join-String -Separator ', ')`n" -ForegroundColor Green

# ───────── STEP 2: Authenticate ─────────
Write-Host "[2/7] Authenticating..." -ForegroundColor Yellow
$loginBody = @{
    username = "admin"
    password = "02AdminA!12"
} | ConvertTo-Json

try {
    $loginResp = Invoke-WebRequest -Uri "http://localhost:8080/auth/login" `
        -Method POST -Body $loginBody -ContentType "application/json" `
        -UseBasicParsing -ErrorAction Stop
    Write-Host "✓ Authenticated successfully" -ForegroundColor Green
} catch {
    Write-Host "✗ Auth failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

$session = $loginResp.Headers["Set-Cookie"]
Write-Host "✓ Session established`n" -ForegroundColor Green

# ───────── STEP 3: Deposit Funds ─────────
Write-Host "[3/7] Preparing wallet..." -ForegroundColor Yellow
$depositBody = @{
    jsonrpc = "2.0"
    method = "tools/call"
    id = 1
    params = @{
        name = "wallet_deposit"
        arguments = @{
            amount = 200000
        }
    }
} | ConvertTo-Json -Depth 10

try {
    $depositResp = Invoke-WebRequest -Uri "http://localhost:8080/mcp" `
        -Method POST -Body $depositBody -ContentType "application/json" `
        -UseBasicParsing -ErrorAction Stop
    $depositData = $depositResp.Content | ConvertFrom-Json
    if ($depositData.result) {
        Write-Host "✓ Deposited 200,000" -ForegroundColor Green
    }
} catch {
    Write-Host "⚠ Deposit failed (trade may continue): $($_.Exception.Message)" -ForegroundColor Yellow
}
Write-Host ""

# ───────── STEP 4: Execute Trades ─────────
Write-Host "[4/7] Executing $($tradePrices.Count) buy trades..." -ForegroundColor Yellow
$tokens = @()
for ($i = 0; $i -lt $tradePrices.Count; $i++) {
    $price = $tradePrices[$i]
    $roundPrice = [Math]::Round($price, 2)
    
    $buyBody = @{
        jsonrpc = "2.0"
        method = "tools/call"
        id = 100 + $i
        params = @{
            name = "execute_buy"
            arguments = @{
                symbol = "GOLD"
                price = $price
                quantity = 0.1
                buyFee = 0
            }
        }
    } | ConvertTo-Json -Depth 10
    
    try {
        $buyResp = Invoke-WebRequest -Uri "http://localhost:8080/mcp" `
            -Method POST -Body $buyBody -ContentType "application/json" `
            -UseBasicParsing -SkipCertificateCheck -ErrorAction Stop
        $buyData = $buyResp.Content | ConvertFrom-Json
        
        if ($buyData.result) {
            $tokens += $buyData.result
            Write-Host "  ✓ Trade $($i+1)/6: Buy @ $$($roundPrice) qty 0.1 - Token: $($buyData.result.Substring(0, 12))..." -ForegroundColor Green
        } elseif ($buyData.error) {
            Write-Host "  ✗ Trade $($i+1)/6: $($buyData.error.message)" -ForegroundColor Red
        }
    } catch {
        Write-Host "  ✗ Trade $($i+1)/6: Request failed - $($_.Exception.Message)" -ForegroundColor Red
    }
    
    Start-Sleep -Milliseconds 100
}
Write-Host "✓ Executed $($tokens.Count) trades`n" -ForegroundColor Green

# ───────── STEP 5: Confirm Trades ─────────
Write-Host "[5/7] Confirming $($tokens.Count) trades..." -ForegroundColor Yellow
$confirmed = 0
for ($i = 0; $i -lt $tokens.Count; $i++) {
    $token = $tokens[$i]
    
    $confirmBody = @{
        jsonrpc = "2.0"
        method = "tools/call"
        id = 200 + $i
        params = @{
            name = "confirm_execution"
            arguments = @{
                token = $token
            }
        }
    } | ConvertTo-Json -Depth 10
    
    try {
        $confirmResp = Invoke-WebRequest -Uri "http://localhost:8080/mcp" `
            -Method POST -Body $confirmBody -ContentType "application/json" `
            -UseBasicParsing -SkipCertificateCheck -ErrorAction Stop
        $confirmData = $confirmResp.Content | ConvertFrom-Json
        
        if ($confirmData.result) {
            $confirmed++
            Write-Host "  ✓ Trade $($i+1) confirmed" -ForegroundColor Green
        } elseif ($confirmData.error) {
            Write-Host "  ✗ Trade $($i+1): $($confirmData.error.message)" -ForegroundColor Red
        }
    } catch {
        Write-Host "  ✗ Trade $($i+1): Request failed" -ForegroundColor Red
    }
    
    Start-Sleep -Milliseconds 50
}
Write-Host "✓ Confirmed $($confirmed) trades`n" -ForegroundColor Green

# ───────── STEP 6: Check Portfolio ─────────
Write-Host "[6/7] Checking portfolio status..." -ForegroundColor Yellow
$portfolioBody = @{
    jsonrpc = "2.0"
    method = "tools/call"
    id = 300
    params = @{
        name = "portfolio_status"
        arguments = @{}
    }
} | ConvertTo-Json -Depth 10

try {
    $portfolioResp = Invoke-WebRequest -Uri "http://localhost:8080/mcp" `
        -Method POST -Body $portfolioBody -ContentType "application/json" `
        -UseBasicParsing -SkipCertificateCheck -ErrorAction Stop
    $portfolioData = $portfolioResp.Content | ConvertFrom-Json
    
    if ($portfolioData.result) {
        $wallet = $portfolioData.result.wallet
        $positions = $portfolioData.result.positions
        
        Write-Host "✓ Wallet Status:" -ForegroundColor Green
        Write-Host "  Liquid: `$$($wallet.liquid)" -ForegroundColor Cyan
        Write-Host "  Deployed: `$$($wallet.deployed)" -ForegroundColor Cyan
        Write-Host "  Total: `$$($wallet.total)" -ForegroundColor Cyan
        
        if ($positions.Count -gt 0) {
            Write-Host "`n✓ Open Positions: $($positions.Count)" -ForegroundColor Green
            foreach ($pos in $positions) {
                Write-Host "    - $($pos.symbol): $($pos.quantity) @ `$$($pos.entry_price)" -ForegroundColor Cyan
            }
        }
    }
} catch {
    Write-Host "✗ Portfolio query failed: $($_.Exception.Message)" -ForegroundColor Red
}
Write-Host ""

# ───────── STEP 7: List All Trades ─────────
Write-Host "[7/7] Listing all trades..." -ForegroundColor Yellow
$tradeListBody = @{
    jsonrpc = "2.0"
    method = "tools/call"
    id = 400
    params = @{
        name = "list_trades"
        arguments = @{}
    }
} | ConvertTo-Json -Depth 10

try {
    $tradeResp = Invoke-WebRequest -Uri "http://localhost:8080/mcp" `
        -Method POST -Body $tradeListBody -ContentType "application/json" `
        -UseBasicParsing -SkipCertificateCheck -ErrorAction Stop
    $tradeData = $tradeResp.Content | ConvertFrom-Json
    
    if ($tradeData.result) {
        Write-Host "✓ Total trades in ledger: $($tradeData.result.Count)" -ForegroundColor Green
    }
} catch {
    Write-Host "⚠ Trade listing failed" -ForegroundColor Yellow
}
Write-Host ""

# ───────── SUMMARY ─────────
Write-Host "════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "✓ TRADING WORKFLOW COMPLETE" -ForegroundColor Green
Write-Host "════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "✓ Prices loaded: $($allPrices.Count)" -ForegroundColor Green
Write-Host "✓ Trades executed: $($tokens.Count)" -ForegroundColor Green
Write-Host "✓ Trades confirmed: $($confirmed)" -ForegroundColor Green
Write-Host "✓ Portfolio active: ✓" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Monitor price movements" -ForegroundColor Cyan
Write-Host "  2. Execute take-profits/stop-losses" -ForegroundColor Cyan
Write-Host "  3. Generate trading signals" -ForegroundColor Cyan
