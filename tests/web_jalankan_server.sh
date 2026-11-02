#!/usr/bin/env bash
# Gelung server bawaan pustaka: app.jalankan().
#
# Yang diuji adalah properti operasionalnya lewat HTTP sungguhan, bukan
# sekadar "metodenya ada": batas waktu aktif TANPA disetel pemakai, satu
# penangan yang meledak tidak mematikan server, dan berhenti() melepas port
# setelah balasan terkirim.

set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
INDK="$AKAR/indk"
PORT="${PORT_UJI:-8771}"

lulus=0
gagal=0

periksa() {
    if [ "$2" = "$3" ]; then
        echo "  ok   $1"
        lulus=$((lulus + 1))
    else
        echo "  GAGAL $1: diharapkan [$2], didapat [$3]"
        gagal=$((gagal + 1))
    fi
}

LOG="$(mktemp)"

# Kuota HARUS jauh lebih besar dari jumlah koneksi yang dibuat tes ini
# (termasuk setiap probe /dev/tcp, yang juga dihitung server). Dengan kuota
# pas-pasan, server keluar karena kuota habis dan pemeriksaan berhenti() lolos
# padahal berhenti() rusak -- itu benar-benar terjadi saat sabotase.
IDK_PORT="$PORT" IDK_SISA=5000 \
    "$INDK" "$AKAR/tests/bantu/server_jalankan.idk" >"$LOG" 2>&1 &
SERVER_PID=$!
trap 'kill $SERVER_PID 2>/dev/null; rm -f "$LOG"' EXIT

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
    curl -s -m 8 -o /dev/null -w '%{http_code}' "http://127.0.0.1:$PORT$1" 2>/dev/null
}

periksa "permintaan normal dilayani" "200" "$(minta /halo)"

# Penangan yang menaikkan galat harus jadi 500, bukan mematikan server.
periksa "penangan yang meledak jadi 500" "500" "$(minta /meledak)"
periksa "server hidup setelah penangan meledak" "200" "$(minta /halo)"

# Batas waktu: TIDAK disetel oleh server_jalankan.idk lewat soket_batas_waktu,
# melainkan datang dari jalankan() itu sendiri. Klien diam tidak boleh
# membekukan server.
exec 9<>"/dev/tcp/127.0.0.1/$PORT" 2>/dev/null
sleep 0.2
periksa "klien diam tidak membekukan server" "200" "$(minta /halo)"
exec 9<&- 2>/dev/null

# Shutdown anggun: balasan /matikan harus TETAP sampai ke klien.
periksa "balasan /matikan tetap terkirim" "200" "$(minta /matikan)"

# ...dan sesudah itu port benar-benar dilepas.
tutup=0
for _ in $(seq 1 50); do
    if ! (exec 3<>"/dev/tcp/127.0.0.1/$PORT") 2>/dev/null; then
        tutup=1
        break
    fi
    exec 3<&- 2>/dev/null
    sleep 0.1
done
periksa "port dilepas setelah berhenti()" "1" "$tutup"

# Server berakhir sendiri, bukan dibunuh.
keluar_sendiri=0
for _ in $(seq 1 50); do
    kill -0 $SERVER_PID 2>/dev/null || { keluar_sendiri=1; break; }
    sleep 0.1
done
periksa "proses server keluar sendiri" "1" "$keluar_sendiri"

# Log: field-nya harus bisa diurai, dan batas waktu tercatat sebagai
# PERINGATAN, bukan diam-diam ditelan.
if grep -qE '^INFO:web:GET /halo 200 [0-9.]+ms$' "$LOG"; then
    periksa "baris log permintaan berformat tetap" "ya" "ya"
else
    periksa "baris log permintaan berformat tetap" "ya" "tidak"
fi

if grep -q 'PERINGATAN:web:koneksi dilewati' "$LOG"; then
    periksa "batas waktu tercatat sebagai peringatan" "ya" "ya"
else
    periksa "batas waktu tercatat sebagai peringatan" "ya" "tidak"
fi

# Galat penangan tidak boleh menempel ke baris log berikutnya (tulis_galat
# tidak menambah baris baru sendiri).
# `.*` bukan `.+`: saat bug-nya muncul kedua teks bersebelahan TANPA
# karakter di antaranya, sehingga `.+` tidak pernah cocok dan pemeriksaan
# ini selalu hijau -- terbukti saat sabotase.
if grep -qE 'sengaja meledak.*GALAT:web' "$LOG"; then
    periksa "pesan galat tidak menempel ke baris log" "terpisah" "menempel"
else
    periksa "pesan galat tidak menempel ke baris log" "terpisah" "terpisah"
fi

# Badan dan header TIDAK boleh muncul di log.
if grep -qi 'Host:' "$LOG"; then
    periksa "header tidak ikut tercatat" "bersih" "ada header"
else
    periksa "header tidak ikut tercatat" "bersih" "bersih"
fi

# Catatan sengaja: menghapus soket_tutup(server) TIDAK bisa dideteksi dari
# luar di sini, karena proses server langsung keluar setelah gelung dan OS
# menutup semua fd-nya. Pemeriksaan untuk itu hanya akan hijau selamanya, jadi
# tidak ditambahkan. soket_tutup(server) tetap benar untuk program yang
# memanggil jalankan() lalu lanjut melakukan hal lain.

echo ""
echo "gelung server: $lulus lulus, $gagal gagal"
[ "$gagal" -eq 0 ]
