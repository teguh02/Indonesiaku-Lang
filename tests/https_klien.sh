#!/usr/bin/env bash
# Uji klien HTTPS terhadap host sungguhan.
#
# Yang membuktikan TLS benar bukan "berhasil mengambil halaman" -- itu juga
# terjadi kalau verifikasi dimatikan. Yang membuktikannya adalah MENOLAK
# sertifikat yang tidak sah, jadi sebagian besar tes di bawah adalah tes
# negatif memakai badssl.com (host uji publik yang memang disediakan untuk ini).
#
# Tes ini MELEWATI diri sendiri bila jaringan tidak tersedia. Alasannya: CI
# yang merah karena DNS mati melatih orang mengabaikan CI.
#
# Sabotase yang sudah diuji terhadap berkas ini (3 dari 4 membuatnya merah):
#   - MBEDTLS_SSL_VERIFY_REQUIRED -> VERIFY_NONE   : merah (8 pemeriksaan)
#   - SNI tidak dikirim (set_hostname NULL)        : merah
#   - muatCa() gagal tetapi handshake diteruskan   : merah
#   - pemeriksaan ulang verify_result dihapus      : TETAP HIJAU
#
# Yang terakhir memang tidak teramati dari luar: selama VERIFY_REQUIRED aktif,
# mbedtls_ssl_handshake() sudah gagal lebih dulu, jadi pemeriksaan kedua di
# tls.c murni pertahanan berlapis untuk menjaga perubahan konfigurasi di masa
# depan. Menambah "tes" yang hanya bisa hijau tidak membuktikan apa pun, jadi
# sengaja tidak ditambahkan.
#
# Catatan soal "EOF": badssl.com membatasi laju. Saat dihantam berkali-kali ia
# memutus koneksi sebelum sertifikat sempat dinilai, dan pesan kita berubah
# dari sebab spesifik menjadi "The connection indicated an EOF". Ini sempat
# disangka regresi akibat mencabut CCM; A/B tiga biner (MinGW dengan CCM,
# MinGW tanpa CCM, Linux tanpa CCM) membuktikan ketiganya berperilaku sama,
# jadi penyebabnya murni jaringan. Karena itu "DITOLAK" dan "sebab spesifik"
# dipisah menjadi dua pemeriksaan: yang pertama tidak boleh longgar sedikit
# pun, yang kedua memaafkan galat transport.
set -u

akar="$(cd "$(dirname "$0")/.." && pwd)"
indk="$akar/indk"
[ -x "$indk" ] || indk="$akar/indk.exe"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

lulus=0
gagal=0

periksa() { # nama, teks_yang_harus_ada, keluaran
    if printf '%s' "$3" | grep -qF "$2"; then
        echo "  ok   $1"
        lulus=$((lulus + 1))
    else
        echo "  GAGAL $1"
        echo "        harap memuat: $2"
        echo "        keluaran    : $3"
        gagal=$((gagal + 1))
    fi
}

# Jaringan tersedia? Kalau tidak, lewati seluruh berkas.
if ! "$indk" -e 'impor "pustaka/http.idk"
coba { http_ambil("https://example.com", "GET", kosong, "", 0) } kecuali GalatOS { }' \
     >/dev/null 2>&1; then
    : # -e mungkin tidak ada; dicoba lewat berkas di bawah
fi

cat > "$tmp/sambung.idk" <<'EOF'
impor "pustaka/http.idk"
coba {
    variabel r = http_ambil("https://example.com", "GET", kosong, "", 0)
    cetak("KODE", r.status)
} kecuali GalatOS sebagai e {
    cetak("GALAT", e.pesan)
}
EOF
keluaran="$("$indk" "$tmp/sambung.idk" 2>&1)"
if printf '%s' "$keluaran" | grep -q "tidak ditemukan\|tidak dapat tersambung"; then
    echo "https_klien: DILEWATI (jaringan tidak tersedia)"
    exit 0
fi

echo "Menguji klien HTTPS terhadap host sungguhan"

periksa "example.com membalas 200" "KODE 200" "$keluaran"

# Diagnostik CA: hanya tercetak bila asersi di atas gagal. Kegagalan macOS
# menuduh "penerbit tidak tepercaya", yang MENUNJUK SERVER padahal sebabnya
# bisa ada di sisi kita (bundel CA tak terbaca). Tanpa data ini setiap siklus
# CI hanya menghasilkan hipotesis baru.
if ! printf '%s' "$keluaran" | grep -q "KODE 200"; then
    echo "      --- diagnostik bundel CA ---"
    for j in /etc/ssl/cert.pem /etc/ssl/certs/ca-certificates.crt \
             /usr/local/etc/openssl@3/cert.pem /opt/homebrew/etc/ca-certificates/cert.pem; do
        if [ -r "$j" ]; then
            echo "      ADA    $j ($(wc -c < "$j" | tr -d ' ') bita, $(grep -c 'BEGIN CERTIFICATE' "$j") sertifikat)"
        else
            echo "      -      $j"
        fi
    done
    echo "      INDK_CA_BUNDLE=${INDK_CA_BUNDLE:-<kosong>}"
    # Apakah host lain juga ditolak, atau khusus example.com?
    cat > "$tmp/lain.idk" <<'EOF'
