#!/usr/bin/env bash
# Modul berkas tidak pernah mengikuti tautan simbolik saat menyalin,
# menjelajah, atau menghapus pohon direktori (Fase 17). Butuh symlink
# sungguhan, jadi tidak bisa menjadi golden. Padanan Windows (junction):
# tests/test_berkas_windows.ps1.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$AKAR" || exit 1
INDK="./indk"
[ -x "$INDK" ] || INDK="./indk.exe"

case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*)
        echo "LEWAT: ln -s di Git Bash menyalin, bukan membuat tautan (lihat test_berkas_windows.ps1)"
        exit 0 ;;
esac

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/pohon" "$TMP/luar"
echo -n "a" > "$TMP/pohon/a.txt"
echo -n "jangan hilang" > "$TMP/luar/penting.txt"
ln -s "$TMP/luar" "$TMP/pohon/tautan"
ln -s "$TMP/pohon" "$TMP/akar_tautan"

KELUARAN="$(IDK_DIR="$TMP" "$INDK" tests/bantu/berkas_tautan.idk 2>&1)"
HASIL=$?

HARAPAN='jelajahi    : 1 [] ["a.txt", "tautan"]
info tautan : tautan
salin       : salin_direktori(): '"'"'pohon/tautan'"'"' tautan simbolik (pakai lewati_tautan=benar)
salin lewati: ["a.txt"]
akar tautan : hapus_direktori_bertingkat(): '"'"'akar_tautan'"'"' tautan simbolik
hapus pohon : ["penting.txt"] jangan hilang'

# Tautan file di posisi tujuan: menulis melaluinya bisa mendarat di mana saja.
ln -s "$TMP/luar/penting.txt" "$TMP/tautan_berkas"
echo -n "x" > "$TMP/asal.txt"
printf 'impor berkas\ncoba {\n    berkas.salin_berkas("%s/asal.txt", "%s/tautan_berkas", timpa=benar)\n} kecuali GalatOS sebagai e {\n    cetak("ditolak")\n}\n' "$TMP" "$TMP" > "$TMP/b.idk"
TUJUAN="$("$INDK" "$TMP/b.idk" 2>&1)"

gagal=0
if [ "$HASIL" -ne 0 ] || [ "$KELUARAN" != "$HARAPAN" ]; then
    echo "GAGAL: keluaran tidak sesuai (kode $HASIL)"
    diff <(echo "$HARAPAN") <(echo "$KELUARAN")
    gagal=1
fi
if [ "$TUJUAN" != "ditolak" ] || [ "$(cat "$TMP/luar/penting.txt")" != "jangan hilang" ]; then
    echo "GAGAL: salin_berkas menulis melalui tautan tujuan: $TUJUAN"
    gagal=1
fi
[ "$gagal" -eq 0 ] && echo "ok   berkas: tautan simbolik tidak diikuti"
exit "$gagal"
