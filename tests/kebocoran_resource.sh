#!/usr/bin/env bash
# Deteksi kebocoran resource: apakah fd server bertambah di bawah beban?
#
# Yang dicari BUKAN jalur sukses -- itu jarang bocor. Yang bocor adalah jalur
# galat: penangan yang melempar, klien yang putus sebelum balasan terkirim,
# dan badan besar. Kalau soket tidak ditutup di salah satu jalur itu, jumlah
# fd naik tiap putaran dan server akhirnya mati dengan EMFILE.
#
# Tes ini membandingkan fd SESUDAH pemanasan dengan fd sesudah beban, bukan
# dengan fd saat start: koneksi pertama mengalokasikan buffer yang wajar dan
# tetap, dan itu bukan kebocoran.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
INDK="$AKAR/indk"
PORT="${PORT_UJI:-8849}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"; [ -n "${SRV:-}" ] && kill "$SRV" 2>/dev/null' EXIT

if [ ! -x "$INDK" ]; then
    echo "LEWAT: $INDK belum dibangun"
    exit 0
fi

# /proc hanya ada di Linux. Di tempat lain tes ini melewati diri sendiri
# daripada memberi angka palsu.
if [ ! -d /proc/self/fd ]; then
    echo "LEWAT: /proc tidak tersedia (bukan Linux)"
    exit 0
fi

cat > "$TMP/srv.idk" <<'EOF'
impor web

variabel app = web.Aplikasi()

app.rute("GET", "/", fungsi(req) {
    kembalikan "ok"
})

# Penangan yang MELEDAK. Soketnya tetap harus ditutup.
app.rute("GET", "/ledak", fungsi(req) {
    kembalikan 1 / 0
})

app.rute("POST", "/besar", fungsi(req) {
    kembalikan ke_teks(panjang(req["badan"]))
})

variabel PORT = ke_bulat(ambil_lingkungan("IDK_PORT"))
variabel SISA = ke_bulat(ambil_lingkungan("IDK_SISA"))

# batas_waktu 1 detik: klien yang sengaja diputus tidak boleh menahan gelung
# lama-lama, kalau tidak tes ini jadi lambat tanpa menambah bukti.
app.jalankan(PORT, 1, SISA, salah)
EOF

IDK_PORT="$PORT" IDK_SISA=600 "$INDK" "$TMP/srv.idk" >/dev/null 2>&1 &
SRV=$!
sleep 2

if ! kill -0 "$SRV" 2>/dev/null; then
    echo "GAGAL: server tidak menyala"
    exit 1
fi

fd_count() { ls "/proc/$SRV/fd" 2>/dev/null | wc -l; }

beban() { # jumlah per jenis
    local n="$1" i=0
    while [ "$i" -lt "$n" ]; do
        curl -s --max-time 5 "http://127.0.0.1:$PORT/ledak" >/dev/null 2>&1
        # Putus di tengah: kirim header lalu tutup tanpa badan, sehingga
        # server melihat koneksi mati di tengah permintaan.
        #
        # Memakai curl --max-time kecil di sini membuat tes 4x lebih lambat:
        # curl pergi tetapi SOKETNYA tetap terbuka, jadi gelung server
        # menunggu batas waktu penuh sebelum menutup. Dengan exec 3<> soket
        # benar-benar ditutup, dan jalur galat yang sama tetap terlewati.
        exec 3<>"/dev/tcp/127.0.0.1/$PORT" 2>/dev/null && {
            printf 'GET / HTTP/1.1\r\nHost: x\r\n' >&3 2>/dev/null
            exec 3<&- 2>/dev/null
            exec 3>&- 2>/dev/null
        }
        curl -s --max-time 10 --data-binary @"$TMP/besar.bin" \
             "http://127.0.0.1:$PORT/besar" >/dev/null 2>&1
        i=$((i + 1))
    done
}

head -c 102400 /dev/zero | tr '\0' 'x' > "$TMP/besar.bin"

# Pemanasan: alokasi awal yang wajar terjadi di sini, bukan dihitung bocor.
beban 10
dasar="$(fd_count)"

beban 30
sesudah="$(fd_count)"

kill "$SRV" 2>/dev/null
SRV=""

echo "  fd sesudah pemanasan : $dasar"
echo "  fd sesudah 90 beban  : $sesudah"

# Ambang 2: fd boleh bergoyang sedikit karena soket di TIME_WAIT, tetapi
# kebocoran sejati tumbuh sebanding jumlah permintaan (puluhan), bukan 1-2.
if [ "$sesudah" -gt $((dasar + 2)) ]; then
    echo "  GAGAL kebocoran fd: $dasar -> $sesudah setelah 90 permintaan"
    echo "=================================================="
    echo "kebocoran_resource: 0 lulus, 1 gagal"
    exit 1
fi

echo "  ok   tidak ada kebocoran fd di jalur galat, putus, dan badan besar"
echo "=================================================="
echo "kebocoran_resource: 1 lulus, 0 gagal"
exit 0
