# Jalur lebih dari 260 karakter di Windows (T-W10).
#
# indk 1.4.0 - 1.6.0-rc.1 memberikan jalur apa adanya ke API berkas Win32,
# yang berhenti di MAX_PATH (260; CreateDirectoryW sudah di 248) kecuali
# biner punya manifest longPathAware. indk tidak punya, jadi
# LongPathsEnabled=1 di registri pun tidak menolong: buat_direktori,
# tulis_semua, baca_semua, isi_direktori, ada_berkas, dan hapus_berkas gagal
# semua pada jalur ~400 karakter. Kini jalur yang bentuk absolutnya sepanjang
# itu diberi awalan \\?\ (src/sistem.c keLebarJalur), yang tidak bergantung
# pada registri.
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

Write-Host "Jalur panjang > 260 karakter (T-W10)"

$t = Join-Path ([System.IO.Path]::GetTempPath()) ("indk-panjang-" + [System.Guid]::NewGuid().ToString("N"))
$seg = "d" * 60
# Folder skrip sendiri juga panjang (>260): menjalankan skrip dan impor
# modul di sebelahnya harus bekerja. Dibuat lewat .NET dengan \\?\ karena
# New-Item di PowerShell 5.1 tidak selalu sanggup.
$jauh = $t
for ($i = 0; $i -lt 4; $i++) { $jauh = Join-Path $jauh $seg }
[System.IO.Directory]::CreateDirectory("\\?\" + $jauh) | Out-Null

$utf8 = New-Object System.Text.UTF8Encoding $false
$program = @'
impor jalur
impor modul_sebelah

dasar = argumen[0]
cetak("modul:", modul_sebelah.NILAI)

# 1. Jalur absolut panjang, garis miring maju.
j = dasar
untuk i dalam rentang(5) {
    j = j + "/" + "e" * 60
    buat_direktori(j)
}
cetak("panjang > 260:", panjang(j) > 260)
f = j + "/f.txt"
tulis_semua(f, "isi")
tambah_berkas(f, " panjang")
cetak("baca:", baca_semua(f))
cetak("ada:", ada_berkas(f), jalur.ada(f), jalur.adalah_direktori(j))
cetak("isi_direktori:", isi_direktori(j))
h = buka_berkas(f, "b")
cetak("buka_berkas:", baca_baris(h))
tutup(h)

# 2. '..' di tengah jalur panjang diselesaikan sebelum \\?\ ditambahkan.
putar = j + "/../" + "e" * 60 + "/f.txt"
cetak("dengan ..:", baca_semua(putar))

# 3. Garis miring terbalik.
cetak("backslash:", ada_berkas(ganti(f, "/", "\\")))

# 4. Bersihkan: hapus_berkas dan hapus_direktori pada jalur panjang.
hapus_berkas(f)
cetak("setelah hapus:", ada_berkas(f))
selagi panjang(j) > panjang(dasar) {
    hapus_direktori(j)
    j = jalur.nama_direktori(j)
}
cetak("direktori bersih:", tidak jalur.ada(dasar + "/" + "e" * 60))

# 5. Jalur RELATIF pendek yang bentuk absolutnya > 260.
r = "r" * 60 + "/" + "r" * 60 + "/" + "r" * 60
cetak("relatif pendek:", panjang(r) < 248)
buat_direktori("r" * 60)
buat_direktori("r" * 60 + "/" + "r" * 60)
buat_direktori(r)
tulis_semua(r + "/g.txt", "relatif")
cetak("relatif:", baca_semua(r + "/g.txt"))
hapus_berkas(r + "/g.txt")
hapus_direktori(r)
hapus_direktori("r" * 60 + "/" + "r" * 60)
hapus_direktori("r" * 60)
'@
[System.IO.File]::WriteAllText("\\?\" + (Join-Path $jauh "uji.idk"), $program, $utf8)
[System.IO.File]::WriteAllText("\\?\" + (Join-Path $jauh "modul_sebelah.idk"), "NILAI = 7`n", $utf8)

$harap = @"
modul: 7
panjang > 260: benar
baca: isi panjang
ada: benar benar benar
isi_direktori: ["f.txt"]
buka_berkas: isi panjang
dengan ..: isi panjang
backslash: benar
setelah hapus: salah
direktori bersih: benar
relatif pendek: benar
relatif: relatif
"@ -replace "`r`n", "`n"

$gagal = 0
try {
    # Direktori kerja = folder panjang, supaya kasus 5 (relatif) benar-benar
    # melewati 260 dalam bentuk absolutnya.
    $ErrorActionPreference = "Continue"
    $mulai = Join-Path $t $seg
    Push-Location $mulai
    try {
        $keluar = & $bin (Join-Path $jauh "uji.idk") ($jauh -replace '\\', '/') 2>&1 | Out-String
    } finally {
        Pop-Location
    }
    $keluar = ($keluar -replace "`r`n", "`n").TrimEnd("`n")
    if ($LASTEXITCODE -eq 0 -and $keluar -eq $harap) {
        Write-Host "  + skrip, impor, dan semua operasi berkas di jalur > 260" -ForegroundColor Green
    } else {
        Write-Host "  x jalur panjang (kode $LASTEXITCODE):" -ForegroundColor Red
        Write-Host $keluar
        $gagal++
    }
} finally {
    if ($t.StartsWith([System.IO.Path]::GetTempPath()) -and (Test-Path -LiteralPath $t)) {
        [System.IO.Directory]::Delete("\\?\" + $t, $true)
    }
}

if ($gagal -eq 0) {
    Write-Host "JALUR PANJANG: semua berjalan" -ForegroundColor Green
    exit 0
}
Write-Host "JALUR PANJANG: $gagal gagal" -ForegroundColor Red
exit 1
