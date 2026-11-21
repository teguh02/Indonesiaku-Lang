#!/usr/bin/env bash
# Tes untuk `indk uji`: tests/uji/<nama>.idk + <nama>.uji (stdout diharapkan).
# Exit code ikut diperiksa: 0 bila semua lulus, 1 bila ada yang gagal.
cd "$(dirname "$0")/.." || exit 1
BIN=./indk
[ -x "$BIN" ] || BIN=./indk.exe   # Windows (Git Bash): make menghasilkan indk.exe
pass=0; fail=0
# Sumbernya tests/uji/<nama>.idk, atau examples/<nama>.idk untuk berkas uji
# yang sengaja diletakkan di samping aplikasi yang diujinya (mis.
# examples/uji_api_siswa.idk mengimpor examples/api_siswa.idk).
for expected in tests/uji/*.uji; do
    [ -e "$expected" ] || continue
    nama=$(basename "$expected" .uji)
    src="tests/uji/$nama.idk"
    [ -e "$src" ] || src="examples/$nama.idk"
    [ -e "$src" ] || { fail=$((fail+1)); echo "GAGAL: $expected (sumber tidak ada)"; continue; }
    actual=$("$BIN" uji "$src" 2>/dev/null)
    rc=$?
    want=$(cat "$expected")
    wantrc=0
    grep -q "GAGAL" "$expected" && wantrc=1
    if [ "$actual" = "$want" ] && [ "$rc" = "$wantrc" ]; then
        pass=$((pass+1))
    else
        fail=$((fail+1))
        echo "GAGAL: $src"
        echo "  diharapkan (rc=$wantrc): $want"
        echo "  didapat    (rc=$rc): $actual"
    fi
done
echo "UJI: $pass/$((pass+fail)) lulus"
[ "$fail" -eq 0 ]
