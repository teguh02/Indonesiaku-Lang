# Jalur skrip, argumen, INDK_PATH, dan nama berkas di luar code page ANSI
# (T-W1). Padanan Windows untuk tests/test_jalur_unicode.sh.
#
# indk 1.4.0 menerima argv ANSI dan membuka berkas dengan fopen(), sehingga
# skrip di folder bernama Jepang/emoji gagal dibuka ("??/s.idk", kode 74).
#
# Berkas ini sengaja hanya ASCII: PowerShell 5.1 membaca .ps1 tanpa BOM
# sebagai ANSI, jadi nama non-ASCII dirakit dengan [char]. Program .idk juga
# hanya mencetak ASCII, supaya hasilnya tidak bergantung pada cara
# PowerShell mendekode keluaran program native.

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$bin = Join-Path $repo "indk.exe"
if (-not (Test-Path $bin)) {
    Write-Host "indk.exe belum dibangun." -ForegroundColor Red
    exit 1
}

$jepang = [string][char]0x65E5 + [char]0x672C          # "Nihon"
$kopi = [string][char]0x2615                            # hot beverage
$aUmlaut = [string][char]0x00E4
$i = [string][char]0x00EF

$t = Join-Path ([System.IO.Path]::GetTempPath()) ("indk-unicode-" + [System.Guid]::NewGuid().ToString("N"))
$dir = Join-Path $t ($jepang + " " + $kopi)
$jauh = Join-Path $t ("pustaka na" + $i + "ve")
New-Item -ItemType Directory -Force -Path $dir | Out-Null
New-Item -ItemType Directory -Force -Path $jauh | Out-Null

$utf8 = New-Object System.Text.UTF8Encoding $false
$program = @'
impor lokal
impor jauh
cetak(lokal.NILAI, jauh.NILAI)
cetak(argumen == ["a" + chr(0xe4) + " " + chr(0x2615)])
nama = "keluaran_" + chr(0x2615) + ".txt"
tulis_semua(nama, "x")
cetak(nama dalam isi_direktori("."), baca_semua(nama))
hapus_berkas(nama)
cetak(ada_berkas(nama))
'@
[System.IO.File]::WriteAllText((Join-Path $dir "s.idk"), $program, $utf8)
[System.IO.File]::WriteAllText((Join-Path $dir "lokal.idk"), "NILAI = `"lokal`"`n", $utf8)
[System.IO.File]::WriteAllText((Join-Path $jauh "jauh.idk"), "NILAI = `"jauh`"`n", $utf8)
$harap = "lokal jauh`nbenar`nbenar x`nsalah"
$argumen = "a" + $aUmlaut + " " + $kopi

$simpanPath = $env:INDK_PATH
$env:INDK_PATH = $jauh
$gagal = 0

function Uji($nama, $kerja, $skrip) {
    # PowerShell 5.1 turns a native program's stderr into an error record;
    # with "Stop" a failing run would abort instead of being reported.
    $ErrorActionPreference = "Continue"
    Push-Location $kerja
    try {
        $keluar = & $bin $skrip $argumen 2>&1 | Out-String
    } finally {
        Pop-Location
    }
    $keluar = ($keluar -replace "`r`n", "`n").TrimEnd("`n")
    if ($LASTEXITCODE -eq 0 -and $keluar -eq $harap) {
        Write-Host "  + $nama" -ForegroundColor Green
        return 0
    }
    Write-Host "  x $nama (kode $LASTEXITCODE): $keluar" -ForegroundColor Red
    return 1
}

Write-Host "Jalur Unicode (skrip, modul, argumen, INDK_PATH)"
try {
    $gagal += Uji "jalur absolut" $dir (Join-Path $dir "s.idk")
    $gagal += Uji "jalur relatif" $t (Join-Path ($jepang + " " + $kopi) "s.idk")
} finally {
    $env:INDK_PATH = $simpanPath
    Remove-Item -Recurse -Force -Path $t -ErrorAction SilentlyContinue
}

if ($gagal -eq 0) {
    Write-Host "JALUR UNICODE: semua berjalan" -ForegroundColor Green
    exit 0
}
Write-Host "JALUR UNICODE: $gagal gagal" -ForegroundColor Red
exit 1
