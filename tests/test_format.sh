#!/usr/bin/env bash
# Tes `indk rapikan`: tests/format/<nama>.idk + <nama>.rapi (hasil diharapkan).
# Memeriksa tiga properti sekaligus:
#   1. hasil format sesuai berkas .rapi
#   2. idempoten: format dua kali == format sekali
#   3. --periksa: exit 0 kalau sudah rapi
cd "$(dirname "$0")/.." || exit 1
pass=0; fail=0
tmp="${TMPDIR:-/tmp}/indk_fmt_$$"

for src in tests/format/*.idk; do
    [ -e "$src" ] || continue
    expected="${src%.idk}.rapi"
    [ -e "$expected" ] || continue

    ./indk rapikan "$src" > "$tmp.1" 2>/dev/null || { fail=$((fail+1)); echo "GAGAL (rc): $src"; continue; }
    if ! diff -q "$tmp.1" "$expected" >/dev/null; then
        fail=$((fail+1)); echo "GAGAL (hasil): $src"; diff "$expected" "$tmp.1" | head -5; continue
    fi
    ./indk rapikan "$tmp.1" > "$tmp.2" 2>/dev/null
    if ! diff -q "$tmp.1" "$tmp.2" >/dev/null; then
        fail=$((fail+1)); echo "GAGAL (tidak idempoten): $src"; continue
    fi
    # --periksa harus MENERIMA berkas yang sudah rapi (.rapi) dan MENOLAK
    # berkas yang belum (.idk sengaja ditulis berantakan).
    if ! ./indk rapikan --periksa "$expected" >/dev/null 2>&1; then
        fail=$((fail+1)); echo "GAGAL (--periksa menolak berkas rapi): $expected"; continue
    fi
    if ! diff -q "$src" "$expected" >/dev/null; then
        if ./indk rapikan --periksa "$src" >/dev/null 2>&1; then
            fail=$((fail+1)); echo "GAGAL (--periksa menerima berkas berantakan): $src"; continue
        fi
    fi
    pass=$((pass+1))
done

# Properti terpenting: format TIDAK BOLEH mengubah arti program.
# Bandingkan bytecode sebelum/sesudah untuk seluruh examples/ dan pustaka/.
bc_fail=0; bc_total=0
for src in examples/*.idk examples/*/*.idk pustaka/*.idk; do
    [ -e "$src" ] || continue
    bc_total=$((bc_total+1))
    ./indk --dis "$src" 2>/dev/null | sed -E 's/^[0-9]{4} +[0-9|]+ //' > "$tmp.bc1"
    ./indk rapikan "$src" > "$tmp.src" 2>/dev/null || { bc_fail=$((bc_fail+1)); continue; }
    ./indk --dis "$tmp.src" 2>/dev/null | sed -E 's/^[0-9]{4} +[0-9|]+ //' > "$tmp.bc2"
    if ! diff -q "$tmp.bc1" "$tmp.bc2" >/dev/null; then
        bc_fail=$((bc_fail+1)); echo "GAGAL (bytecode berubah): $src"
    fi
done
echo "  bytecode identik: $((bc_total-bc_fail))/$bc_total"

# T-U11: berkas dengan bita NUL dulu dipotong di NUL oleh --tulis (data
# hilang, exit 0). Kini ditolak (exit 65) dan berkas tidak disentuh.
printf 'cetak(1)\0cetak(2)\ncetak(3)\n' > "$tmp.nul.idk"
cp "$tmp.nul.idk" "$tmp.nul.asli"
./indk rapikan --tulis "$tmp.nul.idk" >/dev/null 2>&1; rc=$?
if [ "$rc" -ne 65 ] || ! cmp -s "$tmp.nul.idk" "$tmp.nul.asli"; then
    fail=$((fail+1)); echo "GAGAL (NUL): rapikan --tulis exit $rc atau berkas berubah"
else
    pass=$((pass+1))
fi

rm -f "$tmp".*
echo "FORMAT: $pass/$((pass+fail)) lulus"
[ "$fail" -eq 0 ] && [ "$bc_fail" -eq 0 ]
