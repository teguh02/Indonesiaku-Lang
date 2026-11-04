#!/usr/bin/env bash
# Menjaga benchmark tetap bermakna.
#
# Benchmark lama mengukur `examples/performance_stress.idk`, yang isinya 4
# baris komentar dan satu `cetak` — 6 instruksi. Angka hijau yang mengukur
# program kosong lebih berbahaya daripada tidak punya benchmark sama sekali:
# ia menyembunyikan regresi dan sempat menyesatkan pemilihan target optimasi.
# Tes ini memastikan hal itu tidak terulang.
cd "$(dirname "$0")/.." || exit 1
BIN=./indk
[ -f "$BIN" ] || BIN=./indk.exe   # Windows (Git Bash)

# Di bawah ini sebuah beban dianggap tidak mengukur apa pun. Beban paling
# ringan saat ditulis ~3,5 juta instruksi, jadi ambang ini longgar.
MIN_INSTRUKSI=200000

lulus=0; gagal=0
periksa() {
    if [ "$1" = "ya" ]; then lulus=$((lulus+1)); else
        gagal=$((gagal+1)); echo "  x $2"
    fi
}

# 1. Direktori beban ada dan tidak kosong.
jumlah=$(ls tests/beban/*.idk 2>/dev/null | wc -l)
[ "$jumlah" -gt 0 ] && periksa ya || periksa tidak "tidak ada beban di tests/beban/"

# 2. Setiap beban benar-benar jalan DAN benar-benar mengeksekusi sesuatu.
for b in tests/beban/*.idk; do
    [ -f "$b" ] || continue
    nama=$(basename "$b" .idk)

    if "$BIN" "$b" >/dev/null 2>&1; then
        periksa ya
    else
        periksa tidak "$nama: keluar dengan galat"
        continue
    fi

    instr=$("$BIN" profil "$b" 2>&1 >/dev/null | awk '/instruksi :/{print $3}')
    if [ -n "$instr" ] && [ "$instr" -ge "$MIN_INSTRUKSI" ] 2>/dev/null; then
        periksa ya
    else
        periksa tidak "$nama: hanya ${instr:-0} instruksi (min $MIN_INSTRUKSI) — beban placeholder?"
    fi

    # 3. Nama beban tidak boleh bentrok dengan modul pustaka. `impor X`
    #    mencari direktori skrip LEBIH DULU, jadi tests/beban/teks.idk
    #    membuat `impor teks` memuat dirinya sendiri dan mati tanpa pesan
    #    (exit 70, stderr kosong). Ini benar-benar terjadi saat beban ditulis.
    if [ -f "pustaka/$nama.idk" ]; then
        periksa tidak "$nama: bentrok dengan pustaka/$nama.idk — ganti nama"
    else
        periksa ya
    fi
done

# 4. Benchmark jalan dan melaporkan setiap beban.
keluaran=$(bash tests/benchmark.sh 2>&1)
if [ $? -eq 0 ]; then periksa ya; else periksa tidak "benchmark.sh keluar dengan galat"; fi

for b in tests/beban/*.idk; do
    nama=$(basename "$b" .idk)
    case "$keluaran" in
        *"$nama"*) periksa ya ;;
        *) periksa tidak "benchmark.sh tidak melaporkan $nama" ;;
    esac
done

# 5. Kolom instruksi terisi angka, bukan kosong. Benchmark lama mencetak
#    baris kosong di macOS karena `date +%s.%N` dan `bc` tidak ada di sana;
#    kegagalan diam seperti itu tidak boleh lolos lagi.
if echo "$keluaran" | awk '/^  [a-z_]+ / { if ($2 + 0 > 0) found=1 } END { exit !found }'; then
    periksa ya
else
    periksa tidak "tidak ada angka instruksi di keluaran benchmark"
fi

echo "BENCHMARK: $lulus lulus, $gagal gagal"
[ "$gagal" -eq 0 ] || exit 1
