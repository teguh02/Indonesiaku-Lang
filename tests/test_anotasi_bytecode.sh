#!/usr/bin/env bash
# Anotasi tipe harus TIDAK mengubah bytecode sedikit pun.
#
# Ini bukan tes kosmetik. Saat fitur ini dibuat, 'variabel a: teks = "x"' di
# DALAM fungsi menghasilkan satu slot lokal ekstra + OP_KOSONG, karena
# prescanFunctionBody() melihat pasangan 'teks' lalu '=' dan menyimpulkan
# nama tipe itu sebuah variabel yang di-assign. Program tetap mencetak hasil
# yang benar, jadi tes keluaran biasa TIDAK menangkapnya — hanya perbandingan
# bytecode yang menangkapnya.
set -u

INDK="${INDK:-./indk}"
[ -x "$INDK" ] || INDK=./indk.exe   # Windows (Git Bash): make menghasilkan indk.exe
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
gagal=0

bandingkan() {
    local nama="$1" dengan="$2" tanpa="$3"
    printf '%s' "$dengan" > "$TMP/dengan.idk"
    printf '%s' "$tanpa"  > "$TMP/tanpa.idk"

    if ! "$INDK" --dis "$TMP/dengan.idk" > "$TMP/d.txt" 2>&1; then
        echo "GAGAL [$nama]: versi beranotasi tidak terkompilasi"
        cat "$TMP/d.txt"; gagal=1; return
    fi
    "$INDK" --dis "$TMP/tanpa.idk" > "$TMP/t.txt" 2>&1

    # Nomor baris dinormalisasi: anotasi boleh menggeser baris, bukan opcode.
    sed -E 's/^[[:space:]]*[0-9]+/N/' "$TMP/d.txt" > "$TMP/d.n"
    sed -E 's/^[[:space:]]*[0-9]+/N/' "$TMP/t.txt" > "$TMP/t.n"

    if diff -q "$TMP/d.n" "$TMP/t.n" >/dev/null; then
        echo "ok   [$nama]"
    else
        echo "GAGAL [$nama]: bytecode berbeda"
        diff "$TMP/d.n" "$TMP/t.n" | head -12
        gagal=1
    fi
}

# Kasus yang dulu rusak: anotasi pada 'variabel' di dalam fungsi.
bandingkan "variabel lokal beranotasi" \
'fungsi f() {
    variabel a: teks = "x"
    kembalikan a
}
cetak(f())
' \
'fungsi f() {
    variabel a = "x"
    kembalikan a
}
cetak(f())
'

bandingkan "parameter + tipe kembali + bawaan" \
'fungsi sapa(nama: teks, ulang: bilangan = 2) -> teks {
    variabel hasil: teks = nama * ulang
    kembalikan hasil
}
variabel umur: bilangan = 30
cetak(sapa("hai"), umur)
' \
'fungsi sapa(nama, ulang = 2) {
    variabel hasil = nama * ulang
    kembalikan hasil
}
variabel umur = 30
cetak(sapa("hai"), umur)
'

# Fungsi bersarang di dalam fungsi SENGAJA tidak diuji di sini: bentuk itu
# sudah ditolak sejak sebelum anotasi ada ("Harapkan nama fungsi"), jadi
# memasukkannya hanya akan menguji batasan lama, bukan anotasi.
bandingkan "sebagian beranotasi + blok bersarang" \
'fungsi luar(a, b: bilangan) -> kosong {
    variabel x: bilangan = a + b
    jika x > 2 {
        variabel y: teks = "besar"
        cetak(y)
    }
    cetak(x)
}
luar(1, 2)
' \
'fungsi luar(a, b) {
    variabel x = a + b
    jika x > 2 {
        variabel y = "besar"
        cetak(y)
    }
    cetak(x)
}
luar(1, 2)
'

# Anotasi TIDAK boleh mengganggu ':' pada kamus dan irisan.
bandingkan "kamus dan irisan tetap utuh" \
'fungsi f(s: daftar) -> kamus {
    variabel d: kamus = {"a": 1, "b": 2}
    cetak(d["a"], s[1:3])
    kembalikan d
}
f([1, 2, 3, 4])
' \
'fungsi f(s) {
    variabel d = {"a": 1, "b": 2}
    cetak(d["a"], s[1:3])
    kembalikan d
}
f([1, 2, 3, 4])
'

# Anotasi yang salah bentuk = galat kompilasi, bukan diterima diam-diam.
tolak() {
    local nama="$1" kode="$2"
    printf '%s' "$kode" > "$TMP/x.idk"
    if "$INDK" "$TMP/x.idk" >/dev/null 2>&1; then
        echo "GAGAL [$nama]: seharusnya ditolak, tapi diterima"
        gagal=1
    else
        echo "ok   [$nama]"
    fi
}

tolak "':' tanpa nama tipe"  'variabel a: = 1
'
tolak "'->' tanpa nama tipe" 'fungsi f() -> {
}
'

if [ "$gagal" -eq 0 ]; then
    echo "SEMUA UJI ANOTASI BYTECODE LULUS"
else
    echo "ADA UJI ANOTASI BYTECODE YANG GAGAL"
fi
exit "$gagal"
