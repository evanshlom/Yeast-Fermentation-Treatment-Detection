<#
.SYNOPSIS
    Builds the training Docker image and runs train_model.py on the
    given CSV file(s), all in one step.

.PARAMETER Csvs
    One or more CSV file paths or glob patterns, relative to the
    project root. Defaults to "ml/train_data.csv".

.PARAMETER Tail
    Passed through to train_model.py's --tail flag (default 300).

.EXAMPLE
    .\ml\train.ps1
    .\ml\train.ps1 -Csvs "ml/*.csv" -Tail 200

.NOTES
    Can be run from anywhere — it finds the project root itself
    (the parent of this script's own folder) and does the Docker
    build/run from there, since that's what the volume mount and
    Dockerfile paths assume.
#>

param(
    [string[]]$Csvs = @("ml/train_data.csv"),

    [int]$Tail = 300
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path $PSScriptRoot -Parent
Push-Location $projectRoot

try {
    Write-Host "Building training image..." -ForegroundColor Cyan
    docker build -t gas-ml -f ml/Dockerfile ml
    if ($LASTEXITCODE -ne 0) { throw "docker build failed" }

    Write-Host "`nRunning training on: $($Csvs -join ', ')" -ForegroundColor Cyan
    docker run --rm -v "${projectRoot}:/data" -w /data gas-ml `
        python ml/train_model.py @Csvs --tail $Tail
    if ($LASTEXITCODE -ne 0) { throw "training failed" }

    Write-Host "`nDone. ml_model.c / ml_model.h have been overwritten in $projectRoot" -ForegroundColor Green
    Write-Host "Next: make clean; make test; .\flash_and_monitor.ps1 -BinName gas_monitor_test.bin" -ForegroundColor Green
}
finally {
    Pop-Location
}