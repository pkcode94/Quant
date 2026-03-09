# Focused trading workflow - incremental execution with status output
# Extract prices and execute 5 buy trades at different price levels

Write-Host "=== GOLD TRADING WORKFLOW: 667-PRICE SERIES ===" -ForegroundColor Cyan

# Step 1: Load ALL prices
Write-Host "[1/6] Loading prices..." -ForegroundColor Yellow
$allLines = @(Get-Content "d:\q\docs\goldprices_series.csv")
$allPrices = @()
foreach ($line in $allLines) {
    $parts = $line -split ','
    if ($parts.Length -eq 2 -and $parts[1] -match '^\d+\.?\d*$') {
        $allPrices += [double]$parts[1]
    }
}
Write-Host "Loaded $($allPrices.Count) prices. Range: $([Math]::Min($allPrices)) to $([Math]::Max($allPrices))" -ForegroundColor Green

# Select 5 prices across the range (10%, 30%, 50%, 70%, 90%)
$indices = @(
    [int]($allPrices.Count * 0.1)
    [int]($allPrices.Count * 0.3)
    [int]($allPrices.Count * 0.5)
    [int]($allPrices.Count * 0.7)
    [int]($allPrices.Count * 0.9)
)
$selectedPrices = @($indices | ForEach-Object { $allPrices[$_] })
Write-Host "Selected 5 prices from dataset: $($selectedPrices -join ', ')" -ForegroundColor Green

# Step 2: Login
Write-Host "[2/6] Authenticating..." -ForegroundColor Yellow
$loginBody = @{
    username = "admin"
    password = "02AdminA!12"
} | ConvertTo-Json
$loginResp = Invoke-WebRequest -Uri "http://localhost:8080/auth/login" -Method POST -Body $loginBody -ContentType "application/json" -ErrorAction Stop
Write-Host "Login OK - Session established" -ForegroundColor Green

# Extract session from response
$session = $loginResp.Headers["Set-Cookie"]
if ($loginResp.StatusCode -eq 200) {
    Write-Host "Authenticated successfully" -ForegroundColor Green
} else {
    Write-Host "Auth failed: $($loginResp.StatusCode)" -ForegroundColor Red
    exit 1
}

# Step 3: Execute 5 buy trades
Write-Host "[3/6] Executing 5 buy trades..." -ForegroundColor Yellow
$tokens = @()
for ($i = 0; $i -lt $selectedPrices.Count; $i++) {
    $price = $selectedPrices[$i]
    Write-Host "  Trade $($i+1)/5 at price $price..."
    
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
    
    $buyResp = Invoke-WebRequest -Uri "http://localhost:8080/mcp" -Method POST -Body $buyBody -ContentType "application/json" -Cookie $session -ErrorAction Stop
    $buyData = $buyResp.Content | ConvertFrom-Json
    
    if ($buyData.result) {
        $tokens += $buyData.result
        Write-Host "    ✓ Token: $($buyData.result.Substring(0, 12))..." -ForegroundColor Green
    } else {
        Write-Host "    ✗ Failed: $($buyData.error.message)" -ForegroundColor Red
    }
}

# Step 4: Confirm all trades
Write-Host "[4/6] Confirming $($tokens.Count) trades..." -ForegroundColor Yellow
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
    
    $confirmResp = Invoke-WebRequest -Uri "http://localhost:8080/mcp" -Method POST -Body $confirmBody -ContentType "application/json" -Cookie $session -ErrorAction Stop
    $confirmData = $confirmResp.Content | ConvertFrom-Json
    
    if ($confirmData.result) {
        $confirmed++
        Write-Host "  ✓ Trade $($i+1) confirmed" -ForegroundColor Green
    } else {
        Write-Host "  ✗ Trade $($i+1) failed: $($confirmData.error.message)" -ForegroundColor Red
    }
}

# Step 5: Query portfolio
Write-Host "[5/6] Checking portfolio..." -ForegroundColor Yellow
$portfolioBody = @{
    jsonrpc = "2.0"
    method = "tools/call"
    id = 300
    params = @{
        name = "portfolio_status"
        arguments = @{}
    }
} | ConvertTo-Json -Depth 10

$portfolioResp = Invoke-WebRequest -Uri "http://localhost:8080/mcp" -Method POST -Body $portfolioBody -ContentType "application/json" -Cookie $session -ErrorAction Stop
$portfolioData = $portfolioResp.Content | ConvertFrom-Json

if ($portfolioData.result) {
    $wallet = $portfolioData.result.wallet
    Write-Host "Wallet Status:" -ForegroundColor Green
    Write-Host "  Liquid: $($wallet.liquid)" -ForegroundColor Cyan
    Write-Host "  Deployed: $($wallet.deployed)" -ForegroundColor Cyan
    Write-Host "  Total: $($wallet.total)" -ForegroundColor Cyan
    Write-Host "Positions: $($portfolioData.result.positions.Count)" -ForegroundColor Cyan
}

# Step 6: Summary
Write-Host "[6/6] Workflow complete" -ForegroundColor Yellow
Write-Host "=== TRADING RUN SUMMARY ===" -ForegroundColor Cyan
Write-Host "✓ Loaded: $($allPrices.Count) prices" -ForegroundColor Green
Write-Host "✓ Executed: $($tokens.Count) trades" -ForegroundColor Green
Write-Host "✓ Confirmed: $confirmed trades" -ForegroundColor Green
Write-Host "✓ Portfolio: Wallet=$($wallet.total), Positions=$($portfolioData.result.positions.Count)" -ForegroundColor Green
Write-Host "=== SUCCESS ===" -ForegroundColor Green
