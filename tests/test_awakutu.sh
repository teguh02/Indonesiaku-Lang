#!/usr/bin/env bash
# `indk awakutu`: debugger CLI.
#
# Debugger diuji lewat pipa stdin: perintah masuk sebagai baris teks, dan
# keluarannya diperiksa. Yang dijaga di sini adalah sifat yang membuat
# debugger berguna — breakpoint benar-benar menghentikan, berhenti BERULANG
# di dalam loop, nilai yang ditampilkan ikut berubah, dan program tetap
# selesai dengan benar.
cd "$(dirname "$0")/.." || exit 1
BIN=./indk
[ -f "$BIN" ] || BIN=./indk.exe   # Windows (Git Bash)

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

berisi() {
    case "$2" in
        *"$3"*) lulus=$((lulus+1)) ;;
        *) gagal=$((gagal+1)); echo "  x $1 (tidak memuat '$3')" ;;
    esac
}

cat > "$TMP/program.idk" <<'EOF'
variabel total = 0

fungsi tambah(a, b) {
    kembalikan a + b
}

untuk i dalam rentang(0, 3) {
    total = tambah(total, i)
}

cetak(total)
EOF

# 1. Tanpa breakpoint apa pun, 'lanjut' menjalankan program sampai selesai
#    dan hasilnya sama dengan menjalankan biasa.
biasa=$("$BIN" "$TMP/program.idk" 2>/dev/null)
lewat_awakutu=$(printf 'lanjut\n' | "$BIN" awakutu "$TMP/program.idk" 2>/dev/null | tail -1)
periksa "hasil program tidak berubah di bawah debugger" "$lewat_awakutu" "$biasa"

# 2. Breakpoint menghentikan di baris yang diminta.
keluaran=$(printf 'henti 8\nlanjut\nlanjut\nlanjut\nlanjut\n' \
    | "$BIN" awakutu "$TMP/program.idk" 2>&1 >/dev/null)
berisi "breakpoint berhenti di baris 8" "$keluaran" "[baris 8]"

# 3. Breakpoint di dalam loop berhenti BERULANG, bukan sekali.
#    Ini pernah rusak: penanda "baris terakhir" tidak direset, sehingga
#    baris yang sama tidak pernah dianggap baru lagi.
berhenti=$(printf 'henti 8\nlanjut\nlanjut\nlanjut\nlanjut\n' \
    | "$BIN" awakutu "$TMP/program.idk" 2>&1 >/dev/null | grep -c '^\[baris 8\]')
if [ "$berhenti" -ge 3 ]; then
    lulus=$((lulus+1))
else
    gagal=$((gagal+1)); echo "  x breakpoint dalam loop hanya kena $berhenti kali (perlu >= 3)"
fi

# 3b. Satu baris berhenti TEPAT sekali per iterasi, bukan sekali per opcode.
#     Baris 8 memanggil fungsi, jadi eksekusi kembali ke baris itu setelah
#     fungsinya selesai; tanpa penjaga "opcode pertama baris ini", debugger
#     berhenti dua kali per iterasi dan `lihat` menampilkan nilai sebelum
#     hasil panggilan disimpan.
sekali=$(printf 'henti 8\nlanjut\nlanjut\nlanjut\nlanjut\nlanjut\nlanjut\nlanjut\n' \
    | "$BIN" awakutu "$TMP/program.idk" 2>&1 >/dev/null | grep -c '^\[baris 8\]')
periksa "berhenti tepat 3 kali untuk 3 iterasi" "$sekali" "3"

# 4. 'lihat' menampilkan variabel global, dan nilainya IKUT BERUBAH antar
#    iterasi. Tanpa cek perubahan, sebuah 'lihat' yang membeku di nilai awal
#    akan lolos.
nilai=$(printf 'henti 8\nlanjut\nlihat total\nlanjut\nlihat total\nlanjut\nlihat total\nlanjut\n' \
    | "$BIN" awakutu "$TMP/program.idk" 2>&1 >/dev/null \
    | sed -n 's/^(awakutu)  *total = \(.*\)$/total=\1/p')
