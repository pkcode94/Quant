# Test script to diagnose trade persistence
Write-Host "`n=== MCP Trade Persistence Test ===" -ForegroundColor Cyan

# Login
$loginBody = @{ username="admin"; password="02AdminA!12" } | ConvertTo-Json
$loginResp = Invoke-WebRequest -Uri "http://localhost:8080/auth/login" `
    -Method POST -Body $loginBody -ContentType "application/json" `
    -UseBasicParsing -ErrorAction Stop
Write-Host "✓ Logged in" -ForegroundColor Green

$session = $loginResp.Headers["Set-Cookie"]

# Execute ONE buy trade
Write-Host "`n=== Executing single buy trade: GOLD @  999.99, qty 0.05 ===" -ForegroundColor Yellow
$buyBody = @{
    jsonrpc = "2.0"
    method = "tools/call"
    id = 888
    params = @{
        name = "execute_buy"
        arguments = @{
            symbol = "GOLD"
            price = 999.99
            quantity = 0.05
            buyFee = 0
        }
    }
} | ConvertTo-Json -Depth 10

$buyResp = Invoke-WebRequest -Uri "http://localhost:8080/mcp" `
    -Method POST -Body $buyBody -ContentType "application/json" `
    -UseBasicParsing -ErrorAction Stop

$buyData = $buyResp.Content | ConvertFrom-Json
Write-Host "Buy response:" -ForegroundColor Cyan
$buyData | ConvertTo-Json -Depth 3
$token = $buyData.result
Write-Host "`n✓ Token: $token" -ForegroundColor Green

# Confirm the trade
Write-Host "`n=== Confirming trade with token ===" -ForegroundColor Yellow
$confirmBody = @{
    jsonrpc = "2.0"
    method = "tools/call"
    id = 889
    params = @{
        name = "confirm_execution"
        arguments = @{
            token = $token
        }
    }
} | ConvertTo-Json -Depth 10

$confirmResp = Invoke-WebRequest -Uri "http://localhost:8080/mcp" `
    -Method POST -Body $confirmBody -ContentType "application/json" `
    -UseBasicParsing -ErrorAction Stop

$confirmData = $confirmResp.Content | ConvertFrom-Json
Write-Host "Confirm response:" -ForegroundColor Cyan
$confirmData | ConvertTo-Json -Depth 3

if ($confirmData.result) {
    $tradeId = $confirmData.result.tradeId
    Write-Host "`n✓ Trade ID: $tradeId" -ForegroundColor Green
    
    # NOW list trades to see if it appears
    Write-Host "`n=== Listing ALL trades ===" -ForegroundColor Yellow
    $listBody = @{
        jsonrpc = "2.0"
        method = "tools/call"
        id = 890
        params = @{
            name = "list_trades"
            arguments = @{}
        }
    } | ConvertTo-Json -Depth 10
    
    $listResp = Invoke-WebRequest -Uri "http://localhost:8080/mcp" `
        -Method POST -Body $listBody -ContentType "application/json" `
        -UseBasicParsing -ErrorAction Stop
    
    $listData = $listResp.Content | ConvertFrom-Json
    Write-Host "List trades response:" -ForegroundColor Cyan
    $listData | ConvertTo-Json -Depth 5
}

Write-Host "`n=== Checking file system ===" -ForegroundColor Yellow
Write-Host "Trades.json:" -ForegroundColor Cyan
Get-Content 'd:\q\users\admin\db\trades.json' -Raw
