#!/bin/bash
# Benchmark Indonesiaku.
#
# Melaporkan dua angka per beban:
#   instruksi — dari `indk profil`, DETERMINISTIK. Ini yang dipakai untuk
#               menilai regresi: angkanya identik di mesin mana pun, jadi
#               perubahan berarti bytecode benar-benar berubah.
#   waktu     — median beberapa jalanan. Berguna untuk kinerja absolut, tapi
#               berisik; jangan simpulkan apa pun dari selisih kecil.
#
# Bukan gerbang lulus/gagal — dasar pembanding antar-commit.
#
# Beban hidup di tests/beban/, bukan examples/. examples/ terikat tes golden
# (keluarannya tidak boleh berubah) dan beberapa di antaranya placeholder
# 4 baris; menjadikannya benchmark berarti mengukur program yang nyaris tidak
# mengeksekusi apa pun — persis yang dulu terjadi di sini.

set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(dirname "$SCRIPT_DIR")"
cd "$REPO" || exit 1

BIN="./indk"
[ -f "$BIN" ] || BIN="./indk.exe"
if [ ! -f "$BIN" ]; then
    echo "Biner tidak ditemukan. Build dulu (make)."
    exit 1
fi

ULANG="${ULANG:-5}"

# Median waktu jalan, dalam detik. Memakai `time` bawaan bash dan bukan
# `date +%s.%N`: %N tidak ada di macOS (BSD date), dan `bc` tidak selalu
# terpasang — keduanya membuat benchmark lama mencetak baris kosong di sana.
jalankan_waktu() {
    local berkas="$1" i hasil
    hasil=""
    for ((i = 0; i < ULANG; i++)); do
        local t
        t="$( { time -p "$BIN" "$berkas" >/dev/null 2>&1; } 2>&1 | awk '/^real/{print $2}' )"
        hasil="$hasil$t"$'\n'
    done
    printf '%s' "$hasil" | sort -n | awk '{a[NR]=$1} END {print a[int((NR+1)/2)]}'
}

printf '=================================================================\n'
printf ' BENCHMARK INDONESIAKU  (%s)\n' "$(date)"
printf ' %s, median dari %s jalanan\n' "$("$BIN" -v 2>/dev/null | head -1)" "$ULANG"
printf '=================================================================\n'
printf '  %-22s %12s %10s %14s\n' "BEBAN" "INSTRUKSI" "WAKTU(s)" "JUTA INSTR/s"

ada=0
for b in tests/beban/*.idk; do
    [ -f "$b" ] || continue
    ada=1
    instr="$("$BIN" profil "$b" 2>&1 >/dev/null | awk '/instruksi :/{print $3}')"
    waktu="$(jalankan_waktu "$b")"
    laju="$(awk -v i="$instr" -v w="$waktu" 'BEGIN{ if (w > 0) printf "%.1f", i/w/1000000; else print "-" }')"
    printf '  %-22s %12s %10s %14s\n' "$(basename "$b" .idk)" "$instr" "$waktu" "$laju"
done

if [ "$ada" -eq 0 ]; then
    echo "  (tidak ada beban di tests/beban/)"
    exit 1
fi
printf '=================================================================\n'
