#!/usr/bin/env bash
# Driver PostgreSQL: diuji terhadap PostgreSQL SUNGGUHAN, bukan mock.
#
# Mock protokol hanya menguji pemahaman saya sendiri tentang protokol itu.
# Kalau salah paham, mock-nya ikut salah dan tesnya tetap hijau. Jadi bagian
# CRUD bicara dengan postgres:16-alpine di container.
#
# Perkecualiannya autentikasi: server sungguhan JUJUR, jadi ia tidak bisa
# membuktikan klien memeriksa nonce dan tanda tangan server -- melewatkan
# kedua cek itu tetap menghasilkan koneksi sukses. Untuk itu dipakai server
# palsu yang curang (tests/bantu/pg_server_jahat.py).
#
# Tes melewati diri sendiri bila Docker tidak ada: CI yang merah karena
# lingkungan tidak lengkap melatih orang mengabaikan CI.
set -u

INDK="${INDK:-./indk}"
PORT=55432
WADAH=idkpg_uji
SANDI=rahasia123
TMP="$(mktemp -d)"
GAGAL=0

bersihkan() {
    docker rm -f "$WADAH" >/dev/null 2>&1 || true
    rm -rf "$TMP"
}
trap bersihkan EXIT

if ! command -v docker >/dev/null 2>&1; then
    echo "  LEWAT uji pg: docker tidak tersedia"
    exit 0
fi
if ! docker info >/dev/null 2>&1; then
    echo "  LEWAT uji pg: daemon docker tidak jalan"
    exit 0
fi

docker rm -f "$WADAH" >/dev/null 2>&1 || true
if ! docker run -d --rm --name "$WADAH" \
        -e POSTGRES_PASSWORD="$SANDI" -e POSTGRES_DB=ujian \
        -p "127.0.0.1:$PORT:5432" postgres:16-alpine >/dev/null 2>&1; then
    echo "  LEWAT uji pg: image postgres tidak bisa dijalankan"
    exit 0
fi

SIAP=0
for _ in $(seq 1 60); do
    if docker exec "$WADAH" pg_isready -U postgres >/dev/null 2>&1; then
        SIAP=1
        break
    fi
    sleep 1
done
if [ "$SIAP" -eq 0 ]; then
    echo "  GAGAL uji pg: PostgreSQL tidak siap dalam 60 detik"
    exit 1
fi

# pg_isready saja TIDAK cukup: container postgres menjalankan initdb, lalu
# ME-RESTART server. Di sela itu pg_isready sudah bilang siap sementara
# koneksi dari host masih diterima-lalu-diputus. Itu yang membuat CI merah
# sementara mesin lokal hijau -- di lokal containernya sudah panas.
#
# Jadi tunggu sampai sebuah query SUNGGUHAN berhasil DARI HOST, lewat port
# yang dipetakan, bukan lewat unix socket di dalam container.
tunggu_pg_siap() {
    local port="$1" sandi="$2" i
    for i in $(seq 1 60); do
        if PGPORT_T="$port" PGSANDI_T="$sandi" "$INDK" /dev/stdin <<'PROBE' >/dev/null 2>&1
impor "pustaka/pg.idk"
variabel k = sambung({"host":"127.0.0.1",
                      "port": ke_bulat(ambil_lingkungan("PGPORT_T")),
                      "pengguna":"postgres",
                      "sandi": ambil_lingkungan("PGSANDI_T"),
                      "basisdata":"ujian"})
k.ambil_satu("SELECT 1 AS x")
k.tutup()
PROBE
        then
            return 0
        fi
        sleep 1
    done
    return 1
}

periksa() {
    local nama="$1" harap="$2" dapat="$3"
    if [ "$dapat" = "$harap" ]; then
        echo "  OK    $nama"
    else
        echo "  GAGAL $nama: harap '$harap', dapat '$dapat'"
        GAGAL=$((GAGAL+1))
    fi
}

