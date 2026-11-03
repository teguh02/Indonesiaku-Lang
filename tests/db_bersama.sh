#!/usr/bin/env bash
# API bersama pustaka/db.idk diuji terhadap KETIGA backend.
#
# Golden examples/db_bersama.idk hanya menjalankan SQLite, jadi ia tidak bisa
# membuktikan portabilitas -- justru itu yang dijanjikan WDB-04. Di sini
# skrip .idk yang SAMA dijalankan tiga kali, hanya konfigurasi sambung() yang
# berbeda, terhadap MariaDB dan PostgreSQL sungguhan dalam container.
#
# Melewati diri sendiri bila Docker tidak ada: CI merah karena lingkungan tak
# lengkap hanya melatih orang mengabaikan CI.
set -u

INDK="${INDK:-./indk}"
PORT_PG=55433
PORT_MY=33072
WADAH_PG=idkdb_pg_uji
WADAH_MY=idkdb_my_uji
SANDI=rahasia_db
TMP="$(mktemp -d)"
GAGAL=0

bersihkan() {
    docker rm -f "$WADAH_PG" "$WADAH_MY" >/dev/null 2>&1 || true
    rm -rf "$TMP"
}
trap bersihkan EXIT

ok() {
    if [ "$1" = "0" ]; then
        echo "  OK    $2"
    else
        echo "  GAGAL $2"
        GAGAL=1
    fi
}

if ! command -v docker >/dev/null 2>&1 || ! docker info >/dev/null 2>&1; then
    echo "  LEWAT uji db bersama: docker tidak tersedia"
    exit 0
fi

# --- skrip portabel: satu isi, tiga backend --------------------------------
# Kecuali baris sambung(), tidak ada satu pun pemanggilan khusus backend.
# PLACEHOLDER diganti per backend karena postgres memakai $1 sementara
# sqlite/mysql memakai ? -- perbedaan yang memang TIDAK disembunyikan db.idk.
cat > "$TMP/portabel.idk" <<'SKRIP'
impor db
impor json

variabel konf = json.json_urai(ambil_lingkungan("DB_KONF"))
# Dua placeholder siap pakai: postgres bernomor ($1,$2), sqlite/mysql tidak (?,?).
variabel P1 = ambil_lingkungan("DB_P1")
variabel P2 = ambil_lingkungan("DB_P2")

variabel k = db.sambung(konf)
cetak("jenis:", k.jenis())

k.eksekusi("DROP TABLE IF EXISTS uji_portabel")
k.eksekusi("CREATE TABLE uji_portabel (id INTEGER PRIMARY KEY, nama TEXT)")

cetak("terpengaruh:", k.eksekusi("INSERT INTO uji_portabel (id, nama) VALUES (" + P1 + ", " + P2 + ")", [1, "Budi"]))
k.eksekusi("INSERT INTO uji_portabel (id, nama) VALUES (" + P1 + ", " + P2 + ")", [2, "Ani"])

variabel semua = k.ambil_semua("SELECT nama FROM uji_portabel ORDER BY id")
cetak("jumlah:", panjang(semua))
cetak("pertama:", semua[0]["nama"])

variabel satu = k.ambil_satu("SELECT nama FROM uji_portabel WHERE id = " + P1, [2])
cetak("satu:", satu["nama"])

cetak("kosong:", k.ambil_satu("SELECT nama FROM uji_portabel WHERE id = " + P1, [99]))

cetak("tanpa parameter:", panjang(k.ambil_semua("SELECT nama FROM uji_portabel")))

# transaksi: batal harus benar-benar membatalkan
k.mulai()
k.eksekusi("INSERT INTO uji_portabel (id, nama) VALUES (" + P1 + ", " + P2 + ")", [3, "Hilang"])
k.batal()
cetak("setelah batal:", panjang(k.ambil_semua("SELECT nama FROM uji_portabel")))

