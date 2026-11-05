#!/usr/bin/env bash
# Tabel hash memakai `hash & (capacity - 1)` sebagai ganti `hash % capacity`.
#
# Yang diuji di sini KEBENARAN, bukan kecepatan: mask hanya setara modulo bila
# kapasitas pangkat dua, dan kalau asumsi itu dilanggar programnya TIDAK crash
# -- indeksnya cuma salah dan entri menghilang diam-diam. Jadi tesnya menekan
# jalur yang paling mungkin membongkar indeks salah: banyak resize, tombstone
# dari penghapusan, dan pencarian kunci yang sudah dihapus lalu dimasukkan lagi.
cd "$(dirname "$0")/.." || exit 1
BIN=./indk
[ -f "$BIN" ] || BIN=./indk.exe

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
lulus=0; gagal=0

# Windows: indk.exe menulis CRLF. Tanpa membuang \r, penggabungan baris
# menghasilkan "800\r,0\r," dan setiap perbandingan gagal walau nilainya benar.
bersihkan() { tr -d '\r'; }

periksa() {
    if [ "$2" = "$3" ]; then
        lulus=$((lulus+1))
    else
        gagal=$((gagal+1)); echo "  x $1 (didapat '$2', diharapkan '$3')"
    fi
}

# 1. Kamus tumbuh melewati banyak resize (8 -> 16 -> ... -> 2048), lalu setiap
#    kunci dibaca kembali. Satu indeks salah = satu kunci hilang.
cat > "$TMP/tumbuh.idk" <<'EOF'
variabel d = {}
variabel i = 0
selagi i < 1000 {
    d["kunci" + ke_teks(i)] = i * 2
    i = i + 1
}
variabel jumlah = 0
variabel hilang = 0
i = 0
selagi i < 1000 {
    variabel k = "kunci" + ke_teks(i)
    jika k dalam d {
        jumlah = jumlah + d[k]
    } selain {
        hilang = hilang + 1
    }
    i = i + 1
}
cetak(panjang(d))
cetak(jumlah)
cetak(hilang)
EOF
hasil=$("$BIN" "$TMP/tumbuh.idk" 2>&1 | bersihkan | tr '\n' ',')
periksa "1000 kunci bertahan melewati resize" "$hasil" "1000,999000,0,"

# 2. Tombstone: hapus separuh kunci, lalu pastikan yang tersisa masih ketemu
#    DAN yang terhapus benar-benar hilang. Probing yang salah setelah
#    penghapusan akan berhenti di tombstone dan melaporkan kunci sah tak ada.
cat > "$TMP/hapus.idk" <<'EOF'
variabel d = {}
variabel i = 0
selagi i < 500 {
    d["k" + ke_teks(i)] = i
    i = i + 1
}
i = 0
selagi i < 500 {
    jika i % 2 == 0 {
        d.hapus_kunci("k" + ke_teks(i))
    }
    i = i + 1
}
variabel ada = 0
variabel tak_ada = 0
i = 0
selagi i < 500 {
    jika ("k" + ke_teks(i)) dalam d {
        ada = ada + 1
    } selain {
        tak_ada = tak_ada + 1
    }
    i = i + 1
}
cetak(ada)
cetak(tak_ada)
cetak(panjang(d))
EOF
hasil=$("$BIN" "$TMP/hapus.idk" 2>&1 | bersihkan | tr '\n' ',')
periksa "tombstone tidak menyembunyikan kunci sah" "$hasil" "250,250,250,"

# 3. Hapus lalu masukkan lagi kunci yang sama: slot tombstone harus dipakai
#    ulang, bukan menghasilkan entri ganda atau kunci yang tak terbaca.
cat > "$TMP/ulang.idk" <<'EOF'
variabel d = {}
variabel i = 0
selagi i < 200 {
    d["x" + ke_teks(i)] = i
    i = i + 1
}
i = 0
selagi i < 200 {
    d.hapus_kunci("x" + ke_teks(i))
    i = i + 1
}
cetak(panjang(d))
i = 0
selagi i < 200 {
    d["x" + ke_teks(i)] = i + 1000
    i = i + 1
}
cetak(panjang(d))
cetak(d["x0"])
cetak(d["x199"])
EOF
hasil=$("$BIN" "$TMP/ulang.idk" 2>&1 | bersihkan | tr '\n' ',')
periksa "hapus lalu isi ulang memakai slot kembali" "$hasil" "0,200,1000,1199,"

# 4. Himpunan lewat jalur yang sama (ValueTable), termasuk nilai non-teks.
cat > "$TMP/himpunan.idk" <<'EOF'
variabel h = himpunan()
variabel i = 0
selagi i < 600 {
    h.tambah(i)
    h.tambah("s" + ke_teks(i))
    i = i + 1
}
cetak(panjang(h))
cetak(300 dalam h)
cetak("s300" dalam h)
cetak(9999 dalam h)
EOF
hasil=$("$BIN" "$TMP/himpunan.idk" 2>&1 | bersihkan | tr '\n' ',')
periksa "himpunan campur angka dan teks" "$hasil" "1200,benar,benar,salah,"

# 5. Tabrakan hash disengaja: kunci dengan nilai berbeda yang sengaja dibuat
#    banyak agar probing linear berjalan panjang. Mask yang salah paling
#    terlihat di sini, karena probing melewati batas larik.
cat > "$TMP/tabrakan.idk" <<'EOF'
variabel d = {}
variabel i = 0
# kunci sangat mirip -> hash berdekatan -> rantai probing panjang
selagi i < 800 {
    d[ke_teks(i) + "aaaaaaaaaa"] = i
    i = i + 1
}
variabel keliru = 0
i = 0
selagi i < 800 {
    jika d[ke_teks(i) + "aaaaaaaaaa"] != i {
        keliru = keliru + 1
    }
    i = i + 1
}
cetak(panjang(d))
cetak(keliru)
EOF
hasil=$("$BIN" "$TMP/tabrakan.idk" 2>&1 | bersihkan | tr '\n' ',')
periksa "800 kunci mirip, probing panjang" "$hasil" "800,0,"

echo "TABEL-MASK: $lulus lulus, $gagal gagal"
[ "$gagal" -eq 0 ] || exit 1
