#!/usr/bin/env bash
# Driver MySQL/MariaDB: diuji terhadap MariaDB SUNGGUHAN dalam container.
#
# MySQL 8 TIDAK diuji di sini dan itu disengaja: image mysql:8 menolak jalan
# di CPU tanpa x86-64-v2 ("Fatal glibc error: CPU does not support
# x86-64-v2"), dan bawaannya memakai caching_sha2_password yang memang tidak
# didukung driver ini. Menandai MySQL 8 "didukung" tanpa pernah menjalankannya
# akan jadi klaim kosong.
#
# Melewati diri sendiri bila Docker tidak ada.
set -u

INDK="${INDK:-./indk}"
PORT=33071
WADAH=idkmaria_uji
SANDI=rahasia_my
TMP="$(mktemp -d)"
GAGAL=0

bersihkan() {
    docker rm -f "$WADAH" >/dev/null 2>&1 || true
    rm -rf "$TMP"
}
trap bersihkan EXIT

if ! command -v docker >/dev/null 2>&1 || ! docker info >/dev/null 2>&1; then
    echo "  LEWAT uji mysql: docker tidak tersedia"
    exit 0
fi

docker rm -f "$WADAH" >/dev/null 2>&1 || true
if ! docker run -d --rm --name "$WADAH" \
        -e MARIADB_ROOT_PASSWORD="$SANDI" -e MARIADB_DATABASE=ujian \
        -p "127.0.0.1:$PORT:3306" mariadb:11 >/dev/null 2>&1; then
    echo "  LEWAT uji mysql: image mariadb tidak bisa dijalankan"
    exit 0
fi

# Tunggu koneksi SUNGGUHAN dari host, bukan healthcheck di dalam container:
# server sempat siap di unix socket sementara port host masih menolak.
cat > "$TMP/probe.idk" <<'PROBE'
impor "pustaka/my.idk"
variabel k = sambung({"host":"127.0.0.1",
                      "port": ke_bulat(ambil_lingkungan("MYPORT")),
                      "pengguna":"root",
                      "sandi": ambil_lingkungan("MYSANDI"),
                      "basisdata":"ujian"})
k.ambil_satu("SELECT 1 AS x", kosong)
k.tutup()
PROBE
SIAP=0
for _ in $(seq 1 90); do
    if MYPORT="$PORT" MYSANDI="$SANDI" "$INDK" "$TMP/probe.idk" >/dev/null 2>&1; then
        SIAP=1
        break
    fi
    sleep 1
done
if [ "$SIAP" -eq 0 ]; then
    echo "  GAGAL uji mysql: MariaDB tidak bisa dihubungi dari host"
    exit 1
fi

periksa() {
    local nama="$1" harap="$2" dapat="$3"
    if [ "$dapat" = "$harap" ]; then
        echo "  OK    $nama"
    else
        echo "  GAGAL $nama: harap '$harap', dapat '$dapat'"
        GAGAL=$((GAGAL+1))
    fi
}

echo "Driver MySQL/MariaDB (server sungguhan):"

cat > "$TMP/crud.idk" <<'EOF'
impor "pustaka/my.idk"

variabel k = sambung({"host":"127.0.0.1",
                      "port": ke_bulat(ambil_lingkungan("MYPORT")),
                      "pengguna":"root",
                      "sandi": ambil_lingkungan("MYSANDI"),
                      "basisdata":"ujian"})

k.eksekusi("DROP TABLE IF EXISTS siswa", kosong)
k.eksekusi("CREATE TABLE siswa (id int auto_increment primary key, nama varchar(190), nilai int)", kosong)

cetak("insert=" + ke_teks(k.eksekusi("INSERT INTO siswa (nama, nilai) VALUES (?, ?)", ["Budi", 80])))
k.eksekusi("INSERT INTO siswa (nama, nilai) VALUES (?, ?)", ["Siti", 90])
cetak("jumlah=" + ke_teks(panjang(k.ambil_semua("SELECT * FROM siswa", kosong))))
cetak("update=" + ke_teks(k.eksekusi("UPDATE siswa SET nilai = ? WHERE nama = ?", [85, "Budi"])))

# Kolom INT harus kembali sebagai ANGKA, bukan teks: hasil prepared statement
# berformat biner, dan membaca 4 byte mentah sebagai string merusak nilainya.
variabel n = k.ambil_satu("SELECT nilai FROM siswa WHERE nama = ?", ["Budi"])["nilai"]
cetak("nilai=" + ke_teks(n))
cetak("angka=" + ke_teks(jenis(n) == "angka"))

