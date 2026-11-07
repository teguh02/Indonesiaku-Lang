#!/usr/bin/env bash
# API embedding C (Fase 8): membuktikan libindk.a benar-benar bisa dipakai
# program C lain.
#
# Tes ini mengompilasi DAN menjalankan contoh nyata, bukan sekadar memeriksa
# berkas header ada. Pelajaran berulang di repo ini: dokumen yang menjanjikan
# sesuatu yang tidak pernah dieksekusi akan basi diam-diam.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$AKAR" || exit 1

TMP="$(mktemp -d)"
# Git Bash: jalur POSIX tidak dikenal oleh program Windows. Konversi sejak
# awal, bukan setelah CI merah (pelajaran dari templat_cache.sh & sandbox.sh).
if command -v cygpath >/dev/null 2>&1; then
    TMP="$(cygpath -m "$TMP")"
fi
GAGAL=0
bersihkan() { rm -rf "$TMP"; }
trap bersihkan EXIT

ok()    { echo "  ok   $1"; }
gagal() { echo "  GAGAL $1"; GAGAL=$((GAGAL+1)); }

CC="${CC:-cc}"
if ! command -v "$CC" >/dev/null 2>&1; then
    echo "  LEWAT embedding: tidak ada kompiler C ($CC)"
    exit 0
fi

INC="-Isrc -Isrc/vendor/mbedtls/include"

# Windows menaut soket dan kripto dari pustaka sistem terpisah; tanpa ini
# penautan gagal dengan undefined reference ke __imp_WSAStartup dkk.
# Nilai ini sengaja sama dengan LDFLAGS di Makefile.
TAUT="-lm"
case "$(uname -s 2>/dev/null)" in
    MINGW*|MSYS*|CYGWIN*) TAUT="-lm -lws2_32 -lbcrypt" ;;
esac

# --- 1. libindk.a terbentuk -------------------------------------------------
if make -s libindk.a >/dev/null 2>&1 && [ -f libindk.a ]; then
    ok "libindk.a terbentuk"
else
    gagal "libindk.a gagal dibangun"
    echo "FAILED=$GAGAL"; exit 1
fi

# libindk.a tidak boleh memuat main(): kalau ikut, program pemakai bentrok
# saat menaut.
if "${NM:-nm}" libindk.a 2>/dev/null | grep -qE '^[0-9a-f]+ T _?main$'; then
    gagal "libindk.a memuat main() -- akan bentrok dengan main() pemakai"
else
    ok "libindk.a tidak memuat main()"
fi

# --- 2. contoh resmi terkompilasi dan jalan --------------------------------
if "$CC" examples/embed/halo.c libindk.a $INC $TAUT -o "$TMP/halo" 2>"$TMP/cc.log"; then
    ok "examples/embed/halo.c terkompilasi"
else
    gagal "examples/embed/halo.c gagal kompilasi"
    sed 's/^/      /' "$TMP/cc.log"
fi

if [ -x "$TMP/halo" ]; then
    KELUARAN="$("$TMP/halo" 2>/dev/null | tr -d '\r')"
    # 42 bulat, bukan 42.0: fungsi C wajib bisa mengembalikan VAL_INT.
    if echo "$KELUARAN" | grep -qx '42'; then
        ok "fungsi C terpanggil dari Indonesiaku, bilangan bulat terjaga"
    else
        gagal "keluaran tak sesuai: $KELUARAN"
    fi
    if echo "$KELUARAN" | grep -q 'global pesan: halo dari Indonesiaku'; then
        ok "indk_ambil_global_teks membaca global skrip"
    else
        gagal "global tidak terbaca"
    fi
fi

# --- 3. program uji sendiri: nilai balik dan isolasi galat -----------------
cat > "$TMP/uji.c" <<'EOF'
#include <stdio.h>
#include <string.h>
#include "indk.h"

static Value selalu_tujuh(int jumlah, Value* argumen) {
    (void)jumlah; (void)argumen;
    return INT_VAL(7);
}

int main(void) {
    indk_mulai();

    /* sumber sah -> OK */
    printf("A%d\n", (int)indk_jalankan("variabel x = 1"));

    /* sintaks salah -> GALAT_KOMPILASI, proses TIDAK mati */
    printf("B%d\n", (int)indk_jalankan("fungsi {{{"));

    /* galat runtime -> GALAT_RUNTIME, proses TIDAK mati */
    printf("C%d\n", (int)indk_jalankan("naikkan GalatNilai(\"x\")"));

    /* fungsi C terdaftar benar-benar dipanggil */
    indk_daftar_fungsi("selalu_tujuh", selalu_tujuh);
    indk_jalankan("hasil = selalu_tujuh()");
    char buf[64];
    if (indk_ambil_global_teks("hasil", buf, sizeof buf)) printf("D%s\n", buf);
    else printf("Dtidak-ada\n");

    /* global yang tidak ada -> false */
    printf("E%d\n", indk_ambil_global_teks("tidak_ada_ini", buf, sizeof buf) ? 1 : 0);

    /* pemotongan aman: buffer kecil tidak boleh meluap */
    indk_jalankan("panjangsekali = \"abcdefghijklmnopqrstuvwxyz\"");
    char kecil[5];
    indk_ambil_global_teks("panjangsekali", kecil, sizeof kecil);
    printf("F%s|%d\n", kecil, (int)strlen(kecil));

    indk_selesai();

    /* siklus kedua di proses yang sama harus bersih */
    indk_mulai();
    printf("G%d\n", (int)indk_jalankan("cetak(\"siklus2\")"));
    indk_selesai();
    return 0;
}
EOF

if "$CC" "$TMP/uji.c" libindk.a $INC $TAUT -o "$TMP/uji" 2>"$TMP/cc2.log"; then
    ok "program uji terkompilasi"
    HASIL="$("$TMP/uji" 2>/dev/null | tr -d '\r')"
    periksa() {
        if echo "$HASIL" | grep -qx "$1"; then ok "$2"
        else gagal "$2 (dapat: $(echo "$HASIL" | tr '\n' ' '))"; fi
    }
    periksa "A0" "sumber sah -> INDK_OK"
    periksa "B1" "sintaks salah -> INDK_GALAT_KOMPILASI (proses hidup)"
    periksa "C2" "galat runtime -> INDK_GALAT_RUNTIME (proses hidup)"
    periksa "D7" "fungsi C terdaftar dipanggil dari skrip"
    periksa "E0" "global tak dikenal -> false"
    periksa "Fabcd|4" "pemotongan buffer aman"
    periksa "G0" "mulai/selesai dua siklus tetap jalan"
else
    gagal "program uji gagal kompilasi"
    sed 's/^/      /' "$TMP/cc2.log"
fi

echo "FAILED=$GAGAL"
[ "$GAGAL" -eq 0 ]
