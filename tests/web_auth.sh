#!/usr/bin/env bash
# Membuktikan autentikasi dan otorisasi (WEB-06) bekerja lewat HTTP nyata:
# daur masuk/keluar, pemisahan 401 vs 403, dan pembatas percobaan.
#
# Golden examples/web_auth.idk menguji fungsinya satu per satu. Yang dibuktikan
# di sini: alurnya utuh dari sisi klien — kuki sesi dari /masuk benar-benar
# membuka /admin, dan hilang daya setelah /keluar.
#
# Lewat /dev/tcp, bukan curl, supaya jalan di Git Bash Windows.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
INDK="$AKAR/indk"
PORT="${IDK_PORT_AUTH:-18801}"
# 1 probe + 15 permintaan. Hitung tepat: bila SISA kurang, server berhenti
# lebih awal, koneksi ditolak, dan $TMP/res masih memuat respons SEBELUMNYA —
# tesnya lalu membandingkan jawaban permintaan yang salah.
SISA=16

cd "$AKAR" || exit 1

TMP="$(mktemp -d)"
bersihkan() {
    kill "$SERVER_PID" 2>/dev/null
    rm -rf "$TMP"
}

IDK_PORT="$PORT" IDK_SISA="$SISA" "$INDK" tests/bantu/server_auth.idk 2>/dev/null &
SERVER_PID=$!
trap bersihkan EXIT

for _ in $(seq 1 50); do
    if (exec 3<>/dev/tcp/127.0.0.1/"$PORT") 2>/dev/null; then
        exec 3<&- 2>/dev/null
        break
    fi
    sleep 0.1
done

GAGAL=0
periksa() {
    local nama="$1" harap="$2" dapat="$3"
    if [ "$harap" = "$dapat" ]; then
        echo "  ok   $nama"
    else
        echo "  GAGAL $nama: harap '$harap', dapat '$dapat'"
        GAGAL=1
    fi
}

kirim() {
    local metode="$1" jalur="$2" tambahan="$3" badan="$4"
    # Dikosongkan dulu: tanpa ini, koneksi yang gagal meninggalkan respons
    # permintaan SEBELUMNYA di tempatnya dan tes memeriksa jawaban yang salah.
    : > "$TMP/res"
    local permintaan="$metode $jalur HTTP/1.1\r\nHost: 127.0.0.1\r\n"
    if [ -n "$badan" ]; then
        permintaan="${permintaan}Content-Type: application/x-www-form-urlencoded\r\n"
        permintaan="${permintaan}Content-Length: ${#badan}\r\n"
    fi
    if [ -n "$tambahan" ]; then
        permintaan="$permintaan$tambahan\r\n"
    fi
    permintaan="$permintaan\r\n$badan"
    exec 3<>/dev/tcp/127.0.0.1/"$PORT" || return 1
    printf "%b" "$permintaan" >&3
    cat <&3 > "$TMP/res" 2>/dev/null
    exec 3<&- 2>/dev/null
}

status() { head -1 "$TMP/res" | tr -d '\r' | cut -d' ' -f2; }
badan()  { tr -d '\r' < "$TMP/res" | awk 'k { print } /^$/ { k = 1 }'; }

# --- 1. belum masuk ----------------------------------------------------------
kirim GET /admin "" ""
periksa "tanpa sesi -> 401" "401" "$(status)"

# --- 2. sandi salah ----------------------------------------------------------
kirim POST /masuk "" "nama=ani&sandi=salah"
periksa "sandi salah -> 401" "401" "$(status)"
# Pesan untuk nama tak dikenal harus SAMA, supaya tidak membocorkan nama mana
# yang benar-benar terdaftar.
PESAN_SALAH="$(badan)"
kirim POST /masuk "" "nama=tidakada&sandi=apa"
periksa "nama asing: pesan sama" "$PESAN_SALAH" "$(badan)"

# --- 3. masuk sebagai admin --------------------------------------------------
kirim POST /masuk "" "nama=ani&sandi=rahasia123"
periksa "masuk benar -> 200" "200" "$(status)"
SID_ADMIN="$(tr -d '\r' < "$TMP/res" | sed -n 's/^Set-Cookie: sesi=\([0-9a-f]*\);.*/\1/p')"
periksa "dapat kuki sesi" "64" "${#SID_ADMIN}"

kirim GET /admin "Cookie: sesi=$SID_ADMIN" ""
periksa "admin buka /admin" "200" "$(status)"

# --- 4. pengguna biasa: 403, bukan 401 ---------------------------------------
kirim POST /masuk "" "nama=budi&sandi=sandibudi"
SID_BIASA="$(tr -d '\r' < "$TMP/res" | sed -n 's/^Set-Cookie: sesi=\([0-9a-f]*\);.*/\1/p')"
periksa "budi masuk" "200" "$(status)"

kirim GET /beranda "Cookie: sesi=$SID_BIASA" ""
periksa "budi buka /beranda" "200" "$(status)"

# Sudah masuk tapi tidak berhak -> 403. Membedakannya dari 401 penting:
# 401 berarti "masuk dulu", 403 berarti "percuma mencoba lagi".
kirim GET /admin "Cookie: sesi=$SID_BIASA" ""
periksa "budi ditolak /admin" "403" "$(status)"

# --- 5. keluar membatalkan sesi ----------------------------------------------
kirim POST /keluar "Cookie: sesi=$SID_ADMIN" ""
periksa "keluar -> 200" "200" "$(status)"
kirim GET /admin "Cookie: sesi=$SID_ADMIN" ""
periksa "sesudah keluar -> 401" "401" "$(status)"

# --- 6. pembatas percobaan ---------------------------------------------------
# Tiga kegagalan mengunci; permintaan keempat ditolak 429 BAHKAN dengan sandi
# yang benar, membuktikan pembatas berjalan sebelum verifikasi.
kirim POST /masuk "" "nama=budi&sandi=x1"
kirim POST /masuk "" "nama=budi&sandi=x2"
kirim POST /masuk "" "nama=budi&sandi=x3"
kirim POST /masuk "" "nama=budi&sandi=sandibudi"
periksa "terkunci -> 429" "429" "$(status)"

# Pengguna lain tidak ikut terkunci.
kirim POST /masuk "" "nama=ani&sandi=rahasia123"
periksa "pengguna lain bebas" "200" "$(status)"

wait "$SERVER_PID" 2>/dev/null

if [ "$GAGAL" -eq 0 ]; then
    echo "web_auth: semua lulus"
else
    echo "web_auth: ADA YANG GAGAL"
fi
exit "$GAGAL"
