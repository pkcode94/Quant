$ErrorActionPreference = "Stop"
$uri = "http://localhost:8080/mcp"
$headers = @{"Content-Type" = "application/json"}

Write-Host "=== Testing Single Trade ===" -ForegroundColor Cyan

# Execute buy
Write-Host "1. Executing buy..." -ForegroundColor Yellow
$buyBody = @{
    jsonrpc = "2.0"
    method = "tools/call"
    id = 1
    params = @{
        name = "execute_buy"
        arguments = @{
            symbol = "GOLD"
            price = 100.0
            quantity = 0.5
        }
    }
} | ConvertTo-Json -Depth 10

$buyResp = Invoke-RestMethod -Uri $uri -Method POST -Body $buyBody -Headers $headers
Write-Host "Buy response:" -ForegroundColor Green
$buyResp | ConvertTo-Json -Depth 10

# Extract token
$token = ($buyResp.result.content[0].text | ConvertFrom-Json).token
Write-Host "`nConfirmation token: $token" -ForegroundColor Green

# Confirm
Write-Host "`n2. Confirming trade..." -ForegroundColor Yellow
$confirmBody = @{
    jsonrpc = "2.0"
    method = "tools/call"
    id = 2
    params = @{
        name = "confirm_execution"
        arguments = @{
            token = $token
        }
    }
} | ConvertTo-Json -Depth 10

try {
    $confirmResp = Invoke-RestMethod -Uri $uri -Method POST -Body $confirmBody -Headers $headers
    Write-Host "Confirm response:" -ForegroundColor Green
    $confirmResp | ConvertTo-Json -Depth 10
    
    $confirmJson = $confirmResp.result.content[0].text | ConvertFrom-Json
    if ($confirmJson.error) {
        Write-Host "`nERROR DETECTED: $($confirmJson.error)" -ForegroundColor Red
        exit 1
    }
} catch {
    Write-Host "`nException during confirm: $_" -ForegroundColor Red
    exit 1
}

# List trades
Write-Host "`n3. Listing all trades..." -ForegroundColor Yellow
$listBody = @{
    jsonrpc = "2.0"
    method = "tools/call"
    id = 3
    params = @{
        name = "list_trades"
        arguments = @{}
    }
} | ConvertTo-Json -Depth 10

$listResp = Invoke-RestMethod -Uri $uri -Method POST -Body $listBody -Headers $headers
Write-Host "List response:" -ForegroundColor Green
$listResp | ConvertTo-Json -Depth 10

# Check file
Write-Host "`n4. Checking trades.json..." -ForegroundColor Yellow
$tradesContent = Get-Content "D:\q\users\admin\db\trades.json" -Raw
Write-Host "File content:" -ForegroundColor Green
$tradesContent

Write-Host "`n=== Test Complete ===" -ForegroundColor Cyan