if ! tunggu_pg_siap "$PORT" "$SANDI"; then
    echo "  GAGAL uji pg: koneksi dari host tidak pernah berhasil"
    exit 1
fi

echo "Driver PostgreSQL (server sungguhan):"

cat > "$TMP/crud.idk" <<'EOF'
impor "pustaka/pg.idk"

variabel cfg = {"host":"127.0.0.1", "port": ke_bulat(ambil_lingkungan("PGPORT")),
                "pengguna":"postgres", "sandi": ambil_lingkungan("PGSANDI"),
                "basisdata":"ujian"}
variabel k = sambung(cfg)

k.eksekusi("DROP TABLE IF EXISTS siswa")
k.eksekusi("CREATE TABLE siswa (id serial primary key, nama text, nilai int)")

cetak("insert=" + ke_teks(k.eksekusi("INSERT INTO siswa (nama, nilai) VALUES ($1, $2)", ["Budi", 80])))
k.eksekusi("INSERT INTO siswa (nama, nilai) VALUES ($1, $2)", ["Siti", 90])
cetak("jumlah=" + ke_teks(panjang(k.ambil_semua("SELECT * FROM siswa"))))
cetak("update=" + ke_teks(k.eksekusi("UPDATE siswa SET nilai = $1 WHERE nama = $2", [85, "Budi"])))
cetak("nilai=" + k.ambil_satu("SELECT nilai FROM siswa WHERE nama = $1", ["Budi"])["nilai"])

# Baris yang tidak ada HARUS 0, bukan "berhasil". Tanpa pemeriksaan ini,
# DELETE ke id yang salah diam-diam terlihat sukses.
cetak("delete_nihil=" + ke_teks(k.eksekusi("DELETE FROM siswa WHERE nama = $1", ["TidakAda"])))

# Injeksi SQL: harus tersimpan sebagai DATA, tabel tetap ada.
k.eksekusi("INSERT INTO siswa (nama, nilai) VALUES ($1, $2)", ["'; DROP TABLE siswa; --", 1])
cetak("selamat=" + ke_teks(panjang(k.ambil_semua("SELECT * FROM siswa"))))

# NULL, kutip tunggal, dan UTF-8 multi-byte
k.eksekusi("INSERT INTO siswa (nama, nilai) VALUES ($1, $2)", ["O'Brien 世界 😀", kosong])
variabel u = k.ambil_satu("SELECT nama, nilai FROM siswa WHERE nilai IS NULL")
cetak("unicode=" + u["nama"])
cetak("null=" + ke_teks(u["nilai"] == kosong))

# Transaksi benar-benar berpengaruh
k.mulai()
k.eksekusi("INSERT INTO siswa (nama, nilai) VALUES ($1, $2)", ["Sementara", 1])
k.batal()
cetak("rollback=" + ke_teks(panjang(k.ambil_semua("SELECT * FROM siswa WHERE nama = $1", ["Sementara"]))))
k.mulai()
k.eksekusi("INSERT INTO siswa (nama, nilai) VALUES ($1, $2)", ["Tetap", 2])
k.komit()
cetak("komit=" + ke_teks(panjang(k.ambil_semua("SELECT * FROM siswa WHERE nama = $1", ["Tetap"]))))

# Galat server bisa ditangkap DAN koneksi tetap bisa dipakai sesudahnya.
coba {
    k.ambil_semua("SELECT * FROM tabel_tidak_ada")
    cetak("galat=tidak_dinaikkan")
} kecuali e {
    cetak("galat=tertangkap")
}
cetak("hidup=" + k.ambil_satu("SELECT 42 AS x")["x"])

k.tutup()
k.tutup()
cetak("idempoten=ok")
EOF

KELUARAN=$(PGPORT="$PORT" PGSANDI="$SANDI" "$INDK" "$TMP/crud.idk" 2>&1)
if [ $? -ne 0 ]; then
    echo "  GAGAL crud: $KELUARAN"
    GAGAL=$((GAGAL+1))
