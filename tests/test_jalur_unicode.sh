#!/usr/bin/env bash
# Jalur skrip, argumen, INDK_PATH, dan nama berkas di luar ASCII (T-W1).
#
# Di Windows indk dulu menerima argv ANSI dan membuka berkas dengan fopen(),
# sehingga `indk 日本/s.idk` gagal ("??/s.idk", kode 74). Padanan Windows:
# tests/test_jalur_unicode.ps1. Di Linux/macOS jalur hanyalah bita, jadi tes
# ini menjaga agar perilaku itu tidak rusak.

set -u
cd "$(dirname "$0")/.."

BIN="$PWD/indk"
[ -x "$BIN" ] || BIN="$PWD/indk.exe"
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

echo "Jalur Unicode (skrip, modul, argumen, INDK_PATH)"

TMP="${TMPDIR:-/tmp}/indk_unicode_$$"
DIR="$TMP/日本 ☕"
JAUH="$TMP/pustaka naïve"
mkdir -p "$DIR" "$JAUH"
trap 'rm -rf "$TMP"' EXIT

# Sumber ASCII saja: nama non-ASCII dirakit dengan chr(), supaya tes ini
# menguji jalur, bukan pemindai.
cat > "$DIR/s.idk" <<'EOF'
impor lokal
impor jauh
cetak(lokal.NILAI, jauh.NILAI)
cetak(argumen == ["a" + chr(0xe4) + " " + chr(0x2615)])
nama = "keluaran_" + chr(0x2615) + ".txt"
tulis_semua(nama, "x")
cetak(nama dalam isi_direktori("."), baca_semua(nama))
hapus_berkas(nama)
cetak(ada_berkas(nama))
EOF
echo 'NILAI = "lokal"' > "$DIR/lokal.idk"
echo 'NILAI = "jauh"' > "$JAUH/jauh.idk"
HARAP=$'lokal jauh\nbenar\nbenar x\nsalah'

got=$(cd "$DIR" && INDK_PATH="$JAUH" "$BIN" "$DIR/s.idk" "aä ☕" 2>&1 | tr -d '\015')
periksa "jalur absolut" "$got" "$HARAP"

got=$(cd "$TMP" && INDK_PATH="$JAUH" "$BIN" "日本 ☕/s.idk" "aä ☕" 2>&1 | tr -d '\015')
# Berkas ditulis relatif ke direktori kerja ($TMP), modul tetap ditemukan
# dari direktori skrip.
periksa "jalur relatif" "$got" "$HARAP"

if [ "$GAGAL" -eq 0 ]; then
    echo "JALUR UNICODE: semua berjalan"
    exit 0
fi
echo "JALUR UNICODE: $GAGAL gagal" >&2
exit 1
