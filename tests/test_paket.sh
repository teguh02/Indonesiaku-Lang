#!/usr/bin/env bash
# Manajer paket `indk paket` (Fase 14). Logika ujinya ada di
# tests/paket_cli.idk; pembungkus ini hanya menyiapkan direktori kerja
# kosong dan membuangnya (git menulis objek read-only, jadi pembersihan
# memakai rm -rf, bukan Indonesiaku).
set -u
cd "$(dirname "$0")/.." || exit 1

BIN=./indk
[ -x "$BIN" ] || BIN=./indk.exe
if [ ! -x "$BIN" ]; then
    echo "indk belum dibangun. Jalankan 'make' dulu." >&2
    exit 1
fi
if ! command -v git >/dev/null 2>&1; then
    echo "PAKET: dilewati (git tidak terpasang)"
    exit 0
fi

T=$(mktemp -d 2>/dev/null || mktemp -d -t indk-paket)
trap 'rm -rf "$T"' EXIT
"$BIN" tests/paket_cli.idk "$T"
