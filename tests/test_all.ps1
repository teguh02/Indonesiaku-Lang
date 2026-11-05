# =================================================
#  INDONESIAKU - GOLDEN OUTPUT TEST SUITE (Windows)
#  Runs every example and asserts stdout matches
#  tests/expected/<name>.out. Exits non-zero on any
#  failure so CI can gate on it.
# =================================================

# Native stderr from indk.exe must not abort the runner.
$ErrorActionPreference = "Continue"

# Resolve repo root (parent of this script's dir)
$repo = Split-Path -Parent $PSScriptRoot
Set-Location $repo

$exe = Join-Path $repo "indk.exe"
if (-Not (Test-Path $exe)) {
    Write-Host "ERROR: indk.exe not found. Build first (make)." -ForegroundColor Red
    exit 1
}

$expectedDir = Join-Path $repo "tests\expected"
if (-Not (Test-Path $expectedDir)) {
    Write-Host "ERROR: tests\expected not found." -ForegroundColor Red
    exit 1
}

# Map example base name -> source path
$sources = @{}
foreach ($f in Get-ChildItem -Path (Join-Path $repo "examples") -Recurse -Filter *.idk) {
    $sources[$f.BaseName] = $f.FullName
}

$total = 0; $passed = 0; $failed = 0
$failures = @()

Write-Host "=================================================" -ForegroundColor Cyan
Write-Host "  INDONESIAKU - GOLDEN OUTPUT TESTS" -ForegroundColor Cyan
Write-Host "=================================================" -ForegroundColor Cyan

foreach ($exp in Get-ChildItem -Path $expectedDir -Filter *.out | Sort-Object Name) {
    $name = $exp.BaseName
    $total++
    if (-Not $sources.ContainsKey($name)) {
        Write-Host "  x $name  (no matching .idk source)" -ForegroundColor Red
        $failed++; $failures += $name; continue
    }

    $outFile = [System.IO.Path]::GetTempFileName()
    $errFile = [System.IO.Path]::GetTempFileName()
    $proc = Start-Process -FilePath $exe -ArgumentList $sources[$name] -NoNewWindow -Wait -PassThru `
        -RedirectStandardOutput $outFile -RedirectStandardError $errFile
    $code = $proc.ExitCode
    $actual = Get-Content -Raw -Path $outFile -ErrorAction SilentlyContinue
    $stderr = Get-Content -Raw -Path $errFile -ErrorAction SilentlyContinue
    Remove-Item $outFile, $errFile -Force -ErrorAction SilentlyContinue
    if ($null -eq $actual) { $actual = "" }
    $expectedText = Get-Content -Raw -Path $exp.FullName
    if ($null -eq $expectedText) { $expectedText = "" }

    # Normalize line endings for comparison
    $a = ($actual -replace "`r`n", "`n").TrimEnd("`n")
    $e = ($expectedText -replace "`r`n", "`n").TrimEnd("`n")

    if ($code -eq 0 -and $a -eq $e -and [string]::IsNullOrEmpty($stderr)) {
        Write-Host "  + $name" -ForegroundColor Green
        $passed++
    } else {
        $reason = if ($code -ne 0) { "exit=$code" }
                  elseif (-not [string]::IsNullOrEmpty($stderr)) { "stderr: $($stderr.Trim())" }
                  else { "output mismatch" }
        Write-Host "  x $name  ($reason)" -ForegroundColor Red
        $failed++; $failures += $name
    }
}

# Error tests: tests/galat/<name>.idk must exit non-zero and print exactly
# tests/galat/<name>.err to stderr (compile errors, uncatchable runtime errors).
$errorDir = Join-Path $repo "tests\galat"
if (Test-Path $errorDir) {
    Write-Host "  -- tes galat (tests/galat) --" -ForegroundColor Cyan
    foreach ($exp in Get-ChildItem -Path $errorDir -Filter *.err | Sort-Object Name) {
        $name = $exp.BaseName
        $total++
        $src = Join-Path $errorDir "$name.idk"
        if (-Not (Test-Path $src)) {
            Write-Host "  x galat/$name  (no matching .idk source)" -ForegroundColor Red
            $failed++; $failures += "galat/$name"; continue
        }

        $outFile = [System.IO.Path]::GetTempFileName()
        $errFile = [System.IO.Path]::GetTempFileName()
        # Tanpa -Wait + WaitForExit berbatas, sebuah regresi parser yang
        # membuat kompilasi MENGGANTUNG akan menghentikan seluruh suite diam-
        # diam sampai CI membunuhnya (lihat tests/galat/sarang_fungsi_anonim_hang.idk).
        $proc = Start-Process -FilePath $exe -ArgumentList $src -NoNewWindow -PassThru `
            -RedirectStandardOutput $outFile -RedirectStandardError $errFile
        $hung = $false
        if (-not $proc.WaitForExit(30000)) {
            $hung = $true
            try { $proc.Kill() } catch { }
            $proc.WaitForExit()
        }
        $code = $proc.ExitCode
        $stderr = Get-Content -Raw -Path $errFile -ErrorAction SilentlyContinue
        Remove-Item $outFile, $errFile -Force -ErrorAction SilentlyContinue
        if ($null -eq $stderr) { $stderr = "" }
        $expectedText = Get-Content -Raw -Path $exp.FullName
        if ($null -eq $expectedText) { $expectedText = "" }

        $a = ($stderr -replace "`r`n", "`n").TrimEnd("`n")
        $e = ($expectedText -replace "`r`n", "`n").TrimEnd("`n")

        if ($hung) {
            # Galatnya sudah tercetak SEBELUM parser menggantung, jadi stderr
            # cocok dan exit != 0: tanpa cabang ini tes yang menggantung malah
            # dihitung lulus.
            Write-Host "  x galat/$name  (menggantung >30 detik)" -ForegroundColor Red
            $failed++; $failures += "galat/$name"
        } elseif ($code -ne 0 -and $a -eq $e) {
            Write-Host "  + galat/$name" -ForegroundColor Green
            $passed++
        } else {
            $reason = if ($code -eq 0) { "exit=0, expected an error" } else { "stderr mismatch: $($a)" }
            Write-Host "  x galat/$name  ($reason)" -ForegroundColor Red
            $failed++; $failures += "galat/$name"
        }
    }
}

# pustaka/ harus ditemukan dari lokasi interpreter, dari direktori kerja mana pun.
$lokasi = Join-Path $PSScriptRoot "test_impor_lokasi.ps1"
if (Test-Path $lokasi) {
    Write-Host ""
    $total++
    & powershell -ExecutionPolicy Bypass -File $lokasi
    if ($LASTEXITCODE -eq 0) {
        $passed++
    } else {
        $failed++; $failures += "impor-lokasi"
    }
}

# Jalur skrip, argumen, dan nama berkas di luar code page ANSI (T-W1).
$unicode = Join-Path $PSScriptRoot "test_jalur_unicode.ps1"
if (Test-Path $unicode) {
    Write-Host ""
    $total++
    & powershell -ExecutionPolicy Bypass -File $unicode
    if ($LASTEXITCODE -eq 0) {
        $passed++
    } else {
        $failed++; $failures += "jalur-unicode"
    }
}

Write-Host "=================================================" -ForegroundColor Cyan
Write-Host "TOTAL: $passed/$total PASSED" -ForegroundColor Cyan
if ($failed -eq 0) {
    Write-Host "SUCCESS: ALL TESTS PASSING!" -ForegroundColor Green
    exit 0
} else {
    Write-Host "FAILURE: $failed test(s) failed:" -ForegroundColor Red
    $failures | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
    exit 1
}
