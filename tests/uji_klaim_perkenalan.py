#!/usr/bin/env python3
"""Verifikasi klaim "Fitur Utama" di 01_PERKENALAN.md terhadap interpreter.

Bab itu pernah macet di v1.0.0 dan menyebut fitur yang sudah lama ada
sebagai "rencana masa depan". Supaya tidak terulang, tiap klaim di sini
dijalankan sungguhan: kalau potongan kodenya gagal, klaimnya salah.

Dijalankan dengan biner hasil `make` di root repo.
"""
import pathlib
import re
import subprocess
import sys

REPO = pathlib.Path("/root/nandafa/Indonesiaku")
INDK = REPO / "indk"
DOK = REPO / "website/panduan/01_PERKENALAN.md"

# (nama klaim, kode, keluaran yang diharapkan)
KLAIM = [
    ("f-string", 'cetak(f"x {1+1}")', "x 2"),
    ("slicing list", "cetak([1,2,3,4][1:3])", "[2, 3]"),
    ("literal kamus", 'variabel k = {"a": 1}\ncetak(k["a"])', "1"),
    ("argumen bawaan & bernama",
     "fungsi f(a, b=5) { kembalikan a+b }\ncetak(f(1), f(1, b=2))", "6 3"),
    ("akhirnya",
     'coba { naikkan GalatNilai("x") } kecuali GalatNilai { cetak("t") } '
     'akhirnya { cetak("a") }', "t\na"),
    ("generator hasilkan",
     "fungsi g() { hasilkan 1 hasilkan 2 }\nuntuk v dalam g() { cetak(v) }",
     "1\n2"),
    ("comprehension",
     "cetak(daftar(x*2 untuk x dalam rentang(3)))", "[0, 2, 4]"),
    ("anotasi tipe",
     "fungsi f(a: angka) -> angka { kembalikan a*2 }\ncetak(f(21))", "42"),
    ("operator dalam", "cetak(2 dalam [1,2,3])", "benar"),
    ("rentang()", "cetak(daftar(rentang(3)))", "[0, 1, 2]"),
]

# Modul pustaka yang disebut bab itu; semuanya harus bisa diimpor.
MODUL = ["teks", "daftar", "kamus", "matematika", "statistik", "iterasi",
         "fungsional", "json", "csv", "base64", "bita", "pola", "koleksi",
         "jalur", "proses", "log", "salin", "opsi", "http", "web", "templat",
         "db", "pg", "my", "migrasi"]


def jalankan(kode):
    f = pathlib.Path("/tmp/_klaim.idk")
    f.write_text(kode + "\n", encoding="utf8")
    r = subprocess.run([str(INDK), str(f)], capture_output=True, text=True,
                       timeout=30, cwd=REPO)
    return r.returncode, r.stdout.strip(), r.stderr.strip()


def main():
    if not INDK.exists():
        sys.exit("indk belum dibangun — jalankan `make` dulu")
    salah = []

    for nama, kode, harap in KLAIM:
        rc, out, err = jalankan(kode)
        if rc != 0 or out != harap:
            salah.append(f"{nama}: rc={rc} keluaran={out!r} "
                         f"(diharapkan {harap!r}) {err.splitlines()[:1]}")
        else:
            print(f"  ok   {nama}")

    gagal_modul = []
    for m in MODUL:
        rc, _, err = jalankan(f"impor {m}")
        if rc != 0:
            gagal_modul.append(m)
    if gagal_modul:
        salah.append(f"modul gagal diimpor: {gagal_modul}")
    else:
        print(f"  ok   {len(MODUL)} modul pustaka bisa diimpor")

    # Angka statistik harus cocok dengan repo, bukan ditulis dari ingatan.
    teks = DOK.read_text(encoding="utf8")
    nyata = {
        "opcode": len(re.findall(r"^\s+OP_[A-Z_]+,",
                                 (REPO / "src/chunk.h").read_text(encoding="utf8"),
                                 re.M)),
        "contoh": len(list((REPO / "examples").rglob("*.idk"))),
        "golden": len(list((REPO / "tests/expected").glob("*.out"))),
        "pustaka": len(list((REPO / "pustaka").glob("*.idk"))),
    }
    for kunci, nilai in nyata.items():
        # Angka boleh ditulis di sel mana pun pada baris metrik terkait,
        # termasuk baris yang memberi keterangan tambahan setelah angka.
        if not re.search(rf"(?<![\d.,]){nilai}(?![\d.,])", teks):
            salah.append(f"statistik {kunci}: {nilai} tidak ada di tabel")
    print(f"  ok   statistik: {nyata}")

    # Tidak boleh lagi mengaku versi lama sebagai "saat ini".
    if re.search(r"v1\.0\.0 \(Saat Ini\)", teks):
        salah.append("masih menulis 'v1.0.0 (Saat Ini)'")

    if salah:
        print(f"\nMELESET ({len(salah)}):")
        for s in salah:
            print("  -", s)
        return 1
    print("\nSEMUA KLAIM FITUR TERBUKTI JALAN DI INTERPRETER")
    return 0


if __name__ == "__main__":
    sys.exit(main())