k.mulai()
k.eksekusi("INSERT INTO uji_portabel (id, nama) VALUES (" + P1 + ", " + P2 + ")", [4, "Tetap"])
k.komit()
cetak("setelah komit:", panjang(k.ambil_semua("SELECT nama FROM uji_portabel")))

# parameter dikirim sebagai parameter, bukan dirangkai: ini harus jadi DATA
k.eksekusi("INSERT INTO uji_portabel (id, nama) VALUES (" + P1 + ", " + P2 + ")",
           [5, "'; DROP TABLE uji_portabel; --"])
cetak("injeksi jadi data:", panjang(k.ambil_semua("SELECT nama FROM uji_portabel")))

k.eksekusi("DROP TABLE uji_portabel")
k.tutup()
k.tutup()
cetak("tutup dua kali: aman")
SKRIP

# Keluaran yang diharapkan identik untuk ketiga backend, kecuali baris jenis.
harapan() {
    cat <<HARAP
jenis: $1
terpengaruh: 1
jumlah: 2
pertama: Budi
satu: Ani
kosong: kosong
tanpa parameter: 2
setelah batal: 2
setelah komit: 3
injeksi jadi data: 4
tutup dua kali: aman
HARAP
}

jalankan_backend() {
    local nama="$1" konf="$2" p1="$3" p2="$4"
    local keluar="$TMP/$nama.out" galat="$TMP/$nama.err"

    if DB_KONF="$konf" DB_P1="$p1" DB_P2="$p2" "$INDK" "$TMP/portabel.idk" >"$keluar" 2>"$galat"; then
        if [ -s "$galat" ]; then
            ok 1 "$nama: stderr harus kosong"
            sed 's/^/        /' "$galat"
            return
        fi
        if diff -q <(harapan "$nama") "$keluar" >/dev/null 2>&1; then
            ok 0 "$nama: keluaran identik dengan backend lain"
        else
            ok 1 "$nama: keluaran berbeda"
            diff <(harapan "$nama") "$keluar" | sed 's/^/        /' | head -20
        fi
    else
        ok 1 "$nama: skrip portabel gagal jalan"
        sed 's/^/        /' "$galat" | head -10
    fi
}

echo "API bersama db.idk pada tiga backend:"

# --- sqlite ----------------------------------------------------------------
jalankan_backend "sqlite" "{\"jenis\":\"sqlite\",\"berkas\":\"$TMP/uji.db\"}" "?" "?"

# --- postgres --------------------------------------------------------------
docker rm -f "$WADAH_PG" >/dev/null 2>&1 || true
if docker run -d --rm --name "$WADAH_PG" \
        -e POSTGRES_PASSWORD="$SANDI" -e POSTGRES_DB=ujian \
        -p "127.0.0.1:$PORT_PG:5432" postgres:16-alpine >/dev/null 2>&1; then

    KONF_PG="{\"jenis\":\"postgres\",\"host\":\"127.0.0.1\",\"port\":$PORT_PG,\"pengguna\":\"postgres\",\"sandi\":\"$SANDI\",\"basisdata\":\"ujian\"}"

    # pg_isready berjalan di dalam container dan sudah bilang siap ketika
    # postgres masih restart setelah initdb; koneksi dari host masih diputus.
    # Jadi tunggu query SUNGGUHAN dari host lewat port yang dipetakan.
    SIAP=0
    for _ in $(seq 1 90); do
        if DB_KONF="$KONF_PG" "$INDK" /dev/stdin >/dev/null 2>&1 <<'PROBE'
impor db
impor json
variabel k = db.sambung(json.json_urai(ambil_lingkungan("DB_KONF")))
k.ambil_satu("SELECT 1 AS x")
k.tutup()
PROBE
        then SIAP=1; break; fi
        sleep 1
    done
    if [ "$SIAP" -eq 1 ]; then
        jalankan_backend "postgres" "$KONF_PG" "\$1" "\$2"
    else
        ok 1 "postgres: tidak bisa dihubungi dari host dalam 90 detik"
    fi
