# convert_goldprices.ps1
#
# Converts docs/goldprices.csv from YYYY-MM,price format
# to unix_timestamp,price format for use with PriceSeries/Simulator.
#
# Usage:
#   cd D:\Quant
#   .\convert_goldprices.ps1
#
# Output: docs/goldprices_series.csv

$ErrorActionPreference = "Stop"

$inputFile  = Join-Path $PSScriptRoot "docs\goldprices.csv"
$outputFile = Join-Path $PSScriptRoot "docs\goldprices_series.csv"

$epoch = [DateTimeOffset]::new([DateTime]::new(1970, 1, 1, 0, 0, 0, [DateTimeKind]::Utc))

$lines = [System.IO.File]::ReadAllLines($inputFile)
$sb = [System.Text.StringBuilder]::new($lines.Count * 24)

$count = 0
foreach ($line in $lines) {
    if ([string]::IsNullOrWhiteSpace($line)) { continue }
    $parts = $line.Split(',')
    if ($parts.Count -lt 2) { continue }

    $ym    = $parts[0].Trim()
    $price = $parts[1].Trim()
    $dp    = $ym.Split('-')
    if ($dp.Count -ne 2) { continue }

    $y = [int]$dp[0]
    $m = [int]$dp[1]

    $dt = [DateTimeOffset]::new([DateTime]::new($y, $m, 1, 0, 0, 0, [DateTimeKind]::Utc))
    $ts = [long]($dt.ToUnixTimeSeconds())

    [void]$sb.AppendLine("$ts,$price")
    $count++
}

[System.IO.File]::WriteAllText($outputFile, $sb.ToString().TrimEnd())

Write-Host "Wrote $count rows to $outputFile"
Write-Host "First: $($lines[0].Trim()) -> check file"
Write-Host "Last:  $($lines[$lines.Count - 1].Trim()) -> check file"
