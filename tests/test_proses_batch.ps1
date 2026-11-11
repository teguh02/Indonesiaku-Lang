# proses.jalankan ke berkas .bat/.cmd (T-W6).
#
# CreateProcessW menjalankan berkas batch lewat cmd.exe, yang tidak mengikuti
# aturan kutip CommandLineToArgvW: argumen tanpa spasi tidak dikutip, jadi
# "a&perintah" menjalankan perintah, dan %VAR% diperluas walau dikutip. Kini
# argumen untuk berkas batch selalu dikutip, dan ", %, !, serta baris baru
# ditolak dengan GalatNilai.
#
# Bukti "tidak terbobol" bukan tebakan dari stdout: setiap muatan mencoba
# membuat berkas penanda; berkas itu tidak boleh pernah ada.
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

Write-Host "proses.jalankan ke berkas .bat/.cmd (T-W6)"

$t = Join-Path ([IO.Path]::GetTempPath()) ("indk-batch-" + [Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force $t | Out-Null
$utf8 = New-Object Text.UTF8Encoding $false
$gagal = 0
function Lulus($p) { Write-Host "  ok   $p" -ForegroundColor Green }
function Tolak($p) { Write-Host "  GAGAL $p" -ForegroundColor Red; $script:gagal = 1 }

try {
    [IO.File]::WriteAllText("$t\g.bat", "@echo off`r`necho arg=[%1]`r`n", $utf8)
    [IO.File]::WriteAllText("$t\g.cmd", "@echo off`r`necho arg=[%1]`r`n", $utf8)

    # Setiap baris: program, argumen, harapan ("arg=[...]" atau "GalatNilai").
    # "type nul > terbobol.txt" membuat berkas penanda bila perintah lolos.
    $idk = @'
impor proses
fungsi coba_jalan(program, arg) {
    coba {
        cetak(ganti(proses.jalankan([program, arg]).keluaran, "\n", ""))
    } kecuali GalatNilai sebagai e {
        cetak("GalatNilai")
    }
}
coba_jalan("g.bat", "a&type nul > terbobol.txt")
coba_jalan("g.bat", "a|type nul > terbobol.txt")
coba_jalan("G.BAT", "a&type nul > terbobol.txt")
coba_jalan("g.cmd", "a&type nul > terbobol.txt")
coba_jalan("g.bat", chr(34) + "&type nul > terbobol.txt&" + chr(34))
coba_jalan("g.bat", "%OS%")
coba_jalan("g.bat", "!OS!")
coba_jalan("g.bat", "a" + chr(10) + "type nul > terbobol.txt")
coba_jalan("g.bat", "nilai biasa")
coba_jalan("g.bat", "a&b")
'@
    $idk = $idk.Replace('"g.bat"', '".\\g.bat"').Replace('"G.BAT"', '".\\G.BAT"').Replace('"g.cmd"', '".\\g.cmd"')
    [IO.File]::WriteAllText("$t\uji.idk", $idk, $utf8)

    # stderr indk tidak boleh menjadi exception PowerShell: galatnya justru
    # ingin dilihat sebagai baris keluaran.
    $ErrorActionPreference = "Continue"
    Push-Location $t
    $keluaran = & $bin uji.idk 2>&1
    $kode = $LASTEXITCODE
    Pop-Location
    $ErrorActionPreference = "Stop"
    $baris = @($keluaran | ForEach-Object { "$_".TrimEnd("`r") })

    $harap = @(
        'arg=["a&type nul > terbobol.txt"]',
        'arg=["a|type nul > terbobol.txt"]',
        'arg=["a&type nul > terbobol.txt"]',
        'arg=["a&type nul > terbobol.txt"]',
        'GalatNilai',
        'GalatNilai',
        'GalatNilai',
        'GalatNilai',
        'arg=["nilai biasa"]',
        'arg=["a&b"]'
    )
    if ($kode -ne 0) { Tolak "indk keluar dengan kode $kode" }
    for ($i = 0; $i -lt $harap.Count; $i++) {
        $dapat = if ($i -lt $baris.Count) { $baris[$i] } else { "<tidak ada>" }
        if ($dapat -ceq $harap[$i]) { Lulus "kasus $($i + 1): $($harap[$i])" }
        else { Tolak "kasus $($i + 1): harap '$($harap[$i])', dapat '$dapat'" }
    }
    if (Test-Path "$t\terbobol.txt") { Tolak "berkas penanda terbuat: perintah lolos ke cmd.exe" }
    else { Lulus "tidak ada perintah yang lolos ke cmd.exe" }
} finally {
    Remove-Item -Recurse -Force $t -ErrorAction SilentlyContinue
}

if ($gagal -eq 0) {
    Write-Host "PROSES BATCH: semua lulus" -ForegroundColor Green
    exit 0
}
Write-Host "PROSES BATCH: GAGAL" -ForegroundColor Red
exit 1
