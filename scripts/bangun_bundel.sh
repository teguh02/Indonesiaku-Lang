#!/usr/bin/env bash
# Bangun bundel executable Indonesiaku (AOT V1).
#
# Salin biner indk apa adanya, tempel isi SATU skrip .idk di akhir file,
# lalu footer [MAGIC 8 byte "IDKBND01"][panjang payload LE 8 byte]. Biner
# hasil dijalankan tanpa argumen akan mengekstrak dan menjalankan skrip itu
# -- lihat ekstrakBundel() di src/main.c.
#
# ponytail: V1 TIDAK mendukung skrip yang `impor` modul lain (satu berkas
# saja). Multi-berkas ditunda sampai ada kebutuhan nyata -- lihat
# docs/RENCANA_FITUR/aot_bundel_executable.md §2.
set -euo pipefail

if [ "$#" -ne 3 ]; then
    echo "Penggunaan: $0 <biner-indk> <skrip.idk> <output>" >&2
    exit 64
fi
INDK_BIN="$1"
SKRIP="$2"
OUT="$3"

[ -f "$INDK_BIN" ] || { echo "GAGAL: biner tidak ditemukan: $INDK_BIN" >&2; exit 1; }
[ -f "$SKRIP" ] || { echo "GAGAL: skrip tidak ditemukan: $SKRIP" >&2; exit 1; }

cp "$INDK_BIN" "$OUT"
chmod +x "$OUT"
cat "$SKRIP" >> "$OUT"

LEN=$(wc -c < "$SKRIP" | tr -d ' ')

printf 'IDKBND01' >> "$OUT"

# Tulis LEN sebagai 8 byte little-endian -- tanpa dependency baru (tidak
# pakai python3/perl), murni bash + printf.
n="$LEN"
for _ in 1 2 3 4 5 6 7 8; do
    printf "$(printf '\\x%02x' $((n & 0xff)))" >> "$OUT"
    n=$((n >> 8))
done

echo "OK -> $OUT (skrip $LEN byte ditempel)"
