# Pemasang Windows: uninstall membuang entri PATH-nya sendiri (T-W13).
#
# Uji sungguhan, bukan pemeriksaan teks: mengompilasi
# packaging\windows\indonesiaku.iss dengan iscc, memasangnya secara senyap
# dengan tugas tambahpath, lalu meng-uninstall-nya dan membandingkan nilai
# Path HKLM dengan nilai sebelum pemasangan -- harus sama persis.
#
# Aman dijalankan di mesin pengembang: pemasang uji memakai AppId sendiri
# (/DAppIdApp), folder sementara, dan /NOICONS, jadi tidak pernah menimpa
# atau meng-uninstall Indonesiaku milik pengguna dan tidak menyentuh Start
# Menu. Bila uji gagal di tengah jalan, blok finally mengembalikan Path dan
# meng-uninstall pemasang uji.
#
# Butuh hak admin (PATH sistem) dan Inno Setup 6. Runner windows-latest
# punya keduanya. Di mesin tanpa admin, jalankan dengan -IzinkanUac: setiap
# pasang/uninstall akan meminta UAC.
#
# Berkas ini sengaja hanya ASCII: PowerShell 5.1 membaca .ps1 tanpa BOM
# sebagai ANSI.

param([switch]$IzinkanUac)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$iss = Join-Path $repo "packaging\windows\indonesiaku.iss"
$kunci = "HKLM:\SYSTEM\CurrentControlSet\Control\Session Manager\Environment"

Write-Host "Pemasang: uninstall membuang entri PATH (T-W13)"

$iscc = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
if (-not (Test-Path $iscc)) {
    Write-Host "  LEWAT: Inno Setup 6 tidak terpasang"
    exit 0
}
if (-not (Test-Path (Join-Path $repo "indk.exe"))) {
    Write-Host "  GAGAL: indk.exe belum dibangun di akar repo" -ForegroundColor Red
    exit 1
}
$admin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $admin -and -not $IzinkanUac) {
    Write-Host "  LEWAT: butuh hak admin (atau -IzinkanUac)"
    exit 0
}

# Path dibaca mentah (tanpa ekspansi %VAR%), supaya pembandingan benar-benar
# per byte dengan apa yang ada di registri.
function BacaPath {
    $k = Get-Item $kunci
    return $k.GetValue("Path", "", [Microsoft.Win32.RegistryValueOptions]::DoNotExpandEnvironmentNames)
}
function TulisPath($nilai) {
    $k = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey(
        "SYSTEM\CurrentControlSet\Control\Session Manager\Environment", $true)
    $k.SetValue("Path", $nilai, [Microsoft.Win32.RegistryValueKind]::ExpandString)
    $k.Close()
}
function Jalankan($exe, $argumen) {
    $p = Start-Process -FilePath $exe -ArgumentList $argumen -Wait -PassThru
    return $p.ExitCode
}

$gagal = 0
function Lulus($p) { Write-Host "  ok   $p" -ForegroundColor Green }
function Tolak($p) { Write-Host "  GAGAL $p" -ForegroundColor Red; $script:gagal = 1 }

$tmp = Join-Path ([IO.Path]::GetTempPath()) ("indk-pemasang-" + [Guid]::NewGuid().ToString("N"))
$keluar = Join-Path $tmp "keluaran"
$app = Join-Path $tmp "Indonesiaku Uji"
New-Item -ItemType Directory -Force $keluar | Out-Null
$pathAwal = BacaPath

try {
    & $iscc "/Q" "/DVersiApp=0.0.0-uji" "/DAppIdApp=IndonesiakuUjiPathT-W13" "/O$keluar" "/Fpemasang-uji" $iss
    if ($LASTEXITCODE -ne 0) { throw "iscc gagal (kode $LASTEXITCODE)" }
    $setup = Join-Path $keluar "pemasang-uji.exe"

    $kode = Jalankan $setup @("/VERYSILENT", "/SUPPRESSMSGBOXES", "/NORESTART", "/NOICONS",
                               "/TASKS=tambahpath", "/DIR=`"$app`"", "/LOG=`"$tmp\pasang.log`"")
    if ($kode -eq 0 -and (Test-Path (Join-Path $app "indk.exe"))) { Lulus "terpasang di folder sementara" }
    else { Tolak "pemasangan gagal (kode $kode)" }

    $entri = (BacaPath) -split ";" | Where-Object { $_.TrimEnd("\") -ieq $app }
    if ($entri) { Lulus "folder masuk ke Path HKLM" } else { Tolak "folder tidak masuk ke Path HKLM" }

    $kode = Jalankan (Join-Path $app "unins000.exe") @("/VERYSILENT", "/SUPPRESSMSGBOXES", "/NORESTART")
    # unins000.exe menyalin dirinya ke folder sementara lalu langsung kembali;
    # tunggu sampai foldernya benar-benar hilang.
    for ($i = 0; $i -lt 120 -and (Test-Path $app); $i++) { Start-Sleep -Milliseconds 500 }
    if ($kode -eq 0 -and -not (Test-Path $app)) { Lulus "uninstall menghapus folder" }
    else { Tolak "uninstall gagal (kode $kode, folder masih ada: $(Test-Path $app))" }

    $pathAkhir = BacaPath
    $sisa = $pathAkhir -split ";" | Where-Object { $_.TrimEnd("\") -ieq $app }
    if (-not $sisa) { Lulus "entri dibuang dari Path HKLM" } else { Tolak "entri masih ada di Path HKLM" }
    if ($pathAkhir -ceq $pathAwal) { Lulus "Path HKLM sama persis dengan sebelum pemasangan" }
    else {
        Tolak "Path HKLM berubah"
        Write-Host "      sebelum: $pathAwal"
        Write-Host "      sesudah: $pathAkhir"
    }
} catch {
    Tolak $_.Exception.Message
} finally {
    $u = Join-Path $app "unins000.exe"
    if (Test-Path $u) { Jalankan $u @("/VERYSILENT", "/SUPPRESSMSGBOXES", "/NORESTART") | Out-Null }
    if ((BacaPath) -cne $pathAwal) { TulisPath $pathAwal; Write-Host "      (Path HKLM dikembalikan ke nilai awal)" }
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
}

if ($gagal -eq 0) {
    Write-Host "PEMASANG PATH: semua lulus" -ForegroundColor Green
    exit 0
}
Write-Host "PEMASANG PATH: GAGAL" -ForegroundColor Red
exit 1
