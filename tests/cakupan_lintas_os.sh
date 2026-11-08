#!/usr/bin/env bash
# Penjaga cakupan tes lintas-OS.
#
# Dua celah nyata yang pernah terjadi di repo ini, keduanya tidak pernah
# memerahkan apa pun:
#
#   1. tests/test_todo_cli.ps1 dan tests/test_todo_server.ps1 ada di repo
#      berbulan-bulan TANPA PERNAH DIPANGGIL harness mana pun. Berkas tes yang
#      tidak dieksekusi memberi rasa aman palsu -- ia terlihat seperti cakupan.
#
#   2. Sub-suite .sh baru otomatis tidak jalan di Windows, karena Windows
#      memakai daftar EKSPLISIT di .github/workflows/test.yml, bukan glob.
#
# Tes ini memeriksa kedua hal itu secara mekanis. Sengaja TIDAK menuntut semua
# sub-suite .sh jalan di Windows: banyak yang memang Linux-only (docker, /proc,
# dpkg, valgrind). Yang dijaga adalah daftar izin di bawah -- menambah entri ke
# sana adalah keputusan sadar, sedangkan lupa mendaftarkan tes bukan.
#
# CATATAN untuk siapa pun yang menambah entri ke daftar izin: mendaftarkan tes
# ke Windows sering memerahkannya pertama kali karena indk.exe menulis CRLF,
# sedangkan $(...) hanya memangkas newline TERAKHIR. Saring dengan `tr -d '\r'`
# dan ambil kode keluar lewat ${PIPESTATUS[0]}, bukan $? (itu kode keluar tr,
# yang hampir selalu 0 dan menyembunyikan kegagalan).

set -u
cd "$(dirname "$0")/.."

GREEN='\033[0;32m'; RED='\033[0;31m'; NC='\033[0m'
GAGAL=0

lulus() { echo -e "  ${GREEN}+${NC} $1"; }
gagal() { echo -e "  ${RED}x${NC} $1"; GAGAL=$((GAGAL+1)); }

WF=".github/workflows/test.yml"
PS1_HARNESS="tests/test_all.ps1"

echo "Cakupan tes lintas-OS"

# --- 1. Tidak boleh ada .ps1 yatim ------------------------------------------
# Sebuah .ps1 dianggap terpakai bila namanya disebut oleh harness PowerShell
# ATAU oleh workflow CI. benchmark.ps1 dikecualikan: ia alat ukur manual,
# bukan gerbang lulus/gagal, dan memang tidak dipanggil otomatis.
ABAI_PS1="benchmark.ps1 test_all.ps1"
for p in tests/*.ps1; do
    nama="$(basename "$p")"
    case " $ABAI_PS1 " in *" $nama "*) continue ;; esac
    if grep -q "$nama" "$PS1_HARNESS" "$WF" 2>/dev/null; then
        lulus "dipanggil: $nama"
    else
        gagal "$nama ada di repo tapi tidak pernah dipanggil harness/CI"
    fi
done

# --- 2. Sub-suite yang WAJIB ikut jalan di Windows ---------------------------
# Daftar izin: tes perilaku bahasa/pustaka yang tidak butuh perkakas Linux.
# Menambah tes portabel baru? Daftarkan di sini DAN di workflow.
WAJIB_WINDOWS="
tests/builtin_konsisten.sh
tests/acak_aman.sh
tests/test_format.sh
tests/test_lint.sh
tests/test_uji.sh
tests/test_panduan.sh
tests/sandbox.sh
tests/web_kuki_sesi.sh
tests/web_csrf.sh
tests/web_auth.sh
tests/web_cors.sh
tests/keamanan_app.sh
tests/versi_konsisten.sh
tests/http_pengalihan_server.sh
"

# Baris bersambung (\ di ujung) digabung dulu, kalau tidak nama tes yang
# berada tepat sesudah '\' tidak pernah cocok.
WF_SATU_BARIS="$(sed -e :a -e '/\\$/N; s/\\\n\s*/ /; ta' "$WF")"

for t in $WAJIB_WINDOWS; do
    [ -f "$t" ] || { gagal "$t terdaftar wajib tapi berkasnya tidak ada"; continue; }
    if printf '%s' "$WF_SATU_BARIS" | grep -q "$t"; then
        lulus "jalan di Windows: $t"
    else
        gagal "$t tidak terdaftar di daftar Git Bash Windows ($WF)"
    fi
done

# --- 3. Setiap .sh wajib juga terdaftar di harness Linux ---------------------
for t in $WAJIB_WINDOWS; do
    if grep -q "$t" tests/test_all.sh; then
        lulus "jalan di Linux/macOS: $t"
    else
        gagal "$t tidak terdaftar di tests/test_all.sh"
    fi
done

if [ "$GAGAL" -eq 0 ]; then
    echo "CAKUPAN LINTAS-OS: semua terdaftar"
    exit 0
fi
echo "CAKUPAN LINTAS-OS: $GAGAL masalah" >&2
exit 1
