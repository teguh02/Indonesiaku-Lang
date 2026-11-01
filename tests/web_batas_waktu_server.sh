#!/usr/bin/env bash
# Batas waktu soket: server tidak boleh dibekukan oleh satu klien yang diam.
#
# Yang diuji di sini BUKAN "builtin bisa dipanggil", melainkan properti yang
# sebenarnya penting: setelah seorang penyerang menyambung dan tidak mengirim
# apa pun, permintaan berikutnya tetap dijawab. Tanpa soket_batas_waktu()
# pemeriksaan ini menggantung sampai timeout harness.

set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
INDK="$AKAR/indk"
PORT="${PORT_UJI:-8749}"
BATAS=1

lulus=0
gagal=0

periksa() {
    # periksa <nama> <diharapkan> <didapat>
    if [ "$2" = "$3" ]; then
        echo "  ok   $1"
        lulus=$((lulus + 1))
    else
        echo "  GAGAL $1: diharapkan [$2], didapat [$3]"
        gagal=$((gagal + 1))
    fi
}

LOG="$(mktemp)"
trap 'rm -f "$LOG"' EXIT

# SISA memperhitungkan SEMUA koneksi, termasuk probe kesiapan dan soket
# penyerang -- bukan hanya permintaan yang berhasil. Dibuat PAS (5) supaya
# server berhenti sendiri setelah pemeriksaan terakhir: stdout-nya baru
# ter-flush saat proses keluar, dan log itu yang membuktikan jalur batas
# waktu benar-benar diambil.
IDK_PORT="$PORT" IDK_SISA=5 IDK_BATAS="$BATAS" \
    "$INDK" "$AKAR/tests/bantu/server_batas_waktu.idk" >"$LOG" 2>&1 &
SERVER_PID=$!
trap 'kill $SERVER_PID 2>/dev/null; rm -f "$LOG"' EXIT

# Tunggu server siap, jangan tidur buta.
siap=0
for _ in $(seq 1 50); do
    if (exec 3<>"/dev/tcp/127.0.0.1/$PORT") 2>/dev/null; then
        exec 3<&- 2>/dev/null
        siap=1
        break
    fi
    sleep 0.1
done
periksa "server menyala" "1" "$siap"

minta() {
    # Satu permintaan GET /halo, cetak baris status. curl dipakai karena
    # ia menutup koneksi sendiri dan punya timeout sendiri.
    curl -s -m 8 -o /dev/null -w '%{http_code}' "http://127.0.0.1:$PORT/halo" 2>/dev/null
}

periksa "sebelum serangan dilayani" "200" "$(minta)"

# Penyerang: sambung, lalu DIAM. Soket dibiarkan terbuka di fd 9.
exec 9<>"/dev/tcp/127.0.0.1/$PORT" 2>/dev/null
sleep 0.2

# Inilah pemeriksaan intinya. Tanpa batas waktu, baris ini menggantung
# selamanya karena server masih menunggu byte pertama si penyerang.
periksa "tetap melayani saat ada klien diam" "200" "$(minta)"

exec 9<&- 2>/dev/null
periksa "masih melayani setelah penyerang pergi" "200" "$(minta)"

# Buktikan jalur batas waktu benar-benar diambil, bukan kebetulan penyerang
# menutup koneksi duluan (keduanya sama-sama membuat server lanjut).
# Tunggu server keluar sendiri; stdout baru ter-flush saat itu.
for _ in $(seq 1 50); do
    kill -0 $SERVER_PID 2>/dev/null || break
    sleep 0.1
done
if grep -q "melewati batas waktu" "$LOG"; then
    periksa "galat batas waktu tercatat" "ya" "ya"
else
    periksa "galat batas waktu tercatat" "ya" "tidak: $(tr '\n' ' ' <"$LOG")"
fi

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo ""
echo "batas waktu soket: $lulus lulus, $gagal gagal"
[ "$gagal" -eq 0 ]
