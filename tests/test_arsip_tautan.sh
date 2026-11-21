#!/usr/bin/env bash
# arsip.ekstrak() tidak pernah menulis MELALUI tautan yang sudah ada di
# bawah direktori tujuan (Fase 23). Tidak bisa jadi golden: perlu `ln -s`.
# Padanan Windows (junction): tests/test_arsip_tautan.ps1.
#
#   1. tujuan/dir -> luar/   : arsip berisi dir/a.txt ditolak, luar/ kosong,
#      dan entri lain di arsip juga tidak ditulis (pemeriksaan sebelum tulis).
#   2. tujuan/a.txt -> luar/sasaran : ditolak, sasaran tidak berubah.
#   3. tujuan itu sendiri boleh berupa tautan ke direktori (mis. /tmp macOS).
#   4. arsip.buat() melewati tautan di direktori sumber.

set -u

SKRIP_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$SKRIP_DIR/.." && pwd)"
INDK="$REPO/indk"
[ -x "$INDK" ] || INDK="$REPO/indk.exe"
if [ ! -x "$INDK" ]; then
    echo "indk tidak ditemukan; jalankan make dulu" >&2
    exit 1
fi

GREEN='\033[0;32m'; RED='\033[0;31m'; NC='\033[0m'
LULUS=0; GAGAL=0
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cd "$TMP" || exit 1

lulus() { echo -e "${GREEN}LULUS${NC} $1"; LULUS=$((LULUS+1)); }
gagal() { echo -e "${RED}GAGAL${NC} $1"; GAGAL=$((GAGAL+1)); }

# Symlink butuh hak khusus di Windows (MSYS membuat salinan): lewati di sana,
# skrip .ps1 menguji junction.
ln -s / uji_ln 2>/dev/null
if [ ! -L uji_ln ]; then
    echo "ln -s tidak membuat tautan di sistem ini; lewati (lihat test_arsip_tautan.ps1)"
    exit 0
fi
rm -f uji_ln

cat > buat.idk <<'IDK'
impor arsip
arsip.buat("a.tar.gz", [["dir/a.txt", "isi a"], ["z.txt", "isi z"]])
arsip.buat("b.zip", [["a.txt", "baru"]])
IDK
"$INDK" buat.idk || { gagal "membuat arsip uji"; exit 1; }

cat > ekstrak.idk <<'IDK'
impor arsip
dari arsip impor GalatArsip
coba {
    arsip.ekstrak(argumen[0], argumen[1])
    cetak("DIEKSTRAK")
} kecuali GalatArsip sebagai e {
    cetak(e.pesan)
}
IDK

# 1. Direktori perantara berupa tautan.
mkdir -p luar1 tujuan1 && ln -s "$TMP/luar1" tujuan1/dir
keluaran="$("$INDK" ekstrak.idk a.tar.gz tujuan1)"
if [[ "$keluaran" == *"'dir' di tujuan adalah tautan; tidak diikuti"* ]] &&
   [ -z "$(ls -A luar1)" ] && [ ! -e tujuan1/z.txt ]; then
    lulus "tautan direktori di tujuan ditolak sebelum menulis"
else
    gagal "tautan direktori: '$keluaran'; luar1=$(ls -A luar1); z.txt ada? $([ -e tujuan1/z.txt ] && echo ya)"
fi

# 2. Berkas akhir berupa tautan.
mkdir -p luar2 tujuan2 && echo "asli" > luar2/sasaran && ln -s "$TMP/luar2/sasaran" tujuan2/a.txt
keluaran="$("$INDK" ekstrak.idk b.zip tujuan2)"
if [[ "$keluaran" == *"'a.txt' di tujuan adalah tautan; tidak ditimpa"* ]] &&
   [ "$(cat luar2/sasaran)" = "asli" ]; then
    lulus "tautan berkas di tujuan tidak ditimpa"
else
    gagal "tautan berkas: '$keluaran'; sasaran='$(cat luar2/sasaran)'"
fi

# 3. Tujuan itu sendiri tautan ke direktori: diizinkan.
mkdir -p nyata && ln -s "$TMP/nyata" tujuan3
keluaran="$("$INDK" ekstrak.idk a.tar.gz tujuan3)"
if [ "$keluaran" = "DIEKSTRAK" ] && [ "$(cat nyata/dir/a.txt)" = "isi a" ]; then
    lulus "tujuan yang berupa tautan ke direktori tetap bisa dipakai"
else
    gagal "tujuan tautan: '$keluaran'"
fi

# 4. arsip.buat() melewati tautan di sumber.
mkdir -p sumber && echo "x" > sumber/biasa.txt && ln -s /etc sumber/tautan_dir && ln -s "$TMP/luar2/sasaran" sumber/tautan_berkas
cat > buat_sumber.idk <<'IDK'
impor arsip
arsip.buat("s.tar", ["sumber"])
cetak([e["nama"] untuk e dalam arsip.daftar_isi("s.tar")])
IDK
keluaran="$("$INDK" buat_sumber.idk)"
if [ "$keluaran" = '["sumber/", "sumber/biasa.txt"]' ]; then
    lulus "arsip.buat() tidak menyimpan atau mengikuti tautan"
else
    gagal "buat dengan tautan: '$keluaran'"
fi

echo ""
echo "arsip-tautan: $LULUS lulus, $GAGAL gagal"
[ "$GAGAL" -eq 0 ]
