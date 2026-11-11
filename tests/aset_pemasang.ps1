# Memeriksa aset ikon pemasang Windows.
#
# Padanan PowerShell dari tests/aset_pemasang.sh, karena harness Windows
# (test_all.ps1) tidak memanggil satu pun sub-suite .sh: tanpa berkas ini,
# ikon pemasang justru tidak teruji di OS yang memakainya.
#
# Dibaca langsung dari byte, tanpa dependensi gambar apa pun.

$ErrorActionPreference = "Stop"
$paket = Join-Path (Split-Path -Parent $PSScriptRoot) "packaging\windows"
$iss = Join-Path $paket "indonesiaku.iss"
$gagal = 0

function Lulus($p) { Write-Host "  ok   $p" -ForegroundColor Green }
function Tolak($p) { Write-Host "  GAGAL $p" -ForegroundColor Red; $script:gagal = 1 }

Write-Host "Aset pemasang Windows:"

foreach ($b in @("indonesiaku.ico", "wizard-kecil-55.bmp", "wizard-kecil-83.bmp",
                 "wizard-kecil-110.bmp", "wizard-kecil-138.bmp")) {
    if (Test-Path (Join-Path $paket $b)) { Lulus "ada $b" } else { Tolak "hilang $b" }
}

$isi = Get-Content $iss -Raw
foreach ($kunci in @("SetupIconFile=indonesiaku.ico",
                     "WizardSmallImageFile=wizard-kecil-55.bmp",
                     "UninstallDisplayIcon={app}\indonesiaku.ico")) {
    if ($isi.Contains($kunci)) { Lulus "iss memuat $kunci" }
    else { Tolak "iss tidak memuat $kunci" }
}

# T-W13: uninstall harus membuang entri PATH-nya sendiri. Perilakunya diuji
# sungguhan oleh tests\pemasang_path.ps1 (butuh iscc + admin); ini penjaga
# murah yang juga berjalan tanpa keduanya.
foreach ($kunci in @("ChangesEnvironment=yes", "procedure CurUninstallStepChanged",
                     "HapusDariPath(PathLama, ExpandConstant('{app}'))")) {
    if ($isi.Contains($kunci)) { Lulus "iss memuat $kunci" }
    else { Tolak "iss tidak memuat $kunci" }
}

if ($isi -match 'Source: "indonesiaku\.ico"; DestDir: "\{app\}"') {
    Lulus "iss memasang indonesiaku.ico ke {app}"
} else {
    Tolak "iss tidak memasang indonesiaku.ico ke {app}"
}

# --- struktur ICO ---
$d = [IO.File]::ReadAllBytes((Join-Path $paket "indonesiaku.ico"))
$res = [BitConverter]::ToUInt16($d, 0)
$typ = [BitConverter]::ToUInt16($d, 2)
$n = [BitConverter]::ToUInt16($d, 4)
if ($res -ne 0 -or $typ -ne 1) { Tolak "header ICO tidak sah: reserved=$res type=$typ" }
else { Lulus "header ICO sah" }

$png = [byte[]](0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A)
$ada = @{}
for ($i = 0; $i -lt $n; $i++) {
    $e = 6 + 16 * $i
    # [int] wajib: $d[$e] bertipe byte, dan kunci hashtable bertipe byte
    # tidak cocok dengan ContainsKey(16) yang bertipe int -- pemeriksaan
    # ukuran diam-diam selalu gagal tanpa cast ini.
    $w = [int]$d[$e]; if ($w -eq 0) { $w = 256 }
    $bpp = [BitConverter]::ToUInt16($d, $e + 6)
    $size = [BitConverter]::ToUInt32($d, $e + 8)
    $off = [BitConverter]::ToUInt32($d, $e + 12)

    if ($off + $size -gt $d.Length) { Tolak "entri ${w}x${w} menunjuk ke luar berkas"; continue }

    $adalahPng = $true
    for ($j = 0; $j -lt 8; $j++) { if ($d[$off + $j] -ne $png[$j]) { $adalahPng = $false; break } }

    # Windows hanya menjamin PNG pada 256x256.
    if ($adalahPng -and $w -lt 256) { Tolak "entri ${w}x${w} tersimpan PNG; ukuran <256 harus DIB" }
    $ada[$w] = @{ fmt = $(if ($adalahPng) { "PNG" } else { "DIB" }); bpp = $bpp }
}

foreach ($perlu in @(16, 32, 48, 256)) {
    if ($ada.ContainsKey($perlu)) {
        Lulus "ICO punya ${perlu}x${perlu} ($($ada[$perlu].fmt), $($ada[$perlu].bpp) bpp)"
    } else {
        Tolak "ICO tidak punya ukuran ${perlu}x${perlu}"
    }
}

# --- struktur BMP ---
foreach ($s in @(55, 83, 110, 138)) {
    $b = [IO.File]::ReadAllBytes((Join-Path $paket "wizard-kecil-$s.bmp"))
    if ($b[0] -ne 0x42 -or $b[1] -ne 0x4D) { Tolak "wizard-kecil-$s.bmp bukan BMP"; continue }
    $w = [BitConverter]::ToInt32($b, 18)
    $h = [BitConverter]::ToInt32($b, 22)
    $bpp = [BitConverter]::ToUInt16($b, 28)
    $comp = [BitConverter]::ToUInt32($b, 30)
    if ($w -ne $s -or $h -ne $s) { Tolak "wizard-kecil-$s.bmp berukuran ${w}x${h}, seharusnya ${s}x${s}" }
    elseif ($comp -ne 0) { Tolak "wizard-kecil-$s.bmp terkompresi (comp=$comp); Inno menolaknya" }
    elseif ($bpp -ne 24) { Tolak "wizard-kecil-$s.bmp $bpp bpp, seharusnya 24" }
    else { Lulus "wizard-kecil-$s.bmp ${w}x${h} 24 bpp tanpa kompresi" }
}

if ($gagal -eq 0) {
    Write-Host "SEMUA UJI ASET PEMASANG LULUS" -ForegroundColor Green
    exit 0
} else {
    Write-Host "UJI ASET PEMASANG GAGAL" -ForegroundColor Red
    exit 1
}
