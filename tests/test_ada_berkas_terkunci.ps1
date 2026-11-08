# ada_berkas() untuk berkas yang sedang dikunci proses lain (T-W9).
#
# indk 1.4.0 - 1.6.0-rc.1 menjawab ada_berkas() dengan mencoba fopen(path,
# "rb"). Berkas yang dibuka proses lain dengan FileShare None membuat fopen
# gagal (sharing violation), sehingga berkas yang jelas ada dijawab `salah`,
# sementara jalur.ada() -- yang memakai GetFileAttributesExW -- menjawab
# `benar`. Kondisi ini khas Windows: editor, antivirus, dan database (SQLite
# dalam transaksi) mengunci berkas seperti ini.
#
# Berkas ini sengaja hanya ASCII: PowerShell 5.1 membaca .ps1 tanpa BOM
# sebagai ANSI.

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$bin = Join-Path $repo "indk.exe"
if (-not (Test-Path $bin)) {
    Write-Host "indk.exe belum dibangun." -ForegroundColor Red
    exit 1
}

Write-Host "ada_berkas() untuk berkas terkunci (T-W9)"

$t = Join-Path ([System.IO.Path]::GetTempPath()) ("indk-kunci-" + [System.Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $t | Out-Null
$utf8 = New-Object System.Text.UTF8Encoding $false
$kunci = Join-Path $t "kunci.txt"
[System.IO.File]::WriteAllText($kunci, "isi", $utf8)

# Jalur dikirim lewat argumen, bukan ditulis ke dalam program, supaya
# backslash Windows tidak perlu di-escape di literal teks.
$program = @'
impor jalur
p = argumen[0]
cetak(ada_berkas(p), jalur.ada(p), jalur.adalah_berkas(p))
'@
$skrip = Join-Path $t "cek.idk"
[System.IO.File]::WriteAllText($skrip, $program, $utf8)

$gagal = 0

function Uji($nama, $harap) {
    # PowerShell 5.1 turns a native program's stderr into an error record;
    # with "Stop" a failing run would abort instead of being reported.
    $ErrorActionPreference = "Continue"
    $keluar = & $bin $skrip $kunci 2>&1 | Out-String
    $keluar = ($keluar -replace "`r`n", "`n").TrimEnd("`n")
    if ($LASTEXITCODE -eq 0 -and $keluar -eq $harap) {
        Write-Host "  + $nama" -ForegroundColor Green
        return 0
    }
    Write-Host "  x $nama (kode $LASTEXITCODE): '$keluar', diharapkan '$harap'" -ForegroundColor Red
    return 1
}

try {
    $gagal += Uji "tanpa kunci" "benar benar benar"

    # Kunci baca-saja: proses lain boleh tidak membuka sama sekali.
    $fs = [System.IO.File]::Open($kunci, 'Open', 'Read', 'None')
    try {
        $gagal += Uji "dikunci FileShare None (baca)" "benar benar benar"
    } finally {
        $fs.Close()
    }

    # Kunci tulis, seperti yang dilaporkan di uji ekstensif 1.4.0.
    $fs = [System.IO.File]::Open($kunci, 'Open', 'ReadWrite', 'None')
    try {
        $gagal += Uji "dikunci FileShare None (baca-tulis)" "benar benar benar"
    } finally {
        $fs.Close()
    }

    Remove-Item -Force $kunci
    $gagal += Uji "setelah dihapus" "salah salah salah"
} finally {
    Remove-Item -Recurse -Force $t -ErrorAction SilentlyContinue
}

if ($gagal -eq 0) {
    Write-Host "BERKAS TERKUNCI: semua berjalan" -ForegroundColor Green
    exit 0
}
Write-Host "BERKAS TERKUNCI: $gagal gagal" -ForegroundColor Red
exit 1
