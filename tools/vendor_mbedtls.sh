#!/usr/bin/env bash
# Mengambil ulang mbedTLS yang di-vendor ke src/vendor/mbedtls/.
#
# Kenapa skrip, bukan salin-tempel sekali jalan: kode pihak ketiga di dalam
# repo adalah utang keamanan. Saat ada CVE, seseorang harus bisa menaikkan
# versi tanpa menebak-nebak berkas mana yang dulu disalin. Jalankan:
#
#     bash tools/vendor_mbedtls.sh            # versi terkunci di bawah
#     MBEDTLS_VERSI=3.6.3 bash tools/vendor_mbedtls.sh   # menaikkan versi
#
# Sesudah menaikkan versi, WAJIB perbarui MBEDTLS_SHA256 di bawah dengan nilai
# resmi dari halaman rilis mbedTLS -- jangan menyalin nilai yang dicetak skrip
# ini, karena itu berarti mempercayai berkas yang baru saja diunduh.
set -euo pipefail

MBEDTLS_VERSI="${MBEDTLS_VERSI:-3.6.2}"
# sha256 tarball rilis resmi mbedtls-3.6.2.tar.bz2
MBEDTLS_SHA256="${MBEDTLS_SHA256:-8b54fb9bcf4d5a7078028e0520acddefb7900b3e66fec7f7175bb5b7d85ccdca}"

akar="$(cd "$(dirname "$0")/.." && pwd)"
tujuan="$akar/src/vendor/mbedtls"
kerja="$(mktemp -d)"
trap 'rm -rf "$kerja"' EXIT

arsip="mbedtls-$MBEDTLS_VERSI.tar.bz2"
url="https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-$MBEDTLS_VERSI/$arsip"

echo "Mengunduh $url"
curl -fsSL -o "$kerja/$arsip" "$url"

echo "Memeriksa sha256"
echo "$MBEDTLS_SHA256  $kerja/$arsip" | sha256sum -c - || {
  echo "GAGAL: sha256 tidak cocok. Arsip TIDAK dipakai." >&2
  echo "Jika ini karena menaikkan versi, perbarui MBEDTLS_SHA256 dari" >&2
  echo "halaman rilis resmi, bukan dari berkas yang baru diunduh." >&2
  exit 1
}

tar xjf "$kerja/$arsip" -C "$kerja"
sumber="$kerja/mbedtls-$MBEDTLS_VERSI"

# Hanya tiga hal yang dibutuhkan: implementasi, header, dan lisensi.
# Sisanya (tes, program contoh, skrip CMake, dokumentasi) sengaja TIDAK
# di-vendor supaya permukaan audit sekecil mungkin.
rm -rf "$tujuan"
mkdir -p "$tujuan"
cp -R "$sumber/library" "$tujuan/library"
cp -R "$sumber/include" "$tujuan/include"
cp "$sumber/LICENSE" "$tujuan/LICENSE"
rm -f "$tujuan/include/CMakeLists.txt" "$tujuan/library/CMakeLists.txt"
rm -f "$tujuan/library/Makefile"

echo "$MBEDTLS_VERSI" > "$tujuan/VERSI"

echo "Selesai: $tujuan ($(ls "$tujuan"/library/*.c | wc -l) berkas .c)"
echo "Jangan lupa: jalankan 'make clean && make' lalu tests/test_all.sh"
