#!/usr/bin/env bash
# Sabotase pustaka/migrasi.idk: setiap jaminan dilucuti satu per satu, dan tes
# yang bersangkutan WAJIB berubah menjadi merah. Sebuah jaminan yang bisa
# dihapus tanpa membuat tes merah bukan pengaman, melainkan kode mati.
#
# Pemulihan memakai salinan di /tmp, BUKAN `git checkout`: repo biasanya
# memuat pekerjaan yang belum di-commit dan checkout akan menghapusnya.

set -u
cd "$(dirname "$0")/.." || exit 1

INDK="./indk"
PUSTAKA="pustaka/migrasi.idk"
GOLDEN="tests/expected/migrasi_skema.out"
CONTOH="examples/migrasi_skema.idk"
CADANGAN="$(mktemp)"
GAGAL=0
JUMLAH=0

cp "$PUSTAKA" "$CADANGAN"
pulihkan() { cp "$CADANGAN" "$PUSTAKA"; }
trap 'pulihkan; rm -f "$CADANGAN" /tmp/idk_sab_galat.txt' EXIT

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

# Sabotase yang dijaga golden examples/migrasi_skema.idk.
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

# Sabotase yang dijaga tests/galat/, bukan golden: bandingkan stderr terhadap
# berkas .err persis seperti harness tes galat.
sabotase_galat() {
    nama="$1"; kasus="$2"; lama="$3"; baru="$4"
    JUMLAH=$((JUMLAH + 1))
    pulihkan
    if ! grep -qF -- "$lama" "$PUSTAKA"; then
        echo "x $nama -- pola tidak ditemukan (pustaka berubah, perbarui sabotase ini)"
        GAGAL=$((GAGAL + 1))
        return
    fi
    terapkan "$lama" "$baru"
    "$INDK" "tests/galat/$kasus.idk" 2>/tmp/idk_sab_galat.txt 1>/dev/null
    keluar=$?
    if [ "$keluar" -ne 0 ] && diff -q <(bersihkan < /tmp/idk_sab_galat.txt) \
                                      <(bersihkan < "tests/galat/$kasus.err") >/dev/null; then
        echo "x $nama -- LOLOS (tes tidak menangkapnya)"
        GAGAL=$((GAGAL + 1))
    else
        echo "+ $nama -- merah"
    fi
}

# 1. Versi tidak dicatat -> migrasi jalan ulang setiap kali.
sabotase "pencatatan versi dihapus" \
    '                _catat(k, m)' \
    '                variabel abaikan_catat = 0'

# 2. Tidak memeriksa versi terpasang -> DDL diulang dan galat.
sabotase "pemeriksaan versi terpasang dilewati" \
    'jika (m["versi"] dalam sudah) == salah {' \
    'jika benar {'

# 3. Rollback tidak menghapus catatan versi -> status tetap terpasang.
sabotase "penghapusan catatan saat rollback dihapus" \
    '                k.eksekusi("DELETE FROM " + TABEL + " WHERE versi = ?", [m["versi"]])' \
    '                variabel abaikan_hapus = 0'

# 4. turunkan_satu mengambil yang TERLAMA, bukan terbaru.
sabotase "urutan rollback dibalik" \
    '    variabel i = panjang(urut) - 1
    selagi i >= 0 {' \
    '    variabel i = 0
    selagi i < panjang(urut) {'

# 5. Pengurutan versi dibuang -> ALTER jalan sebelum CREATE.
sabotase "pengurutan versi dibuang" \
    '    untuk v dalam terurut(versi) {' \
    '    untuk v dalam versi {'

# 6. Penolakan turun kosong dihapus -> catatan versi hilang tanpa ubah skema.
sabotase_galat "penolakan migrasi tanpa turun dihapus" "migrasi_tanpa_turun" \
    '            jika m["turun"] == kosong {' \
    '            jika salah {'

# 7. Penolakan versi ganda dihapus.
sabotase_galat "penolakan versi ganda dihapus" "migrasi_versi_ganda" \
    '        jika v dalam terlihat {' \
    '        jika salah {'

pulihkan
echo "SABOTASE: $((JUMLAH - GAGAL))/$JUMLAH merah"
[ "$GAGAL" -eq 0 ]
