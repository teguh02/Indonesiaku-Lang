#!/usr/bin/env bash
# jaringan.unduh() terhadap server HTTP lokal (127.0.0.1, tanpa internet):
# badan biner 300 KB + checksum, pengalihan, Content-Length hilang/berbohong,
# 404, maks_ukuran, batal lewat kemajuan, dan tidak ada berkas parsial yang
# tertinggal. Server dan klien dua proses: klien memblokir saat menunggu.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
INDK="$AKAR/indk"
[ -x "$INDK" ] || INDK="$AKAR/indk.exe"
PORT="${IDK_PORT_UJI:-18781}"

cd "$AKAR" || exit 1

IDK_PORT="$PORT" IDK_SISA=40 "$INDK" tests/bantu/server_unduh.idk &
SERVER_PID=$!
trap 'kill "$SERVER_PID" 2>/dev/null' EXIT

for _ in $(seq 1 50); do
    if (exec 3<>/dev/tcp/127.0.0.1/"$PORT") 2>/dev/null; then
        exec 3<&- 2>/dev/null
        break
    fi
    sleep 0.1
done
# Probe di atas memakan satu kuota permintaan server; kuota dilebihkan.

KELUARAN="$(IDK_PORT="$PORT" "$INDK" tests/bantu/klien_unduh.idk 2>&1 | tr -d '\r'; exit "${PIPESTATUS[0]}")"
HASIL=$?

kill "$SERVER_PID" 2>/dev/null
wait "$SERVER_PID" 2>/dev/null
trap - EXIT

HARAPAN="biner       : 300008 benar benar application/octet-stream
pengalihan  : benar 300008 benar
tanpa panjang: isi tanpa panjang
sudah ada   : GalatUnduhan
timpa       : 17
404         : GalatUnduhan
sha salah   : GalatUnduhan
maks awal   : GalatUnduhan
maks aliran : GalatUnduhan
terpotong   : GalatUnduhan
putar       : GalatUnduhan
batal       : GalatUnduhan
batas waktu : Galat: soket_baca() melewati batas waktu menunggu data
isi akhir   : [\"alih.bin\", \"arsip ☕.bin\", \"kecil.txt\"]"

if [ "$HASIL" -ne 0 ]; then
    echo "GAGAL: klien keluar dengan kode $HASIL"
    echo "$KELUARAN"
    exit 1
fi

if [ "$KELUARAN" = "$HARAPAN" ]; then
    echo "ok   jaringan.unduh() terhadap server lokal"
    exit 0
fi

echo "GAGAL: keluaran tidak sesuai harapan"
diff <(echo "$HARAPAN") <(echo "$KELUARAN")
exit 1
