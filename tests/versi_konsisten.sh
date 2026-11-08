#!/usr/bin/env bash
# Tag image playground harus mengikuti INDK_VERSION.
#
# Playground publik pernah menjalankan indk 1.0.0 selama empat rilis sementara
# situs mengiklankan 1.4.0: pengunjung yang mencoba `impor web` dari panduan
# mendapat "Modul 'web' tidak ditemukan". Tag yang tertinggal tidak pernah
# gagal dengan sendirinya -- hanya diam dan salah.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$AKAR" || exit 1

VERSI="$(grep -oE '#define INDK_VERSION "[^"]+"' src/common.h | grep -oE '[0-9][^"]*')"
if [ -z "$VERSI" ]; then
    echo "GAGAL: tidak dapat membaca INDK_VERSION dari src/common.h"
    exit 1
fi

GAGAL=0

periksa() {
    local nama="$1" berkas="$2" pola="$3"
    local dapat
    dapat="$(grep -oE "$pola" "$berkas" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+(-[a-z]+\.[0-9]+)?' | sort -u | tr '\n' ' ' | sed 's/ $//')"
    if [ "$dapat" = "$VERSI" ]; then
        echo "  ok   $nama = $VERSI"
    else
        echo "  GAGAL $nama: harap '$VERSI', dapat '$dapat' ($berkas)"
        GAGAL=1
    fi
}

echo "Tag versi konsisten dengan INDK_VERSION ($VERSI):"
periksa "base image playground" docker/playground/Dockerfile '^FROM teguh02/indonesiaku:[0-9][0-9a-z.-]*'
periksa "image compose playground" docker/playground/docker-compose.yml 'image: indonesiaku-website:[0-9][0-9a-z.-]*'
periksa "LABEL version image rilis" docker/images/Dockerfile 'LABEL version="[0-9][0-9a-z.-]*"'
# Image aplikasi (docker/aplikasi) ikut dijaga di sini; kalau tidak, tag-nya
# diam-diam menyimpang setiap kali versi dinaikkan.
periksa "image compose aplikasi" docker/aplikasi/docker-compose.yml 'image: indonesiaku-aplikasi:[0-9][0-9a-z.-]*'

# Cache-buster aset situs: HTML baru dengan ?v= lama menampilkan CSS basi
# karena Cloudflare menyimpan assets/* selama 4 jam.
LAIN="$(grep -rhoE '\?v=[0-9]+\.[0-9]+\.[0-9]+(-[a-z]+\.[0-9]+)?' website/ | sort -u | grep -v "?v=$VERSI" | tr '\n' ' ')"
if [ -z "$LAIN" ]; then
    echo "  ok   cache-buster website semuanya ?v=$VERSI"
else
    echo "  GAGAL cache-buster website masih memakai: $LAIN"
    GAGAL=1
fi

if [ "$GAGAL" -eq 0 ]; then
    echo "ok   semua tag versi konsisten"
    exit 0
fi
echo "GAGAL: ada tag versi yang tertinggal"
exit 1
