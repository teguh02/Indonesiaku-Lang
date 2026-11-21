# Manajer paket `indk paket` (Fase 14), padanan tests/test_paket.sh untuk
# Windows. Logika ujinya ada di tests/paket_cli.idk.
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$bin = Join-Path $repo "indk.exe"
if (-not (Test-Path $bin)) {
    Write-Host "indk.exe belum dibangun." -ForegroundColor Red
    exit 1
}
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Write-Host "PAKET: dilewati (git tidak terpasang)"
    exit 0
}
$t = Join-Path ([System.IO.Path]::GetTempPath()) ("indk-paket-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $t | Out-Null
try {
    Push-Location $repo
    & $bin "tests/paket_cli.idk" $t
    $kode = $LASTEXITCODE
} finally {
    Pop-Location
    Remove-Item -Recurse -Force $t -ErrorAction SilentlyContinue
}
exit $kode
