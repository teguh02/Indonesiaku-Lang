# arsip.ekstrak() tidak pernah menulis MELALUI junction/tautan yang sudah ada
# di bawah direktori tujuan (Fase 23). Padanan POSIX: test_arsip_tautan.sh.
# Junction tidak memerlukan hak administrator, jadi selalu bisa diuji.

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$indk = Join-Path $repo "indk.exe"
if (-not (Test-Path $indk)) { Write-Host "indk.exe tidak ditemukan; jalankan make dulu"; exit 1 }

$tmp = Join-Path ([System.IO.Path]::GetTempPath()) ("arsip_tautan_" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory $tmp | Out-Null
$lulus = 0; $gagal = 0
function Lulus($m) { Write-Host "LULUS $m" -ForegroundColor Green; $script:lulus++ }
function Gagal($m) { Write-Host "GAGAL $m" -ForegroundColor Red; $script:gagal++ }

try {
    Push-Location $tmp
    Set-Content buat.idk -Encoding ascii -Value @'
impor arsip
arsip.buat("a.tar.gz", [["dir/a.txt", "isi a"], ["z.txt", "isi z"]])
'@
    Set-Content ekstrak.idk -Encoding ascii -Value @'
impor arsip
dari arsip impor GalatArsip
coba {
    arsip.ekstrak(argumen[0], argumen[1])
    cetak("DIEKSTRAK")
} kecuali GalatArsip sebagai e {
    cetak(e.pesan)
}
'@
    & $indk buat.idk
    if ($LASTEXITCODE -ne 0) { throw "gagal membuat arsip uji" }

    # 1. Direktori perantara berupa junction.
    New-Item -ItemType Directory luar1, tujuan1 | Out-Null
    New-Item -ItemType Junction -Path tujuan1\dir -Target (Join-Path $tmp "luar1") | Out-Null
    $keluaran = (& $indk ekstrak.idk a.tar.gz tujuan1) -join "`n"
    $isiLuar = @(Get-ChildItem luar1).Count
    if ($keluaran -like "*'dir' di tujuan adalah tautan; tidak diikuti*" -and $isiLuar -eq 0 -and -not (Test-Path tujuan1\z.txt)) {
        Lulus "junction di tujuan ditolak sebelum menulis"
    } else {
        Gagal "junction: '$keluaran'; isi luar1=$isiLuar"
    }

    # 2. Tujuan itu sendiri junction ke direktori: diizinkan.
    New-Item -ItemType Directory nyata | Out-Null
    New-Item -ItemType Junction -Path tujuan2 -Target (Join-Path $tmp "nyata") | Out-Null
    $keluaran = (& $indk ekstrak.idk a.tar.gz tujuan2) -join "`n"
    if ($keluaran -eq "DIEKSTRAK" -and (Get-Content nyata\dir\a.txt -Raw) -eq "isi a") {
        Lulus "tujuan yang berupa junction tetap bisa dipakai"
    } else {
        Gagal "tujuan junction: '$keluaran'"
    }
} finally {
    Pop-Location
    # Hapus junction dulu agar Remove-Item tidak menyeberang ke targetnya.
    Get-ChildItem $tmp -Recurse -Attributes ReparsePoint -ErrorAction SilentlyContinue |
        ForEach-Object { [System.IO.Directory]::Delete($_.FullName) }
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
}

Write-Host ""
Write-Host "arsip-tautan: $lulus lulus, $gagal gagal"
if ($gagal -ne 0) { exit 1 }
exit 0