unik=$(printf '%s\n' "$nilai" | sort -u | wc -l)
if [ "$unik" -ge 2 ]; then
    lulus=$((lulus+1))
else
    gagal=$((gagal+1)); echo "  x nilai 'total' tidak berubah antar iterasi: $nilai"
fi

# 4b. Urutan nilainya harus PERSIS 0, 0, 1 (akumulasi 0+0, lalu +1, lalu +2).
#     Tanpa cek nilai konkret, `lihat` yang jatuh ke tabel global yang salah
#     tetap mencetak sesuatu dan lolos.
periksa "urutan nilai total benar" "$(printf '%s' "$nilai" | tr '\n' ',')" "total=0,total=0,total=1"

# 5. 'lihat' menampilkan parameter fungsi di dalam frame fungsi itu.
par=$(printf 'henti 4\nlanjut\nlihat a\nlihat b\nlanjut\nlanjut\nlanjut\nlanjut\n' \
    | "$BIN" awakutu "$TMP/program.idk" 2>&1 >/dev/null)
berisi "lihat parameter a" "$par" "a = "
berisi "lihat parameter b" "$par" "b = "

# 6. 'tumpukan' menampilkan pemanggil, bukan hanya frame teratas.
tump=$(printf 'henti 4\nlanjut\ntumpukan\nlanjut\nlanjut\nlanjut\nlanjut\n' \
    | "$BIN" awakutu "$TMP/program.idk" 2>&1 >/dev/null)
berisi "tumpukan memuat fungsi" "$tump" "tambah"
# Wajib memuat frame #1 (pemanggil), bukan hanya menyebut kata "skrip" yang
# juga muncul di baris lain: sebuah tumpukan yang hanya mencetak frame
# teratas akan lolos cek substring biasa.
berisi "tumpukan memuat frame pemanggil" "$tump" "#1 "

# 7. Nama yang tidak dikenal ditolak dengan pesan, bukan diam atau crash.
tidak=$(printf 'henti 8\nlanjut\nlihat tidak_ada_ini\nlanjut\nlanjut\nlanjut\nlanjut\n' \
    | "$BIN" awakutu "$TMP/program.idk" 2>&1 >/dev/null)
berisi "nama tak dikenal dilaporkan" "$tidak" "tidak dikenal"

# 8. 'keluar' menghentikan program sebelum selesai: keluaran akhir tidak ada.
kel=$(printf 'henti 8\nlanjut\nkeluar\n' | "$BIN" awakutu "$TMP/program.idk" 2>/dev/null | tail -1)
if [ "$kel" = "$biasa" ]; then
    gagal=$((gagal+1)); echo "  x 'keluar' tetap menjalankan program sampai habis"
else
    lulus=$((lulus+1))
fi

# 9. 'hapus' melepas breakpoint: setelahnya program jalan tanpa berhenti lagi.
hapus=$(printf 'henti 8\nhapus 8\ndaftar\nlanjut\n' \
    | "$BIN" awakutu "$TMP/program.idk" 2>&1 >/dev/null)
berisi "hapus melepas breakpoint" "$hapus" "belum ada breakpoint"

# 10. Tanpa argumen berkas: pesan penggunaan, exit 64.
"$BIN" awakutu >/dev/null 2>&1
periksa "tanpa argumen -> exit 64" "$?" "64"

# 11. stdout program MURNI: tidak ada satu pun teks debugger yang bocor ke
#     sana, supaya `indk awakutu x.idk > hasil.txt` menghasilkan berkas yang
#     identik dengan menjalankan programnya biasa.
murni=$(printf 'henti 8\nlanjut\nlanjut\nlanjut\nlanjut\n' \
    | "$BIN" awakutu "$TMP/program.idk" 2>/dev/null)
periksa "stdout program murni (tanpa teks debugger)" "$murni" "$biasa"

echo "AWAKUTU: $lulus lulus, $gagal gagal"
[ "$gagal" -eq 0 ] || exit 1
