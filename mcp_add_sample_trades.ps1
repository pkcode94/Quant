$ErrorActionPreference = "Stop"

$uri = "http://localhost:8080/mcp"
$headers = @{ "Content-Type" = "application/json" }

function Invoke-McpTool([string]$name, $arguments, [int]$id) {
    $body = @{
        jsonrpc = "2.0"
        method = "tools/call"
        id = $id
        params = @{
            name = $name
            arguments = $arguments
        }
    } | ConvertTo-Json -Depth 20

    $resp = Invoke-RestMethod -Uri $uri -Method POST -Body $body -Headers $headers
    if (-not $resp.result.content -or $resp.result.content.Count -lt 1) {
        throw "No MCP content for tool '$name'"
    }
    return ($resp.result.content[0].text | ConvertFrom-Json)
}

Write-Host "[1/4] Creating sample trades via MCP" -ForegroundColor Cyan
$orders = @(
    @{ symbol = "GOLD";   price = 2100.0; quantity = 0.05 },
    @{ symbol = "SILVER"; price = 27.5;   quantity = 4.0  },
    @{ symbol = "PLATIN"; price = 980.0;  quantity = 0.2  }
)

$confirmed = @()
$reqId = 1
foreach ($o in $orders) {
    $exec = Invoke-McpTool -name "execute_buy" -arguments $o -id $reqId
    $reqId += 1

    if ($exec.error) {
        throw "execute_buy failed: $($exec.error)"
    }
    if (-not $exec.token) {
        throw "execute_buy returned no token"
    }

    $confirm = Invoke-McpTool -name "confirm_execution" -arguments @{ token = $exec.token } -id $reqId
    $reqId += 1

    if ($confirm.error) {
        throw "confirm_execution failed: $($confirm.error)"
    }

    $confirmed += [PSCustomObject]@{
        symbol = $o.symbol
        price = $o.price
        quantity = $o.quantity
        tradeId = $confirm.tradeId
    }
}

Write-Host "[2/4] Confirmed trades:" -ForegroundColor Green
$confirmed | Format-Table -AutoSize | Out-String | Write-Host

Write-Host "[3/4] Reading MCP list_trades" -ForegroundColor Cyan
$list = Invoke-McpTool -name "list_trades" -arguments @{} -id $reqId
$reqId += 1

if ($list -is [System.Array]) {
    Write-Host ("MCP list_trades count: " + $list.Count) -ForegroundColor Green
} else {
    Write-Host "MCP list_trades returned non-array payload:" -ForegroundColor Yellow
    $list | ConvertTo-Json -Depth 10 | Write-Host
}

Write-Host "[4/4] Checking on-disk database files" -ForegroundColor Cyan
$adminPath = "D:\q\users\admin\db\trades.json"
$rootPath = "D:\q\db\trades.json"

$adminTrades = @()
if (Test-Path $adminPath) {
    $adminTrades = (Get-Content $adminPath -Raw | ConvertFrom-Json)
}
$rootTrades = @()
if (Test-Path $rootPath) {
    $rootTrades = (Get-Content $rootPath -Raw | ConvertFrom-Json)
}

Write-Host ("users/admin/db/trades.json count: " + $adminTrades.Count) -ForegroundColor Green
Write-Host ("db/trades.json count: " + $rootTrades.Count) -ForegroundColor Yellow

Write-Host "Recent entries in users/admin/db/trades.json:" -ForegroundColor Green
$adminTrades | Select-Object -Last 6 | Select-Object tradeId, symbol, value, quantity, timestamp | Format-Table -AutoSize | Out-String | Write-Host
