<#
.SYNOPSIS
    Flashes a chosen .bin to a connected STM32 Nucleo board and opens
    a live serial monitor over the ST-Link Virtual COM Port.

.PARAMETER BinName
    Which binary to flash. Defaults to gas_monitor.bin (training-data
    collection firmware). Pass -BinName gas_monitor_test.bin to flash
    the ML-classification test firmware instead — no need to edit
    this script or rename files to switch between the two.

.EXAMPLE
    .\flash_and_monitor.ps1
    .\flash_and_monitor.ps1 -BinName gas_monitor_test.bin
#>

param(
    [string]$BinName = "gas_monitor.bin"
)

$ErrorActionPreference = "Stop"

$binFile = Join-Path $PSScriptRoot $BinName
$baudRate = 115200

if (-not (Test-Path $binFile)) {
    Write-Error "$BinName not found in $PSScriptRoot. Run 'make' (or 'make test') first."
    exit 1
}

# ── Step 1: Find the Nucleo's mass-storage drive ────────────────────────
Write-Host "Looking for Nucleo drive..." -ForegroundColor Cyan

$nucleoDrive = Get-Volume | Where-Object {
    $_.FileSystemLabel -match "^(NODE?_|DIS_)"
} | Select-Object -First 1

if (-not $nucleoDrive) {
    Write-Error "No Nucleo drive found (expected a volume labeled like 'NODE_F446RE' or 'DIS_F446RE'). Is the board plugged in?"
    exit 1
}

$driveLetter = $nucleoDrive.DriveLetter
Write-Host "Found Nucleo drive: $driveLetter`: ($($nucleoDrive.FileSystemLabel))" -ForegroundColor Green

# ── Step 2: Flash by copying the .bin onto the drive ────────────────────
Write-Host "Flashing $BinName..." -ForegroundColor Cyan
Copy-Item -Path $binFile -Destination "$driveLetter`:\" -Force

# Give the board a moment to program and auto-reset
Start-Sleep -Seconds 3
Write-Host "Flash complete." -ForegroundColor Green

# ── Step 3: Find the ST-Link Virtual COM Port ────────────────────────────
Write-Host "Looking for ST-Link COM port..." -ForegroundColor Cyan

$comDevice = Get-CimInstance -ClassName Win32_PnPEntity |
    Where-Object { $_.Name -match "STMicroelectronics STLink Virtual COM Port \((COM\d+)\)" } |
    Select-Object -First 1

if (-not $comDevice) {
    Write-Error "No ST-Link Virtual COM Port found. Check Device Manager manually."
    exit 1
}

if ($comDevice.Name -match "\((COM\d+)\)") {
    $comPort = $Matches[1]
} else {
    Write-Error "Found the ST-Link device but couldn't parse its COM port name: $($comDevice.Name)"
    exit 1
}

Write-Host "Found COM port: $comPort" -ForegroundColor Green

# ── Step 4: Open serial monitor ──────────────────────────────────────────
Write-Host "`nOpening serial monitor on $comPort @ $baudRate baud. Press Ctrl+C to stop.`n" -ForegroundColor Cyan

$port = New-Object System.IO.Ports.SerialPort $comPort, $baudRate, "None", 8, "One"
$port.ReadTimeout = 3000

# ── Step 5: Set up CSV logging ────────────────────────────────────────────
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$csvFile = Join-Path $PSScriptRoot "gas_log_$timestamp.csv"
$csvHeader = "Sample,CO2raw,CO2_ADCv,CO2_AOUTv,ALCraw,ALC_ADCv,ALC_AOUTv"
Set-Content -Path $csvFile -Value $csvHeader

# Matches a data row like:
# 121      | 386      0.311    0.452    | 1044     0.841    1.223
$rowPattern = '^\s*(\d+)\s*\|\s*(\d+)\s+([\d.]+)\s+([\d.]+)\s*\|\s*(\d+)\s+([\d.]+)\s+([\d.]+)\s*$'

Write-Host "Logging to $csvFile`n" -ForegroundColor Cyan

try {
    $port.Open()
    while ($true) {
        try {
            $line = $port.ReadLine()
            Write-Host $line

            if ($line -match $rowPattern) {
                $csvLine = "$($Matches[1]),$($Matches[2]),$($Matches[3]),$($Matches[4]),$($Matches[5]),$($Matches[6]),$($Matches[7])"
                Add-Content -Path $csvFile -Value $csvLine
            }
        } catch [System.TimeoutException] {
            # no data yet, keep waiting
        }
    }
}
finally {
    if ($port.IsOpen) {
        $port.Close()
    }
    Write-Host "`nSerial monitor closed." -ForegroundColor Yellow
    Write-Host "CSV log saved to $csvFile" -ForegroundColor Yellow
}