cetak("delnihil=" + ke_teks(k.eksekusi("DELETE FROM siswa WHERE nama = ?", ["TidakAda"])))

k.eksekusi("INSERT INTO siswa (nama, nilai) VALUES (?, ?)", ["O'Brien 世界 😀", kosong])
variabel u = k.ambil_satu("SELECT nama, nilai FROM siswa WHERE nilai IS NULL", kosong)
cetak("unicode=" + u["nama"])
cetak("null=" + ke_teks(u["nilai"] == kosong))

k.mulai()
k.eksekusi("INSERT INTO siswa (nama, nilai) VALUES (?, ?)", ["Sementara", 1])
k.batal()
cetak("rollback=" + ke_teks(panjang(k.ambil_semua("SELECT * FROM siswa WHERE nama = ?", ["Sementara"]))))
k.mulai()
k.eksekusi("INSERT INTO siswa (nama, nilai) VALUES (?, ?)", ["Tetap", 2])
k.komit()
cetak("komit=" + ke_teks(panjang(k.ambil_semua("SELECT * FROM siswa WHERE nama = ?", ["Tetap"]))))

coba {
    k.ambil_semua("SELECT * FROM tabel_hantu", kosong)
    cetak("galat=tidak_dinaikkan")
} kecuali e { cetak("galat=tertangkap") }
cetak("hidup=" + ke_teks(k.ambil_satu("SELECT 42 AS x", kosong)["x"]))

# Jumlah parameter dijaga di kedua arah.
coba { k.eksekusi("SELECT ?", kosong); cetak("kurang=lolos") } kecuali e { cetak("kurang=ok") }
coba { k.eksekusi("SELECT 1", [5]); cetak("lebih=lolos") } kecuali e { cetak("lebih=ok") }

# Hasil besar memaksa satu paket terpecah di beberapa pembacaan soket.
k.eksekusi("DROP TABLE IF EXISTS besar", kosong)
k.eksekusi("CREATE TABLE besar (id int, isi text)", kosong)
variabel i = 0
selagi i < 300 {
    k.eksekusi("INSERT INTO besar VALUES (?, ?)", [i, "data panjang untuk memaksa paket terpecah " + ke_teks(i)])
    i = i + 1
}
variabel b = k.ambil_semua("SELECT * FROM besar ORDER BY id", kosong)
cetak("besar=" + ke_teks(panjang(b)))
cetak("akhir=" + ke_teks(b[299]["id"]))

k.tutup()
k.tutup()
cetak("idempoten=ok")
EOF

KELUARAN=$(MYPORT="$PORT" MYSANDI="$SANDI" "$INDK" "$TMP/crud.idk" 2>&1)
if [ $? -ne 0 ]; then
    echo "  GAGAL crud: $KELUARAN"
    GAGAL=$((GAGAL+1))
else
    ambil() { echo "$KELUARAN" | grep "^$1=" | head -1 | cut -d= -f2-; }
    periksa "insert 1 baris"       "1"  "$(ambil insert)"
    periksa "select 2 baris"       "2"  "$(ambil jumlah)"
    periksa "update 1 baris"       "1"  "$(ambil update)"
    periksa "nilai ter-update"     "85" "$(ambil nilai)"
    periksa "INT kembali angka"    "benar" "$(ambil angka)"
    periksa "delete nihil = 0"     "0"  "$(ambil delnihil)"
    periksa "unicode utuh"         "O'Brien 世界 😀" "$(ambil unicode)"
    periksa "NULL jadi kosong"     "benar" "$(ambil null)"
    periksa "rollback batal"       "0"  "$(ambil rollback)"
    periksa "komit tersimpan"      "1"  "$(ambil komit)"
    periksa "galat tertangkap"     "tertangkap" "$(ambil galat)"
    periksa "koneksi hidup"        "42" "$(ambil hidup)"
    periksa "parameter kurang"     "ok" "$(ambil kurang)"
    periksa "parameter lebih"      "ok" "$(ambil lebih)"
    periksa "300 baris utuh"       "300" "$(ambil besar)"
    periksa "baris terakhir benar" "299" "$(ambil akhir)"
    periksa "tutup idempoten"      "ok" "$(ambil idempoten)"
fi

