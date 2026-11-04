#!/usr/bin/env bash
# `indk profil`: histogram opcode + hitung panggilan.
#
# Yang dijaga di sini adalah sifat yang membuat profil bisa dipercaya:
# laporan ke stderr (stdout program tetap bersih), hitungan deterministik,
# dan angkanya benar-benar mengikuti kerja program -- bukan konstanta.
cd "$(dirname "$0")/.." || exit 1
BIN=./indk
[ -x "$BIN" ] || BIN=./indk.exe   # Windows (Git Bash): make menghasilkan indk.exe

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
lulus=0; gagal=0

periksa() {
    if [ "$2" = "$3" ]; then
        lulus=$((lulus+1))
    else
        gagal=$((gagal+1))
        echo "  x $1 (didapat: '$2', diharapkan: '$3')"
    fi
}

cat > "$TMP/kerja.idk" <<'EOF'
variabel t = 0
untuk i dalam rentang(0, 1000) {
    t = t + i
}
cetak(t)
EOF

# 1. stdout HANYA berisi keluaran program; laporan profil pergi ke stderr.
keluaran=$("$BIN" profil "$TMP/kerja.idk" 2>/dev/null)
periksa "stdout bersih dari laporan profil" "$keluaran" "499500"

# 2. Laporan benar-benar muncul di stderr.
laporan=$("$BIN" profil "$TMP/kerja.idk" 2>&1 >/dev/null)
case "$laporan" in
    *"Profil eksekusi"*) lulus=$((lulus+1)) ;;
    *) gagal=$((gagal+1)); echo "  x laporan tidak ada di stderr" ;;
esac

# 3. Deterministik: dua jalanan atas masukan sama memberi angka sama.
a=$("$BIN" profil "$TMP/kerja.idk" 2>&1 >/dev/null | grep "instruksi :")
b=$("$BIN" profil "$TMP/kerja.idk" 2>&1 >/dev/null | grep "instruksi :")
periksa "hitungan deterministik" "$a" "$b"

# 4. Angkanya mengikuti kerja nyata: loop 10x lebih panjang harus menaikkan
#    jumlah instruksi. Tanpa ini, penghitung yang macet di konstanta lolos.
sed 's/rentang(0, 1000)/rentang(0, 10000)/' "$TMP/kerja.idk" > "$TMP/besar.idk"
n1=$("$BIN" profil "$TMP/kerja.idk" 2>&1 >/dev/null | awk '/instruksi :/{print $3}')
n2=$("$BIN" profil "$TMP/besar.idk" 2>&1 >/dev/null | awk '/instruksi :/{print $3}')
if [ "$n2" -gt "$n1" ] 2>/dev/null; then
    lulus=$((lulus+1))
else
    gagal=$((gagal+1)); echo "  x instruksi tidak naik saat kerja bertambah ($n1 -> $n2)"
fi

# 5. Hitungan panggilan mengikuti jumlah panggilan fungsi.
cat > "$TMP/panggil.idk" <<'EOF'
fungsi f(x) { kembalikan x }
variabel i = 0
selagi i < 50 {
    f(i)
    i = i + 1
}
EOF
p=$("$BIN" profil "$TMP/panggil.idk" 2>&1 >/dev/null | awk '/panggilan :/{print $3}')
# 50 panggilan f() + 1 untuk skrip utama.
periksa "hitungan panggilan" "$p" "51"

# 6. Opcode yang jelas harus dipakai memang muncul di histogram.
hist=$("$BIN" profil "$TMP/kerja.idk" 2>&1 >/dev/null)
case "$hist" in
    *OP_LOOP*) lulus=$((lulus+1)) ;;
    *) gagal=$((gagal+1)); echo "  x OP_LOOP tidak muncul padahal ada perulangan" ;;
esac

# 7. Tanpa argumen berkas: pesan penggunaan, exit 64.
"$BIN" profil >/dev/null 2>&1
periksa "tanpa argumen -> exit 64" "$?" "64"

# 8. Kode keluar skrip diteruskan apa adanya (galat tetap galat).
printf 'naikkan Galat("x")\n' > "$TMP/galat.idk"
"$BIN" profil "$TMP/galat.idk" >/dev/null 2>&1
periksa "kode keluar diteruskan" "$?" "70"

echo "PROFIL: $lulus lulus, $gagal gagal"
[ "$gagal" -eq 0 ] || exit 1
