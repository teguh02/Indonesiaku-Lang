#!/usr/bin/env bash
# Daftar builtin di ekstensi VS Code harus sama persis dengan defineNative()
# di src/vm.c.
#
# Daftar ini pernah diketik tangan dan tertinggal diam-diam: `teks_gabung`,
# `hmac_sha256`, `pbkdf2_sha256` dan `soket_baca_bita` tidak pernah disorot
# di editor meski sudah lama ada di VM. Daftar yang tertinggal tidak pernah
# gagal sendiri -- hanya diam dan salah.
#
# Logikanya ada di tests/builtin_konsisten.js; berkas ini hanya pembungkus
# agar pemanggilannya seragam dengan test lain.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$AKAR" || exit 1

if ! command -v node >/dev/null 2>&1; then
    echo "LEWAT: node tidak tersedia"
    exit 0
fi

exec node tests/builtin_konsisten.js
