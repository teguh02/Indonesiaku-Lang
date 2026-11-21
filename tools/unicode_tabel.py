#!/usr/bin/env python3
"""Bangkitkan dan periksa tabel huruf besar/kecil di pustaka/unicode.idk.

    python tools/unicode_tabel.py              # tulis ulang blok TABEL
    python tools/unicode_tabel.py --periksa    # bandingkan modul dengan Python

Tabel diambil dari unicodedata Python yang menjalankan skrip ini (versi
Unicode tercatat di komentar modul). `--periksa` menjalankan indk pada setiap
code point U+0001-U+052F dan ribuan teks acak (termasuk emoji, spasi Unicode
dan sigma Yunani), lalu membandingkan huruf_besar/huruf_kecil/lipat_huruf/
pangkas/pisah_spasi/adalah_spasi dengan str.upper/lower/casefold/strip/
split/isspace. Hasil harus identik.

Jalankan dari akar repo dengan `python -I` (tidak memuat modul dari cwd).
"""
import os
import random
import subprocess
import sys
import tempfile
import unicodedata

AKAR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MODUL = os.path.join(AKAR, "pustaka", "unicode.idk")
RENTANG = range(0, 0x530)
MULAI = "# --- TABEL MULAI (dibangkitkan tools/unicode_tabel.py) ---\n"
AKHIR = "# --- TABEL AKHIR ---\n"


def pasangan(fungsi):
    satu, khusus = [], {}
    for cp in RENTANG:
        c = chr(cp)
        hasil = fungsi(c)
        if hasil != c:
            if len(hasil) == 1:
                satu.append(c + hasil)
            else:
                khusus[cp] = hasil
    return "".join(satu), khusus


def teks_idk(nama, isi, lebar=60):
    potong = [isi[i:i + lebar] for i in range(0, len(isi), lebar)]
    baris = [f"_{nama} = ("]
    for i, p in enumerate(potong):
        aman = p.replace("\\", "\\\\").replace('"', '\\"')
        baris.append(f'    "{aman}"' + (" +" if i < len(potong) - 1 else ""))
    baris.append(")")
    return "\n".join(baris)


def khusus_idk(nama, khusus):
    isi = ", ".join(
        f"{cp}: [{', '.join(str(ord(x)) for x in v)}]" for cp, v in khusus.items()
    )
    return f"_{nama}_KHUSUS_KODE = {{{isi}}}"


def blok():
    bagian = []
    for nama, fungsi in (("BESAR", str.upper), ("KECIL", str.lower), ("LIPAT", str.casefold)):
        satu, khusus = pasangan(fungsi)
        bagian.append(teks_idk("PASANGAN_" + nama, satu))
        bagian.append(khusus_idk(nama, khusus))
    berhuruf = "".join(
        chr(cp) for cp in RENTANG
        if chr(cp).islower() or chr(cp).isupper() or chr(cp).istitle()
    )
    abai = "".join(
        chr(cp) for cp in RENTANG
        if unicodedata.category(chr(cp)) in ("Mn", "Me", "Cf", "Lm", "Sk")
        or chr(cp) in "'.:·"
    )
    bagian.append(teks_idk("BERHURUF", berhuruf))
    bagian.append(teks_idk("ABAI_HURUF", abai))
    return "\n".join(bagian) + "\n"


def tulis():
    with open(MODUL, encoding="utf-8", newline="") as f:
        isi = f.read()
    a = isi.index(MULAI) + len(MULAI)
    b = isi.index(AKHIR)
    with open(MODUL, "w", encoding="utf-8", newline="") as f:
        f.write(isi[:a] + blok() + isi[b:])
    print("ditulis ulang dengan Unicode", unicodedata.unidata_version)


PEMISAH = ""
SKRIP_IDK = r'''
impor unicode
fungsi proses(l) {
    variabel sp = "-"
    jika panjang(l) == 1 {
        sp = "true" jika unicode.adalah_spasi(l) selain "false"
    }
    kembalikan teks_gabung([unicode.huruf_besar(l), unicode.huruf_kecil(l),
        unicode.lipat_huruf(l), unicode.pangkas(l),
        teks_gabung(unicode.pisah_spasi(l), " "), sp], chr(57344))
}
keluaran = []
kini = []
untuk c dalam baca_semua(argumen[0]) {
    jika c == "\n" {
        tambah(keluaran, proses(teks_gabung(kini, "")))
        kini = []
    } selain {
        tambah(kini, c)
    }
}
tambah(keluaran, proses(teks_gabung(kini, "")))
tulis_semua(argumen[1], teks_gabung(keluaran, "\n"))
'''


def periksa(indk):
    random.seed(7)
    kolam = [chr(c) for c in range(1, 0x530) if c not in (10, 13)]
    ekstra = ["😀", "日", " ", "'", ".", "́", "　", " ", " ", "Σ", "σ"]
    baris = list(kolam)
    for _ in range(4000):
        baris.append("".join(random.choice(kolam + ekstra * 20) for _ in range(random.randint(1, 8))))
    harap = [
        PEMISAH.join([
            l.upper(), l.lower(), l.casefold(), l.strip(), " ".join(l.split()),
            str(l.isspace()).lower() if len(l) == 1 else "-",
        ])
        for l in baris
    ]
    with tempfile.TemporaryDirectory() as d:
        masuk, keluar, skrip = (os.path.join(d, n) for n in ("in.txt", "out.txt", "uji.idk"))
        with open(masuk, "w", encoding="utf-8", newline="") as f:
            f.write("\n".join(baris))
        with open(skrip, "w", encoding="utf-8", newline="") as f:
            f.write(SKRIP_IDK)
        lingkungan = dict(os.environ, INDK_PATH=os.path.join(AKAR, "pustaka"))
        subprocess.run([indk, skrip, masuk, keluar], check=True, env=lingkungan)
        with open(keluar, encoding="utf-8", newline="") as f:
            dapat = f.read().split("\n")
    salah = [(ascii(baris[i]), ascii(dapat[i]), ascii(harap[i]))
             for i in range(len(baris)) if dapat[i] != harap[i]]
    print(f"Unicode {unicodedata.unidata_version}: {len(baris)} kasus, {len(salah)} beda")
    for s in salah[:10]:
        print("  masukan", s[0], "\n    indk  ", s[1], "\n    python", s[2])
    return 1 if salah else 0


if __name__ == "__main__":
    if "--periksa" in sys.argv:
        bawaan = os.path.join(AKAR, "indk.exe" if os.name == "nt" else "indk")
        sys.exit(periksa(os.environ.get("INDK", bawaan)))
    tulis()
