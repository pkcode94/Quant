# Load price data
$data = Get-Content "D:\q\docs\goldprices_series.csv" | ForEach-Object {
    $parts = $_ -split ','
    [PSCustomObject]@{
        Timestamp = [long]$parts[0]
        Price = [double]$parts[1]
    }
}

Write-Host "Loaded $($data.Count) price points"

# Calculate Moving Averages
$shortPeriod = 20
$longPeriod = 50
$signals = @()

for ($i = 0; $i -lt $data.Count; $i++) {
    $shortMA = $null
    $longMA = $null
    
    # Calculate short MA
    if ($i -ge ($shortPeriod - 1)) {
        $shortSum = 0
        for ($j = 0; $j -lt $shortPeriod; $j++) {
            $shortSum += $data[$i - $j].Price
        }
        $shortMA = $shortSum / $shortPeriod
    }
    
    # Calculate long MA
    if ($i -ge ($longPeriod - 1)) {
        $longSum = 0
        for ($j = 0; $j -lt $longPeriod; $j++) {
            $longSum += $data[$i - $j].Price
        }
        $longMA = $longSum / $longPeriod
    }
    
    # Detect crossovers
    if ($shortMA -and $longMA -and $i -gt 0) {
        $prevShortSum = 0
        $prevLongSum = 0
        
        for ($j = 0; $j -lt $shortPeriod; $j++) {
            $prevShortSum += $data[$i - 1 - $j].Price
        }
        $prevShortMA = $prevShortSum / $shortPeriod
        
        for ($j = 0; $j -lt $longPeriod; $j++) {
            $prevLongSum += $data[$i - 1 - $j].Price
        }
        $prevLongMA = $prevLongSum / $longPeriod
        
        # Golden Cross - Buy Signal
        if ($prevShortMA -le $prevLongMA -and $shortMA -gt $longMA) {
            $signals += [PSCustomObject]@{
                Index = $i
                Timestamp = $data[$i].Timestamp
                Price = $data[$i].Price
                Signal = "BUY"
                ShortMA = [math]::Round($shortMA, 2)
                LongMA = [math]::Round($longMA, 2)
            }
        }
        
        # Death Cross - Sell Signal
        if ($prevShortMA -ge $prevLongMA -and $shortMA -lt $longMA) {
            $signals += [PSCustomObject]@{
                Index = $i
                Timestamp = $data[$i].Timestamp
                Price = $data[$i].Price
                Signal = "SELL"
                ShortMA = [math]::Round($shortMA, 2)
                LongMA = [math]::Round($longMA, 2)
            }
        }
    }
}

Write-Host "`nGenerated $($signals.Count) trading signals:"
$signals | Format-Table -AutoSize

# Export signals for execution
$signals | ConvertTo-Json | Out-File "D:\q\trade_signals.json"
Write-Host "`nSignals exported to trade_signals.json"