else
    echo "  LEWAT postgres: image tidak bisa dijalankan"
fi
docker rm -f "$WADAH_PG" >/dev/null 2>&1 || true

# --- mysql/mariadb ---------------------------------------------------------
# MariaDB, bukan mysql:8: image mysql:8 menolak jalan di CPU tanpa x86-64-v2
# dan bawaannya memakai caching_sha2_password yang tidak didukung driver ini.
docker rm -f "$WADAH_MY" >/dev/null 2>&1 || true
if docker run -d --rm --name "$WADAH_MY" \
        -e MARIADB_ROOT_PASSWORD="$SANDI" -e MARIADB_DATABASE=ujian \
        -p "127.0.0.1:$PORT_MY:3306" mariadb:11 >/dev/null 2>&1; then

    KONF_MY="{\"jenis\":\"mysql\",\"host\":\"127.0.0.1\",\"port\":$PORT_MY,\"pengguna\":\"root\",\"sandi\":\"$SANDI\",\"basisdata\":\"ujian\"}"

    SIAP=0
    for _ in $(seq 1 90); do
        if DB_KONF="$KONF_MY" "$INDK" /dev/stdin >/dev/null 2>&1 <<'PROBE'
impor db
impor json
variabel k = db.sambung(json.json_urai(ambil_lingkungan("DB_KONF")))
k.ambil_satu("SELECT 1 AS x")
k.tutup()
PROBE
        then SIAP=1; break; fi
        sleep 1
    done
    if [ "$SIAP" -eq 1 ]; then
        jalankan_backend "mysql" "$KONF_MY" "?" "?"
    else
        ok 1 "mysql: tidak bisa dihubungi dari host dalam 90 detik"
    fi
else
    echo "  LEWAT mysql: image tidak bisa dijalankan"
fi
docker rm -f "$WADAH_MY" >/dev/null 2>&1 || true

# --- jenis tidak dikenal ditolak -------------------------------------------
if "$INDK" /dev/stdin >"$TMP/tolak.out" 2>&1 <<'TOLAK'
impor db
coba {
    db.sambung({"jenis": "oracle"})
    cetak("SALAH: diterima")
} kecuali GalatOS sebagai g {
    cetak("ditolak")
}
TOLAK
then
    if grep -q "^ditolak$" "$TMP/tolak.out"; then
        ok 0 "jenis tidak dikenal ditolak sebagai GalatOS"
    else
        ok 1 "jenis tidak dikenal TIDAK ditolak"
        sed 's/^/        /' "$TMP/tolak.out" | head -5
    fi
else
    ok 1 "uji jenis tidak dikenal gagal jalan"
fi

# --- konfigurasi tidak lengkap ditolak -------------------------------------
if "$INDK" /dev/stdin >"$TMP/kurang.out" 2>&1 <<'KURANG'
impor db
coba {
    db.sambung({"jenis": "sqlite"})
    cetak("SALAH: diterima")
} kecuali GalatOS sebagai g {
    cetak("ditolak")
}
coba {
    db.sambung({"berkas": "x.db"})
    cetak("SALAH: tanpa jenis diterima")
} kecuali GalatOS sebagai g {
    cetak("ditolak tanpa jenis")
}
KURANG
then
    if grep -q "^ditolak$" "$TMP/kurang.out" && grep -q "^ditolak tanpa jenis$" "$TMP/kurang.out"; then
        ok 0 "konfigurasi tidak lengkap ditolak"
    else
        ok 1 "konfigurasi tidak lengkap TIDAK ditolak"
        sed 's/^/        /' "$TMP/kurang.out" | head -5
    fi
else
    ok 1 "uji konfigurasi tidak lengkap gagal jalan"
fi

if [ "$GAGAL" -eq 0 ]; then
    echo "LULUS"
else
    echo "GAGAL"
fi
exit "$GAGAL"
