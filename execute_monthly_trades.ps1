# Monthly Trading Strategy with $500 Pump per Month
# Extracts monthly prices from goldprices_series.csv and executes chain trades

$csv = Get-Content "D:\q\docs\goldprices_series.csv"
$prices = @()

foreach ($line in $csv) {
    if ($line -match '^(-?\d+),(\d+\.?\d*)$') {
        $timestamp = [long]$Matches[1]
        $price = [double]$Matches[2]
        $prices += @{
            Timestamp = $timestamp
            Price = $price
            Date = (Get-Date "1970-01-01").AddSeconds($timestamp)
        }
    }
}

Write-Host "`n=== Monthly Gold Trading Strategy ===" -ForegroundColor Cyan
Write-Host "Total data points: $($prices.Count)" -ForegroundColor White
Write-Host "Date range: $($prices[0].Date.ToString('yyyy-MM')) to $($prices[-1].Date.ToString('yyyy-MM'))" -ForegroundColor White
Write-Host "Monthly pump: `$500" -ForegroundColor Yellow

# Sample monthly (take one price per ~30 days)
$monthlyPrices = @()
$lastTimestamp = $prices[0].Timestamp
$monthlyPrices += $prices[0]

foreach ($p in $prices) {
    $daysDiff = ($p.Timestamp - $lastTimestamp) / 86400
    if ($daysDiff -ge 30) {
        $monthlyPrices += $p
        $lastTimestamp = $p.Timestamp
    }
}

Write-Host "`nMonthly samples extracted: $($monthlyPrices.Count)" -ForegroundColor Green
Write-Host "Period per trade: ~30 days" -ForegroundColor White

# Calculate total duration
$totalYears = ($monthlyPrices[-1].Timestamp - $monthlyPrices[0].Timestamp) / (365.25 * 86400)
Write-Host "Total duration: $([math]::Round($totalYears, 1)) years" -ForegroundColor White

# Show first 10 monthly prices
Write-Host "`n=== First 10 Monthly Prices ===" -ForegroundColor Cyan
for ($i = 0; $i -lt [Math]::Min(10, $monthlyPrices.Count); $i++) {
    $p = $monthlyPrices[$i]
    Write-Host ("{0}. {1:yyyy-MM-dd}: `${2:N2}" -f ($i+1), $p.Date, $p.Price) -ForegroundColor Gray
}

Write-Host "`n=== Trading Parameters ===" -ForegroundColor Cyan
Write-Host "Strategy: Chain (4 levels, sigmoid-distributed)" -ForegroundColor White
Write-Host "Capital per month: `$500 (cumulative deployment)" -ForegroundColor White
Write-Host "Stop Losses: DISABLED (per equations.md)" -ForegroundColor White
Write-Host "Fee Hedging: 2x" -ForegroundColor White
Write-Host "Total months to execute: $($monthlyPrices.Count)" -ForegroundColor Yellow

Write-Host "`nExporting monthly prices to monthly_gold_prices.json..." -ForegroundColor Gray
$monthlyData = $monthlyPrices | ForEach-Object {
    @{
        Timestamp = $_.Timestamp
        Price = $_.Price
        Date = $_.Date.ToString('yyyy-MM-dd')
    }
}
$monthlyData | ConvertTo-Json | Out-File "D:\q\monthly_gold_prices.json" -Encoding utf8

Write-Host "Done! Use MCP tools to execute $($monthlyPrices.Count) monthly trades." -ForegroundColor Green
Write-Host "`nExpected results:" -ForegroundColor Cyan
Write-Host "  - Total capital injected: `$$($monthlyPrices.Count * 500)" -ForegroundColor White
Write-Host "  - Starting balance: `$100,000" -ForegroundColor White
Write-Host "  - Final balance: TBD after execution" -ForegroundColor White
