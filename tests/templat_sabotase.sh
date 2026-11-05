#!/usr/bin/env bash
# Sabotase pustaka/templat.idk: setiap jaminan dilucuti satu per satu, dan tes
# yang bersangkutan WAJIB berubah menjadi merah. Sebuah jaminan yang bisa
# dihapus tanpa membuat tes merah bukan pengaman, melainkan kode mati.
#
# Pemulihan memakai salinan di /tmp, BUKAN `git checkout`: repo biasanya
# memuat pekerjaan yang belum di-commit dan checkout akan menghapusnya.

set -u
cd "$(dirname "$0")/.." || exit 1

INDK="./indk"
PUSTAKA="pustaka/templat.idk"
GOLDEN="tests/expected/templat_dasar.out"
CONTOH="examples/templat_dasar.idk"
CADANGAN="$(mktemp)"
GAGAL=0
JUMLAH=0

cp "$PUSTAKA" "$CADANGAN"
pulihkan() { cp "$CADANGAN" "$PUSTAKA"; }
trap 'pulihkan; rm -f "$CADANGAN"' EXIT

bersihkan() { tr -d '\r'; }

terapkan() {
    python3 - "$PUSTAKA" "$1" "$2" <<'PY'
import sys
p, lama, baru = sys.argv[1], sys.argv[2], sys.argv[3]
t = open(p, encoding="utf-8").read()
open(p, "w", encoding="utf-8").write(t.replace(lama, baru, 1))
PY
}

# Baseline: tanpa sabotase harus hijau, kalau tidak seluruh skrip tak bermakna.
if ! diff -q <("$INDK" "$CONTOH" 2>/dev/null | bersihkan) \
             <(bersihkan < "$GOLDEN") >/dev/null; then
    echo "x baseline sudah merah sebelum sabotase apa pun"
    exit 1
fi

sabotase() {
    nama="$1"; lama="$2"; baru="$3"
    JUMLAH=$((JUMLAH + 1))
    pulihkan
    if ! grep -qF -- "$lama" "$PUSTAKA"; then
        echo "x $nama -- pola tidak ditemukan (pustaka berubah, perbarui sabotase ini)"
        GAGAL=$((GAGAL + 1))
        return
    fi
    terapkan "$lama" "$baru"
    if diff -q <("$INDK" "$CONTOH" 2>/dev/null | bersihkan) \
               <(bersihkan < "$GOLDEN") >/dev/null; then
        echo "x $nama -- LOLOS (tes tidak menangkapnya)"
        GAGAL=$((GAGAL + 1))
    else
        echo "+ $nama -- merah"
    fi
}

# 1. Escaping `<` dimatikan -> payload XSS lolos utuh ke HTML.
sabotase "escaping '<' dihapus" \
    '    h = ganti(h, "<", "&lt;")' \
    '    h = h'

# 2. Escaping kutip ganda dimatikan -> `" onload=` lolos keluar dari atribut.
sabotase "escaping kutip ganda dihapus" \
    '    h = ganti(h, "\"", "&quot;")' \
    '    h = h'

# 3. Urutan escaping dibalik: `&` di-escape terakhir, sehingga `&lt;` hasil
#    langkah sebelumnya ikut jadi `&amp;lt;`.
sabotase "urutan escaping dibalik" \
    '    variabel h = ganti(t, "&", "&amp;")' \
    '    variabel h = t'

# 4. Pagar SSR-03 dicabut: `{{ }}` boleh memuat kurung -> templat bisa jadi
#    jalan masuk pemanggilan fungsi.
sabotase "penolakan pemanggilan fungsi dicabut" \
    '    variabel terlarang = ["(", ")", "+", "-", "*", "/", "=", "<", ">", "!",' \
    '    variabel terlarang = ["+", "-", "*", "/", "=", "<", ">", "!",'

# 5. Variabel tidak ada tidak lagi menaikkan galat -> typo diam-diam kosong.
sabotase "galat variabel hilang diganti kosong" \
    '            jika !(bagian dalam kini) {' \
    '            jika salah {'

# 6. Pemeriksaan pasangan tag dicabut -> `{% untuk %}` tanpa penutup lolos.
sabotase "pemeriksaan tag tak tertutup dicabut" \
    '        jika titik == 0 - 1 {
            jika panjang(henti) > 0 {' \
    '        jika titik == 0 - 1 {
            jika salah {'

# 7. Penanda Aman diabaikan -> konten tepercaya ikut di-escape (regresi
#    diam-diam: halaman tetap tampil, tapi HTML-nya rusak jadi teks).
sabotase "penanda aman() diabaikan" \
    '                    jika adalah_instansi(v, Aman) {' \
    '                    jika salah {'

# 8. Isolasi variabel loop dicabut -> `{% untuk i %}` menimpa data pemanggil
#    dan bocor keluar blok.
sabotase "isolasi variabel loop dicabut" \
    '                        variabel lokal = kamus()
                        untuk k dalam kunci(data) { lokal[k] = data[k] }
                        lokal[n["nama"]] = item' \
    '                        variabel lokal = data
                        lokal[n["nama"]] = item'

# 9. Penolakan path traversal dicabut -> nama templat dari rute HTTP bisa
#    membaca berkas di luar akar.
sabotase "penolakan '..' pada nama templat dicabut" \
    '        jika ".." dalam nama {' \
    '        jika salah {'

pulihkan
echo
if [ "$GAGAL" -eq 0 ]; then
    echo "SABOTASE TEMPLAT: $JUMLAH/$JUMLAH merah seperti seharusnya"
    exit 0
fi
echo "SABOTASE TEMPLAT: $GAGAL dari $JUMLAH LOLOS -- jaminan itu tidak diuji"
exit 1
