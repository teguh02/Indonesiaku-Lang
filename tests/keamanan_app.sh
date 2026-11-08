#!/usr/bin/env bash
# Regresi keamanan menyeluruh (Fase 13 langkah 6) terhadap aplikasi nyata
# examples/catatan_app.idk — bukan terhadap fungsi pustaka satu per satu.
#
# Yang diuji adalah hal-hal yang hanya muncul saat semuanya disatukan:
# session fixation, CSRF lintas situs, izin lintas pengguna, XSS tersimpan,
# SQL injection, enumerasi nama pengguna, kuncian brute force, dan flag kuki.
#
# Lewat /dev/tcp, bukan curl, supaya jalan di Git Bash Windows.
set -u

AKAR="$(cd "$(dirname "$0")/.." && pwd)"
INDK="$AKAR/indk"
PORT="${IDK_PORT_KEAMANAN:-18841}"

cd "$AKAR" || exit 1

TMP="$(mktemp -d)"
DB="$AKAR/data_catatan_app.db"
bersihkan() {
    kill "$SERVER_PID" 2>/dev/null
    rm -rf "$TMP"
    rm -f "$DB"
}

# Basis data lama akan membawa pengguna yang sudah terkunci dari jalan
# sebelumnya; tes harus mulai dari keadaan bersih.
rm -f "$DB"

"$INDK" examples/catatan_app.idk "$PORT" >"$TMP/server.log" 2>&1 &
SERVER_PID=$!
trap bersihkan EXIT

for _ in $(seq 1 80); do
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

# kirim METODE JALUR KUKI BADAN [HEADER_TAMBAHAN]
kirim() {
    local metode="$1" jalur="$2" kuki="$3" badan="$4" tambahan="${5:-}"
    : > "$TMP/res"
    local p="$metode $jalur HTTP/1.1\r\nHost: 127.0.0.1\r\n"
    [ -n "$kuki" ] && p="${p}Cookie: sesi=$kuki\r\n"
    [ -n "$tambahan" ] && p="${p}${tambahan}\r\n"
    if [ -n "$badan" ]; then
        p="${p}Content-Type: application/x-www-form-urlencoded\r\n"
        p="${p}Content-Length: ${#badan}\r\n"
    fi
    p="${p}Connection: close\r\n\r\n$badan"
    exec 3<>/dev/tcp/127.0.0.1/"$PORT" || return 1
    printf "%b" "$p" >&3
    cat <&3 > "$TMP/res" 2>/dev/null
    exec 3<&- 2>/dev/null
}

status()  { head -1 "$TMP/res" | tr -d '\r' | cut -d' ' -f2; }
isi()     { tr -d '\r' < "$TMP/res"; }
sid_set() { tr -d '\r' < "$TMP/res" | sed -n 's/^Set-Cookie: sesi=\([0-9a-f]*\);.*/\1/p'; }
token()   { tr -d '\r' < "$TMP/res" \
              | grep -o 'name=_csrf value="[a-f0-9]*"' | head -1 \
              | sed 's/.*value="//;s/"//'; }

masuk() {   # masuk NAMA SANDI -> cetak sid
    kirim POST /masuk "" "nama=$1&sandi=$2"
    sid_set
}

echo "keamanan_app.sh: regresi keamanan aplikasi CRUD"

# --- 1. butuh sesi -----------------------------------------------------------
kirim GET / "" ""
periksa "tanpa sesi -> dialihkan" "303" "$(status)"

kirim POST /buat "" "judul=x&isi=y"
periksa "buat tanpa sesi -> 401" "401" "$(status)"

# --- 2. session fixation -----------------------------------------------------
SID_A="$(masuk admin rahasia123)"
periksa "login memberi sesi" "64" "${#SID_A}"

# Skenario sesungguhnya: penyerang menanam ID sesi pada korban, lalu korban
# login MEMBAWA ID itu. Login kedua harus mengirim kuki lama — tanpa itu yang
# teruji cuma "dua login terpisah menghasilkan ID berbeda", yang benar sendiri
# dan tidak membuktikan apa pun soal rotasi.
kirim POST /masuk "$SID_A" "nama=admin&sandi=rahasia123"
SID_B="$(sid_set)"
if [ "$SID_A" != "$SID_B" ]; then
    echo "  ok   ID sesi diputar saat login"
else
    echo "  GAGAL ID sesi TIDAK berubah — session fixation terbuka"
    GAGAL=1
fi

# ID lama harus mati begitu diputar, kalau tidak rotasi tidak ada gunanya.
kirim GET / "$SID_A" ""
periksa "ID sesi lama mati" "303" "$(status)"

# --- 3. flag kuki ------------------------------------------------------------
kirim POST /masuk "" "nama=admin&sandi=rahasia123"
KUKI_BARIS="$(tr -d '\r' < "$TMP/res" | grep -i '^Set-Cookie:')"
case "$KUKI_BARIS" in
    *HttpOnly*) echo "  ok   kuki HttpOnly" ;;
    *) echo "  GAGAL kuki tanpa HttpOnly — terbaca JavaScript"; GAGAL=1 ;;
esac
case "$KUKI_BARIS" in
    *SameSite=Strict*) echo "  ok   kuki SameSite=Strict" ;;
    *) echo "  GAGAL kuki tanpa SameSite"; GAGAL=1 ;;
esac

SID="$(echo "$KUKI_BARIS" | sed -n 's/.*sesi=\([0-9a-f]*\);.*/\1/p')"

