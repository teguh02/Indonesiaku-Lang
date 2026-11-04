#!/usr/bin/env bash
# Linter tests: tests/lint/<nama>.idk + <nama>.lint (stderr yang diharapkan).
# Exit code: 1 bila ada temuan, 0 bila bersih -- ikut diperiksa.
cd "$(dirname "$0")/.." || exit 1
BIN=./indk
[ -x "$BIN" ] || BIN=./indk.exe   # Windows (Git Bash): make menghasilkan indk.exe
pass=0; fail=0
for src in tests/lint/*.idk; do
    [ -e "$src" ] || continue
    expected="${src%.idk}.lint"
    [ -e "$expected" ] || continue
    actual=$("$BIN" periksa "$src" 2>&1 >/dev/null)
    rc=$?
    want=$(cat "$expected")
    wantrc=0
    grep -q "tidak pernah dipakai" "$expected" && wantrc=1
    if [ "$actual" = "$want" ] && [ "$rc" = "$wantrc" ]; then
        pass=$((pass+1))
    else
        fail=$((fail+1))
        echo "GAGAL: $src"
        echo "  diharapkan (rc=$wantrc): $want"
        echo "  didapat    (rc=$rc): $actual"
    fi
done
echo "LINTER: $pass/$((pass+fail)) lulus"
[ "$fail" -eq 0 ]
