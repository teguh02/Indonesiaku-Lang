#!/usr/bin/env python3
"""Verifikasi tombol "Salin sebagai Markdown" di dokumentasi.

Tombol ini menyalin sumber .md apa adanya supaya bisa ditempel ke LLM.
Yang mudah salah: halaman "Riwayat Rilis" dirakit di browser dan TIDAK punya
berkas .md, jadi tautan "Markdown mentah" di sana akan menunjuk alamat
"__rilis__" yang 404 kalau tidak disembunyikan.

Fungsi aslinya dipotong dari docs.html lalu dijalankan di Node dengan DOM
tiruan seadanya, supaya yang diuji adalah kode yang dikirim ke browser.
"""
import json
import pathlib
import re
import subprocess
import sys

REPO = pathlib.Path("/root/nandafa/Indonesiaku")
DOCS = REPO / "website/docs.html"


def potong_fungsi(teks, nama):
    m = re.search(rf"(async\s+)?function\s+{nama}\s*\(", teks)
    if not m:
        sys.exit(f"fungsi {nama} tidak ditemukan di docs.html")
    i = teks.index("{", m.end() - 1)
    dalam = 0
    for j in range(i, len(teks)):
        if teks[j] == "{":
            dalam += 1
        elif teks[j] == "}":
            dalam -= 1
            if dalam == 0:
                return teks[m.start():j + 1]
    sys.exit(f"kurung fungsi {nama} tidak seimbang")


HARNESS = r"""
%s
globalThis.window = { indkVersi: "1.7.0", indkLang: () => "id" };
const window = globalThis.window;

const tombolSalin = { disabled: false };
const tautanMentah = { hidden: false, href: "" };
let currentDoc, mdMentah;

function uji(doc, md) {
  currentDoc = doc;
  mdMentah = md;
  perbaruiTombolMd();
  return { href: tautanMentah.href, hidden: tautanMentah.hidden,
           disabled: tombolSalin.disabled };
}

console.log(JSON.stringify({
  biasa:  uji("panduan/01_PERKENALAN.md", "# isi"),
  rilis:  uji("__rilis__", "# Riwayat"),
  kosong: uji("panduan/02_INSTALASI.md", ""),
}));
"""


def main():
    html = DOCS.read_text(encoding="utf8")
    kode = potong_fungsi(html, "perbaruiTombolMd")

    tmp = pathlib.Path("/tmp/_uji_salin.mjs")
    tmp.write_text(HARNESS % kode, encoding="utf8")
    r = subprocess.run(["node", str(tmp)], capture_output=True, text=True,
                       timeout=60)
    if r.returncode != 0:
        print(r.stderr[:1200])
        sys.exit("kode JS gagal dijalankan")
    h = json.loads(r.stdout)
    salah = []

    # Dokumen biasa: tautan mentah tampil, ber-penanda versi (anti-cache).
    if h["biasa"]["hidden"]:
        salah.append("dokumen biasa: tautan mentah malah disembunyikan")
    if h["biasa"]["href"] != "panduan/01_PERKENALAN.md?v=1.7.0":
        salah.append(f"dokumen biasa: href salah -> {h['biasa']['href']}")
    if h["biasa"]["disabled"]:
        salah.append("dokumen biasa: tombol salin mati padahal ada isi")

    # Riwayat Rilis: tidak ada berkas .md, tautan mentah WAJIB disembunyikan.
    if not h["rilis"]["hidden"]:
        salah.append("Riwayat Rilis: tautan mentah tidak disembunyikan "
                     "-> menunjuk alamat __rilis__ yang 404")
    if "__rilis__" in h["rilis"]["href"]:
        salah.append(f"Riwayat Rilis: href bocor -> {h['rilis']['href']}")
    if h["rilis"]["disabled"]:
        salah.append("Riwayat Rilis: tombol salin mati padahal isinya ada")

    # Belum termuat: tombol harus mati, jangan menyalin string kosong.
    if not h["kosong"]["disabled"]:
        salah.append("dokumen kosong: tombol salin masih hidup")

    # Tombol dan label memang ada di HTML.
    for penanda in ('id="salin-md"', 'id="buka-md"', "salin-md-label",
                    "indkSalinTeks"):
        if penanda not in html:
            salah.append(f"penanda {penanda} tidak ada di docs.html")

    for nama, v in h.items():
        print(f"  ok   {nama:7} hidden={v['hidden']} disabled={v['disabled']} "
              f"href={v['href'] or '-'}")

    if salah:
        print(f"\nMELESET ({len(salah)}):")
        for s in salah:
            print("  -", s)
        return 1
    print("\nTOMBOL SALIN MARKDOWN BERPERILAKU BENAR")
    return 0


if __name__ == "__main__":
    sys.exit(main())
