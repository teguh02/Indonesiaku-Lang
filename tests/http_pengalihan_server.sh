#!/usr/bin/env bash
# Menguji http_ambil() mengikuti pengalihan terhadap server HTTP sungguhan.
# Server dan klien dijalankan sebagai dua proses terpisah: klien memblokir
# saat menunggu balasan, jadi keduanya tidak bisa berada di satu proses.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
INDK="$AKAR/indk"
PORT="${IDK_PORT_UJI:-18779}"

# Jumlah permintaan yang akan diterima server, supaya ia berhenti sendiri:
#   /mulai->/tengah->/akhir      3
#   /lihat->/akhir               2
#   /tetap->/akhir               2
#   /mulai dengan maks 0         1
#   /mulai->/tengah (batas 1)    2
#   /putar->/putar2 (loop)       2
#   /tanpa                       1
# Dilebihkan: server berhenti saat kuota habis, dan kuota yang kurang
# membuat permintaan terakhir ditolak -- kelebihan kuota tidak berbahaya
# karena skrip ini mematikan server lewat trap.
SISA=20

cd "$AKAR" || exit 1

IDK_PORT="$PORT" IDK_SISA="$SISA" "$INDK" tests/bantu/server_pengalihan.idk &
SERVER_PID=$!
# Pastikan server dimatikan walau tes gagal di tengah jalan.
trap 'kill "$SERVER_PID" 2>/dev/null' EXIT

# Tunggu port benar-benar menerima koneksi, jangan tidur buta.
for _ in $(seq 1 50); do
    if (exec 3<>/dev/tcp/127.0.0.1/"$PORT") 2>/dev/null; then
        exec 3<&- 2>/dev/null
        break
    fi
    sleep 0.1
done

# indk.exe menulis CRLF, dan $(...) hanya memangkas newline TERAKHIR -- setiap
# baris lain tetap berakhir '\r', sehingga seluruh perbandingan gagal di
# Windows walau nilainya benar. Saring sebelum membandingkan.
#
# PIPESTATUS, bukan $?: dengan pipa ke tr, $? adalah kode keluar tr (selalu 0)
# sehingga klien yang jatuh akan lolos tanpa terdeteksi.
KELUARAN="$(IDK_PORT="$PORT" "$INDK" tests/bantu/klien_pengalihan.idk 2>&1 | tr -d '\r'; exit "${PIPESTATUS[0]}")"
HASIL=$?

# Kuota sengaja dilebihkan, jadi server masih menunggu permintaan berikutnya.
# Matikan langsung; jangan "wait", itu akan menggantung.
kill "$SERVER_PID" 2>/dev/null
wait "$SERVER_PID" 2>/dev/null
trap - EXIT

HARAPAN="status      : 200
badan       : metode=GET
url akhir   : http://127.0.0.1:$PORT/akhir
303         : metode=GET
307         : metode=POST
tanpa ikut  : 301
batas       : ditolak
loop        : ditolak
tanpa Lokasi: 301"

if [ "$HASIL" -ne 0 ]; then
    echo "GAGAL: klien keluar dengan kode $HASIL"
    echo "$KELUARAN"
    exit 1
fi

if [ "$KELUARAN" = "$HARAPAN" ]; then
    echo "ok   http_ambil() mengikuti pengalihan"
    exit 0
fi

echo "GAGAL: keluaran tidak sesuai harapan"
diff <(echo "$HARAPAN") <(echo "$KELUARAN")
exit 1
