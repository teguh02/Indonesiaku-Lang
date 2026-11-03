#!/usr/bin/env bash
# Regresi T-U10: input() dan REPL membaca baris dengan panjang berapa pun.
#
# Dulu keduanya memakai fgets dengan buffer 1024 bita: baris yang lebih
# panjang terpotong pada 1023 bita dan sisanya muncul pada pembacaan
# berikutnya. Buffer blok REPL (8192 bita) diam-diam membuang baris yang
# tidak muat. Tes golden tidak bisa memberi stdin, jadi ini skrip terpisah.

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

ulang() {   # karakter, jumlah
    local s=""
    local i
    for ((i = 0; i < $2; i++)); do s="$s$1"; done
    printf '%s' "$s"
}

echo "Regresi baris panjang (input() dan REPL)"

TMP="${TMPDIR:-/tmp}/indk_baris_$$"
mkdir -p "$TMP"
trap 'rm -rf "$TMP"' EXIT

printf 'a = input()\nb = input()\ncetak(panjang(a), panjang(b))\ncetak(input())\n' > "$TMP/masukan.idk"

# 1. Baris 5000 bita lalu baris pendek: masing-masing satu input().
got=$( { ulang x 5000; printf '\nyyy\n'; } | "$BIN" "$TMP/masukan.idk" 2>&1 | tr -d '\r')
periksa "input() 5000 bita" "$got" "$(printf '5000 3\nkosong')"

# 2. Tepat di batas lama (1023 dan 1024 bita) dan baris terakhir tanpa '\n'.
got=$( { ulang a 1023; printf '\n'; ulang b 1024; printf '\nakhir'; } | "$BIN" "$TMP/masukan.idk" 2>&1 | tr -d '\r')
periksa "input() di batas 1024" "$got" "$(printf '1023 1024\nakhir')"

# 3. REPL: satu baris 3000 bita dan blok multi-baris di atas 8192 bita.
got=$( {
    printf 'cetak(panjang("%s"))\n' "$(ulang z 3000)"
    printf 'fungsi f() {\n    kembalikan "%s"\n}\n' "$(ulang q 9000)"
    printf 'cetak(panjang(f()))\n'
} | "$BIN" 2>&1 | tr -d '\r' | grep -o '[0-9][0-9]*$' | tr '\n' ' ')
periksa "REPL baris dan blok panjang" "$got" "3000 9000 "

if [ "$GAGAL" -eq 0 ]; then
    echo "BARIS PANJANG: semua berjalan"
    exit 0
fi
echo "BARIS PANJANG: $GAGAL gagal" >&2
exit 1