# NO_BACKSLASH_ESCAPES: inilah yang membunuh versi COM_QUERY + escaping manual
# dari driver ini. Dengan mode itu backslash BUKAN karakter escape, sehingga
# "x\" tersimpan sebagai "x\\" -- korupsi data diam-diam, bukan galat.
# Prepared statement membuat sql_mode tidak relevan.
cat > "$TMP/nbs.idk" <<'EOF'
impor "pustaka/my.idk"
variabel k = sambung({"host":"127.0.0.1",
                      "port": ke_bulat(ambil_lingkungan("MYPORT")),
                      "pengguna":"root",
                      "sandi": ambil_lingkungan("MYSANDI"),
                      "basisdata":"ujian"})
k.eksekusi("SET SESSION sql_mode = 'NO_BACKSLASH_ESCAPES'", kosong)
k.eksekusi("DROP TABLE IF EXISTS nbs", kosong)
k.eksekusi("CREATE TABLE nbs (nama varchar(200))", kosong)
k.eksekusi("INSERT INTO nbs VALUES (?)", ["x\\"])
cetak("isi=" + k.ambil_satu("SELECT nama FROM nbs", kosong)["nama"])
k.tutup()
EOF
KELUARAN=$(MYPORT="$PORT" MYSANDI="$SANDI" "$INDK" "$TMP/nbs.idk" 2>&1)
periksa "NO_BACKSLASH_ESCAPES utuh" 'isi=x\' "$(echo "$KELUARAN" | grep '^isi=')"

# Payload injeksi harus tersimpan sebagai DATA, persis seperti dikirim.
cat > "$TMP/inj.idk" <<'EOF'
impor "pustaka/my.idk"
variabel k = sambung({"host":"127.0.0.1",
                      "port": ke_bulat(ambil_lingkungan("MYPORT")),
                      "pengguna":"root",
                      "sandi": ambil_lingkungan("MYSANDI"),
                      "basisdata":"ujian"})
k.eksekusi("DROP TABLE IF EXISTS korban", kosong)
k.eksekusi("CREATE TABLE korban (nama varchar(200))", kosong)
variabel jahat = ["' OR '1'='1", "'; DROP TABLE korban; --", "\\' OR 1=1 --", "x\\", "a' UNION SELECT user() --"]
untuk j dalam jahat { k.eksekusi("INSERT INTO korban VALUES (?)", [j]) }
variabel b = k.ambil_semua("SELECT nama FROM korban", kosong)
cetak("tersimpan=" + ke_teks(panjang(b)))
variabel cocok = benar
variabel i = 0
selagi i < panjang(jahat) {
    jika b[i]["nama"] != jahat[i] { cocok = salah }
    i = i + 1
}
cetak("utuh=" + ke_teks(cocok))
k.tutup()
EOF
KELUARAN=$(MYPORT="$PORT" MYSANDI="$SANDI" "$INDK" "$TMP/inj.idk" 2>&1)
periksa "injeksi jadi data"   "5"     "$(echo "$KELUARAN" | grep '^tersimpan=' | cut -d= -f2)"
periksa "payload utuh"        "benar" "$(echo "$KELUARAN" | grep '^utuh=' | cut -d= -f2)"

# Sandi salah ditolak, dan sandinya tidak bocor ke pesan galat.
cat > "$TMP/sandi.idk" <<'EOF'
impor "pustaka/my.idk"
coba {
    sambung({"host":"127.0.0.1", "port": ke_bulat(ambil_lingkungan("MYPORT")),
             "pengguna":"root", "sandi":"SANDI-SALAH-XYZ", "basisdata":"ujian"})
    cetak("diterima")
} kecuali e { cetak("ditolak: " + ke_teks(e)) }
EOF
KELUARAN=$(MYPORT="$PORT" "$INDK" "$TMP/sandi.idk" 2>&1)
case "$KELUARAN" in
    *ditolak*) echo "  OK    sandi salah ditolak" ;;
    *) echo "  GAGAL sandi salah diterima"; GAGAL=$((GAGAL+1)) ;;
esac
case "$KELUARAN" in
    *SANDI-SALAH-XYZ*) echo "  GAGAL sandi bocor ke pesan galat"; GAGAL=$((GAGAL+1)) ;;
    *) echo "  OK    sandi tidak bocor" ;;
esac

if [ "$GAGAL" -gt 0 ]; then
    echo "GAGAL: $GAGAL pemeriksaan"
    exit 1
fi
echo "LULUS"