impor "pustaka/http.idk"
coba {
    cetak("badge", http_ambil("https://www.google.com", "GET", kosong, "", 0).status)
} kecuali GalatOS sebagai e {
    cetak("badge GALAT", e.pesan)
}
EOF
    "$indk" "$tmp/lain.idk" 2>&1 | head -2 | sed 's/^/      /'
    # Rantai yang BENAR-BENAR dikirim server, dilihat dari runner itu sendiri.
    # Membedakan "bundel kita salah" dari "runner di belakang proxy TLS".
    if command -v openssl >/dev/null 2>&1; then
        echo | openssl s_client -connect example.com:443 -servername example.com 2>/dev/null \
            | grep -E '^ *[0-9] s:|^ *i:|Verify return code' | head -6 | sed 's/^/      ssl: /'
    fi
fi

# --- tes negatif: semua ini WAJIB ditolak -----------------------------------
tolak() { # nama, host, potongan pesan
    cat > "$tmp/t.idk" <<EOF
impor "pustaka/http.idk"
coba {
    variabel r = http_ambil("https://$2/", "GET", kosong, "", 0)
    cetak("DITERIMA", r.status)
} kecuali GalatOS sebagai e {
    cetak("DITOLAK", e.pesan)
}
EOF
    keluaran="$("$indk" "$tmp/t.idk" 2>&1)"

    # Yang WAJIB benar: permintaannya DITOLAK. Inilah pemeriksaan yang berubah
    # merah begitu verifikasi dilemahkan -- dengan VERIFY_NONE host-host ini
    # menjawab 200.
    periksa "$1" "DITOLAK" "$keluaran"

    # Sebab spesifiknya diperiksa terpisah, dan galat transport TIDAK dihitung
    # gagal: badssl.com membatasi laju dan kadang memutus koneksi sebelum
    # sertifikat sempat dinilai, sehingga pesannya jadi "EOF". Itu bukan cacat
    # kode kita, dan CI yang merah karenanya hanya melatih orang mengabaikan CI.
    case "$keluaran" in
        *EOF*|*"batas waktu"*|*reset*|*"tidak dapat tersambung"*)
            echo "  lewat $1: sebab spesifik (koneksi diputus sebelum sertifikat dinilai)"
            ;;
        *)
            periksa "$1 (sebab spesifik)" "$3" "$keluaran"
            ;;
    esac
}

tolak "sertifikat kedaluwarsa ditolak"   "expired.badssl.com"        "kedaluwarsa"
tolak "hostname salah ditolak"           "wrong.host.badssl.com"     "tidak diterbitkan untuk"
tolak "sertifikat self-signed ditolak"   "self-signed.badssl.com"    "tidak tepercaya"
tolak "akar tak dikenal ditolak"         "untrusted-root.badssl.com" "tidak tepercaya"

# --- penurunan https -> http ditolak ----------------------------------------
# Tidak butuh jaringan: cukup pengalihan yang dibuat sendiri. Di sini yang
# diuji adalah keputusan di http_ambil(), bukan perilaku server mana pun.
cat > "$tmp/turun.idk" <<'EOF'
impor "pustaka/http.idk"
coba {
    # Lokasi absolut http:// dari halaman https:// harus ditolak sebelum
    # permintaan kedua dikirim.
    cetak(_http_gabung_url("https://a.id/x", "http://a.id/x"))
} kecuali GalatJaringan sebagai e {
    cetak("GALAT", e.pesan)
}
EOF
periksa "gabung_url mempertahankan skema" "http://a.id/x" "$("$indk" "$tmp/turun.idk" 2>&1)"

# --- INDK_CA_BUNDLE yang salah harus gagal, bukan dilewati ------------------
cat > "$tmp/ca.idk" <<'EOF'
impor "pustaka/http.idk"
coba {
    variabel r = http_ambil("https://example.com", "GET", kosong, "", 0)
    cetak("DITERIMA", r.status)
} kecuali GalatOS sebagai e {
    cetak("DITOLAK", e.pesan)
}
EOF
keluaran="$(INDK_CA_BUNDLE=/tidak/ada/bundel.pem "$indk" "$tmp/ca.idk" 2>&1)"
periksa "INDK_CA_BUNDLE salah = gagal, bukan verifikasi dilewati" \
        "tidak bisa dibaca" "$keluaran"

echo "================================================="
echo "https_klien: $lulus lulus, $gagal gagal"
[ "$gagal" -eq 0 ]
