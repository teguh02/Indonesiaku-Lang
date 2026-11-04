#!/usr/bin/env bash
# Tes untuk `indk uji`: tests/uji/<nama>.idk + <nama>.uji (stdout diharapkan).
# Exit code ikut diperiksa: 0 bila semua lulus, 1 bila ada yang gagal.
cd "$(dirname "$0")/.." || exit 1
BIN=./indk
[ -x "$BIN" ] || BIN=./indk.exe   # Windows (Git Bash): make menghasilkan indk.exe
pass=0; fail=0
for src in tests/uji/*.idk; do
    [ -e "$src" ] || continue
    expected="${src%.idk}.uji"
    [ -e "$expected" ] || continue
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