# --- 4. CSRF -----------------------------------------------------------------
kirim GET / "$SID" ""
CSRF="$(token)"
periksa "token CSRF di formulir" "64" "${#CSRF}"

kirim POST /buat "$SID" "_csrf=$CSRF&judul=Halo&isi=dunia"
periksa "buat dengan token -> 303" "303" "$(status)"

# Inilah bentuk serangan sesungguhnya: kuki IKUT terkirim (peramban selalu
# melampirkannya), yang tidak dimiliki penyerang hanyalah tokennya.
kirim POST /buat "$SID" "_csrf=palsu&judul=jahat&isi=jahat"
periksa "token salah -> 403" "403" "$(status)"

kirim POST /buat "$SID" "judul=jahat&isi=jahat"
periksa "tanpa token -> 403" "403" "$(status)"

kirim GET / "$SID" ""
case "$(isi)" in
    *jahat*) echo "  GAGAL catatan dari serangan CSRF tersimpan"; GAGAL=1 ;;
    *) echo "  ok   serangan CSRF tidak meninggalkan data" ;;
esac

# --- 5. XSS tersimpan --------------------------------------------------------
kirim GET / "$SID" ""
CSRF="$(token)"
kirim POST /buat "$SID" "_csrf=$CSRF&judul=xss&isi=%3Cscript%3Ealert(1)%3C/script%3E"
periksa "simpan muatan XSS" "303" "$(status)"

kirim GET / "$SID" ""
case "$(isi)" in
    *"<script>alert"*) echo "  GAGAL <script> keluar mentah — XSS tersimpan"; GAGAL=1 ;;
    *"&lt;script&gt;"*) echo "  ok   muatan XSS ter-escape" ;;
    *) echo "  GAGAL muatan XSS tidak ditemukan sama sekali"; GAGAL=1 ;;
esac

# --- 6. SQL injection --------------------------------------------------------
# Kalau parameter tidak terikat, judul ini akan menutup tabelnya.
kirim GET / "$SID" ""
CSRF="$(token)"
kirim POST /buat "$SID" "_csrf=$CSRF&judul=x%27%29%3B+DROP+TABLE+catatan%3B--&isi=uji"
periksa "judul ber-SQL diterima apa adanya" "303" "$(status)"
kirim GET / "$SID" ""
periksa "tabel masih hidup sesudahnya" "200" "$(status)"
case "$(isi)" in
    *"DROP TABLE"*) echo "  ok   judul tersimpan sebagai teks, bukan perintah" ;;
    *) echo "  GAGAL judul ber-SQL hilang — mungkin tereksekusi"; GAGAL=1 ;;
esac

# --- 7. izin lintas pengguna -------------------------------------------------
# Catatan #1 milik admin. Budi tidak boleh menghapusnya walau tokennya sah.
SID_BUDI="$(masuk budi rahasia123)"
kirim GET / "$SID_BUDI" ""
CSRF_BUDI="$(token)"

kirim POST /hapus "$SID_BUDI" "_csrf=$CSRF_BUDI&id=1"
periksa "budi hapus catatan admin -> 403" "403" "$(status)"

# Token milik sesi LAIN juga tidak boleh menolong.
kirim GET / "$SID" ""
CSRF_ADMIN="$(token)"
kirim POST /hapus "$SID_BUDI" "_csrf=$CSRF_ADMIN&id=1"
periksa "token sesi lain -> 403" "403" "$(status)"

kirim GET / "$SID" ""
CSRF_ADMIN="$(token)"
kirim POST /hapus "$SID" "_csrf=$CSRF_ADMIN&id=1"
periksa "admin boleh menghapus -> 303" "303" "$(status)"

# --- 8. enumerasi nama pengguna ----------------------------------------------
kirim POST /masuk "" "nama=admin&sandi=salahsekali"
PESAN_ADA="$(isi | grep -o 'nama atau sandi salah' | head -1)"
ST_ADA="$(status)"
kirim POST /masuk "" "nama=tidakadaorangini&sandi=salahsekali"
PESAN_TIADA="$(isi | grep -o 'nama atau sandi salah' | head -1)"
periksa "pesan sama untuk nama asing" "$PESAN_ADA" "$PESAN_TIADA"
periksa "status sama untuk nama asing" "$ST_ADA" "$(status)"

# --- 9. kuncian brute force --------------------------------------------------
# Memakai nama tersendiri supaya tidak mengunci akun yang dipakai tes lain.
for i in 1 2 3 4 5; do
    kirim POST /masuk "" "nama=budi&sandi=tebakan$i"
done
kirim POST /masuk "" "nama=budi&sandi=tebakan6"
periksa "percobaan ke-6 -> 429" "429" "$(status)"

# Yang paling penting: kuncian tetap berlaku walau sandinya benar, kalau
# tidak penyerang cukup menebak terus sampai kena.
kirim POST /masuk "" "nama=budi&sandi=rahasia123"
periksa "sandi benar saat terkunci -> 429" "429" "$(status)"

# Pengguna lain tidak boleh ikut terkunci.
kirim POST /masuk "" "nama=admin&sandi=rahasia123"
periksa "pengguna lain tidak terkunci" "303" "$(status)"

if [ "$GAGAL" -eq 0 ]; then
    echo "keamanan_app.sh: SEMUA LULUS"
    exit 0
fi
echo "keamanan_app.sh: ADA YANG GAGAL"
exit 1
