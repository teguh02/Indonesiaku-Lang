#!/usr/bin/env bash
# Regresi readFile(): indk harus membaca sumber dari pipe/karakter device,
# bukan hanya dari berkas biasa.
#
# Dulu readFile() mempercayai ftell(). Pada pipe seek gagal, ftell() mengembalikan
# -1, dan -1 di dalam size_t membuat malloc(fileSize + 1) mengalokasikan nol bita
# -- fread lalu menulis melewatinya: "*** buffer overflow detected ***".
# Lolos dari seluruh suite karena setiap tes lain memakai berkas biasa.

set -u
cd "$(dirname "$0")/.."

BIN=./indk
[ -x "$BIN" ] || BIN=./indk.exe
if [ ! -x "$BIN" ]; then
    echo "indk belum dibangun. Jalankan 'make' dulu." >&2
    exit 1
fi

GREEN='\033[0;32m'; RED='\033[0;31m'; NC='\033[0m'
GAGAL=0

periksa() {   # nama, keluaran_didapat, keluaran_diharapkan
    if [ "$2" = "$3" ]; then
        echo -e "  ${GREEN}+${NC} $1"
    else
        echo -e "  ${RED}x${NC} $1 (didapat: '$2', diharapkan: '$3')"
        GAGAL=$((GAGAL+1))
    fi
}

echo "Regresi baca sumber (berkas vs pipe)"

TMP="${TMPDIR:-/tmp}/indk_baca_$$"
mkdir -p "$TMP"
trap 'rm -rf "$TMP"' EXIT

# 1. Pipe pendek: yang dulu memicu buffer overflow.
printf 'cetak("halo pipa")\n' > "$TMP/pendek.idk"
got=$(printf 'cetak("halo pipa")\n' | "$BIN" /dev/stdin 2>&1)
periksa "pipe pendek" "$got" "halo pipa"

# 2. Berkas dan pipe dengan isi identik harus memberi hasil identik.
dari_berkas=$("$BIN" "$TMP/pendek.idk" 2>&1)
periksa "berkas = pipe" "$got" "$dari_berkas"

# 3. Lebih besar dari buffer awal (8 KiB): memaksa jalur realloc.
{
    for _ in $(seq 400); do
        echo "# padding xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    done
    echo 'cetak("akhir berkas besar")'
} > "$TMP/besar.idk"

got=$("$BIN" "$TMP/besar.idk" 2>&1)
periksa "berkas > 8 KiB" "$got" "akhir berkas besar"

got=$("$BIN" /dev/stdin < "$TMP/besar.idk" 2>&1)
periksa "pipe > 8 KiB" "$got" "akhir berkas besar"

# 4. BOM UTF-8 di awal berkas dilewati, baik pada skrip utama maupun modul
#    yang diimpor (T-W2). Dibuat di sini dengan printf, supaya editor atau
#    alat yang membuang BOM dari examples/bom_utf8.idk tidak diam-diam
#    menghilangkan cakupannya.
printf '\357\273\277X = "modul ber-BOM"\n' > "$TMP/modbom.idk"
printf '\357\273\277impor modbom\ncetak(modbom.X)\n' > "$TMP/bom.idk"
got=$("$BIN" "$TMP/bom.idk" 2>&1)
periksa "BOM skrip + modul" "$got" "modul ber-BOM"
printf '\357\273\277\357\273\277cetak(1)\n' > "$TMP/bom2.idk"
"$BIN" "$TMP/bom2.idk" >/dev/null 2>&1; kode=$?
periksa "hanya satu BOM dilewati (exit)" "$kode" "65"

# 5. Masukan kosong: tidak boleh crash, tidak boleh ada keluaran.
got=$(printf '' | "$BIN" /dev/stdin 2>&1); kode=$?
periksa "pipe kosong (exit $kode)" "$got" ""

if [ "$GAGAL" -eq 0 ]; then
    echo "BACA SUMBER: semua berjalan"
    exit 0
fi
echo "BACA SUMBER: $GAGAL gagal" >&2
exit 1
