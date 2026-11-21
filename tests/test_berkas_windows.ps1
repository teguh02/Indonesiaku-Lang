# Modul berkas (Fase 17) di Windows: jalur di luar code page ANSI dan
# junction (reparse point) yang tidak boleh diikuti. Padanan POSIX untuk
# tautan: tests/berkas_tautan.sh (Git Bash tidak bisa membuat symlink).
#
# Berkas ini sengaja hanya ASCII: PowerShell 5.1 membaca .ps1 tanpa BOM
# sebagai ANSI, jadi nama non-ASCII dirakit dengan [char], dan program .idk
# hanya mencetak ASCII.

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$bin = Join-Path $repo "indk.exe"
if (-not (Test-Path $bin)) {
    Write-Host "indk.exe belum dibangun." -ForegroundColor Red
    exit 1
}

$jepang = [string][char]0x65E5 + [char]0x672C
$kopi = [string][char]0x2615
$t = Join-Path ([System.IO.Path]::GetTempPath()) ("indk-berkas-" + [System.Guid]::NewGuid().ToString("N"))
$d = Join-Path $t ($jepang + " " + $kopi)
New-Item -ItemType Directory -Force -Path (Join-Path $d "pohon") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $d "luar") | Out-Null
$utf8 = New-Object System.Text.UTF8Encoding $false
[System.IO.File]::WriteAllText((Join-Path $d "pohon\a.txt"), "a", $utf8)
[System.IO.File]::WriteAllText((Join-Path $d "luar\penting.txt"), "jangan hilang", $utf8)

# Junction tidak butuh hak admin (symlink butuh Developer Mode).
$null = cmd /c mklink /J "`"$(Join-Path $d 'pohon\tautan')`"" "`"$(Join-Path $d 'luar')`"" 2>&1
$null = cmd /c mklink /J "`"$(Join-Path $d 'akar_tautan')`"" "`"$(Join-Path $d 'pohon')`"" 2>&1

$gagal = 0
function Uji($nama, $harap, $skrip) {
    $ErrorActionPreference = "Continue"
    $env:IDK_DIR = $d
    $keluar = & $bin $skrip 2>&1 | Out-String
    $kode = $LASTEXITCODE
    $keluar = ($keluar -replace "`r", "").TrimEnd()
    if ($kode -eq 0 -and $keluar -eq $harap) {
        Write-Host "  OK   $nama"
    } else {
        Write-Host "  GAGAL $nama (kode $kode)" -ForegroundColor Red
        Write-Host "        harap: $harap"
        Write-Host "        dapat: $keluar"
        $script:gagal = 1
    }
}

$harapTautan = @"
jelajahi    : 1 [] ["a.txt", "tautan"]
info tautan : tautan
salin       : salin_direktori(): 'pohon/tautan' tautan simbolik (pakai lewati_tautan=benar)
salin lewati: ["a.txt"]
akar tautan : hapus_direktori_bertingkat(): 'akar_tautan' tautan simbolik
hapus pohon : ["penting.txt"] jangan hilang
"@ -replace "`r", ""

try {
    Uji "junction tidak diikuti, jalur non-ASCII" $harapTautan (Join-Path $repo "tests\bantu\berkas_tautan.idk")

    $program = @'
impor berkas
D = ganti(ambil_lingkungan("IDK_DIR"), "\\", "/")
nama = D + "/catatan " + chr(0x2615) + ".txt"
tulis_semua(nama, "isi")
berkas.salin_berkas(nama, D + "/" + chr(0x65e5) + ".txt")
berkas.pindahkan(D + "/" + chr(0x65e5) + ".txt", D + "/luar/" + chr(0x672c) + ".txt")
cetak(berkas.sha256(D + "/luar/" + chr(0x672c) + ".txt") == sha256("isi"))
t = berkas.berkas_sementara(direktori=D)
berkas.ganti_nama(t, D + "/sementara " + chr(0x2615), timpa=benar)
cetak(berkas.info(D + "/sementara " + chr(0x2615))["jenis"])
berkas.hapus_direktori_bertingkat(D + "/luar")
cetak(berkas.jelajahi_direktori(D)[0][1])
'@
    $skrip = Join-Path $t "unicode.idk"
    [System.IO.File]::WriteAllText($skrip, $program, $utf8)
    Uji "salin/pindah/ganti_nama/sha256 di jalur non-ASCII" "benar`nberkas`n[`"salinan`", `"salinan2`"]" $skrip
} finally {
    Remove-Item -Recurse -Force $t -ErrorAction SilentlyContinue
}

if ($gagal -ne 0) { exit 1 }
Write-Host "berkas Windows: lulus" -ForegroundColor Green
exit 0