else
    ambil() { echo "$KELUARAN" | grep "^$1=" | head -1 | cut -d= -f2-; }
    periksa "insert 1 baris"        "1"  "$(ambil insert)"
    periksa "select 2 baris"        "2"  "$(ambil jumlah)"
    periksa "update 1 baris"        "1"  "$(ambil update)"
    periksa "nilai ter-update"      "85" "$(ambil nilai)"
    periksa "delete nihil = 0"      "0"  "$(ambil delete_nihil)"
    periksa "injeksi jadi data"     "3"  "$(ambil selamat)"
    periksa "unicode utuh"          "O'Brien 世界 😀" "$(ambil unicode)"
    periksa "NULL jadi kosong"      "benar" "$(ambil null)"
    periksa "rollback batal"        "0"  "$(ambil rollback)"
    periksa "komit tersimpan"       "1"  "$(ambil komit)"
    periksa "galat tertangkap"      "tertangkap" "$(ambil galat)"
    periksa "koneksi hidup setelah galat" "42" "$(ambil hidup)"
    periksa "tutup idempoten"       "ok" "$(ambil idempoten)"
fi

# Sandi salah ditolak, dan sandi TIDAK muncul di pesan galat.
cat > "$TMP/sandi.idk" <<'EOF'
impor "pustaka/pg.idk"
coba {
    sambung({"host":"127.0.0.1", "port": ke_bulat(ambil_lingkungan("PGPORT")),
             "pengguna":"postgres", "sandi":"SANDI-SALAH-XYZ", "basisdata":"ujian"})
    cetak("diterima")
} kecuali e {
    cetak("ditolak: " + ke_teks(e))
}
EOF
KELUARAN=$(PGPORT="$PORT" "$INDK" "$TMP/sandi.idk" 2>&1)
case "$KELUARAN" in
    *ditolak*) echo "  OK    sandi salah ditolak" ;;
    *) echo "  GAGAL sandi salah diterima: $KELUARAN"; GAGAL=$((GAGAL+1)) ;;
esac
case "$KELUARAN" in
    *SANDI-SALAH-XYZ*) echo "  GAGAL sandi bocor ke pesan galat"; GAGAL=$((GAGAL+1)) ;;
    *) echo "  OK    sandi tidak bocor ke pesan galat" ;;
esac

# --- server palsu: membuktikan klien benar-benar memeriksa -----------------
if command -v python3 >/dev/null 2>&1; then
    jahat() {
        local mode="$1" port="$2" nama="$3"
        python3 tests/bantu/pg_server_jahat.py "$port" "$mode" > "$TMP/jahat.log" 2>&1 &
        local pid=$!
        for _ in $(seq 1 30); do
            grep -q siap "$TMP/jahat.log" 2>/dev/null && break
            sleep 0.2
        done
        cat > "$TMP/jahat.idk" <<EOF
impor "pustaka/pg.idk"
coba {
    sambung({"host":"127.0.0.1","port":$port,"pengguna":"p","sandi":"x","basisdata":"u"})
    cetak("DITERIMA")
} kecuali e {
    cetak("DITOLAK")
}
EOF
        local hasil
        hasil=$("$INDK" "$TMP/jahat.idk" 2>&1)
        wait "$pid" 2>/dev/null || true
        case "$hasil" in
            *DITOLAK*) echo "  OK    $nama" ;;
            *) echo "  GAGAL $nama: server curang DITERIMA"; GAGAL=$((GAGAL+1)) ;;
        esac
    }
    jahat nonce 8921 "nonce server palsu ditolak"
    jahat tanda 8922 "tanda tangan server palsu ditolak"
else
    echo "  LEWAT server palsu: python3 tidak ada"
fi

if [ "$GAGAL" -gt 0 ]; then
    echo "GAGAL: $GAGAL pemeriksaan"
    exit 1
fi
echo "LULUS"
