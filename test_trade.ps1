# Sample MCP Trade Test using JSON-RPC 2.0
Write-Host "`n=== MCP Trade Test ===" -ForegroundColor Cyan

# Create session
$session = New-Object Microsoft.PowerShell.Commands.WebRequestSession

# Login
Write-Host "`n[1/4] Logging in..." -ForegroundColor Yellow
$loginBody = @{username='admin'; password='admin123'} | ConvertTo-Json
try {
    $loginResp = Invoke-WebRequest -Uri "http://localhost:8080/api/login" `
        -Method POST -Body $loginBody -ContentType "application/json" `
        -WebSession $session -UseBasicParsing -ErrorAction Stop
    Write-Host "[OK] Login successful" -ForegroundColor Green
} catch {
    Write-Host "[FAIL] Login failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

Start-Sleep -Milliseconds 300

# Execute buy via MCP JSON-RPC
Write-Host "`n[2/4] Executing buy order via MCP..." -ForegroundColor Yellow
$buyRpc = @{
    jsonrpc = "2.0"
    id = 1
    method = "tools/call"
    params = @{
        name = "execute_buy"
        arguments = @{
            symbol = "GOLD"
            price = 1500.0
            quantity = 0.1
        }
    }
} | ConvertTo-Json -Depth 10

try {
    $buyResp = Invoke-WebRequest -Uri "http://localhost:8080/mcp" `
        -Method POST -Body $buyRpc -ContentType "application/json" `
        -WebSession $session -UseBasicParsing -ErrorAction Stop
    $buyData = $buyResp.Content | ConvertFrom-Json
    if ($buyData.error) {
        Write-Host "[FAIL] Buy failed: $($buyData.error.message)" -ForegroundColor Red
        exit 1
    }
    Write-Host "[OK] Buy executed" -ForegroundColor Green
    Write-Host "  Token: $($buyData.result.content[0].text)" -ForegroundColor Cyan
    $token = ($buyData.result.content[0].text | ConvertFrom-Json).token
} catch {
    Write-Host "[FAIL] Buy failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

Start-Sleep -Milliseconds 300

# Confirm execution via MCP
Write-Host "`n[3/4] Confirming execution via MCP..." -ForegroundColor Yellow
$confirmRpc = @{
    jsonrpc = "2.0"
    id = 2
    method = "tools/call"
    params = @{
        name = "confirm_execution"
        arguments = @{
            token = $token
        }
    }
} | ConvertTo-Json -Depth 10

try {
    $confirmResp = Invoke-WebRequest -Uri "http://localhost:8080/mcp" `
        -Method POST -Body $confirmRpc -ContentType "application/json" `
        -WebSession $session -UseBasicParsing -ErrorAction Stop
    $confirmData = $confirmResp.Content | ConvertFrom-Json
    if ($confirmData.error) {
        Write-Host "[FAIL] Confirm failed: $($confirmData.error.message)" -ForegroundColor Red
        exit 1
    }
    Write-Host "[OK] Trade confirmed" -ForegroundColor Green
} catch {
    Write-Host "[FAIL] Confirm failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

# Check portfolio via MCP
Write-Host "`n[4/4] Checking portfolio via MCP..." -ForegroundColor Yellow
$portfolioRpc = @{
    jsonrpc = "2.0"
    id = 3
    method = "tools/call"
    params = @{
        name = "portfolio_status"
        arguments = @{}
    }
} | ConvertTo-Json -Depth 10

try {
    $portfolioResp = Invoke-WebRequest -Uri "http://localhost:8080/mcp" `
        -Method POST -Body $portfolioRpc -ContentType "application/json" `
        -WebSession $session -UseBasicParsing -ErrorAction Stop
    $portfolioData = $portfolioResp.Content | ConvertFrom-Json
    if ($portfolioData.error) {
        Write-Host "[FAIL] Portfolio failed: $($portfolioData.error.message)" -ForegroundColor Red
        exit 1
    }
    $portfolio = $portfolioData.result.content[0].text | ConvertFrom-Json
    Write-Host "`n=== Portfolio Status ===" -ForegroundColor Cyan
    Write-Host "Wallet: $($portfolio.wallet)" -ForegroundColor Yellow
    Write-Host "Positions: $($portfolio.positions.Count)" -ForegroundColor Yellow
    if ($portfolio.positions -and $portfolio.positions.Count -gt 0) {
        Write-Host "`nRecent positions:" -ForegroundColor Gray
        $portfolio.positions | Select-Object -First 3 | ForEach-Object {
            Write-Host "  Symbol: $($_.symbol), Qty: $($_.quantity), Price: $($_.price)" -ForegroundColor White
        }
    }
} catch {
    Write-Host "[FAIL] Portfolio failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

Write-Host "`n[OK] All tests passed!" -ForegroundColor Green
