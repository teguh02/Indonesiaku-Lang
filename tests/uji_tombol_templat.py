#!/usr/bin/env python3
"""Verifikasi tombol template playground tahan kamus-lambat dan cache lama.

Gejalanya: label "TEMPLATE" muncul tapi barisan tombolnya kosong. Penyebabnya
dua-duanya soal kamus i18n, bukan daftar templatnya:

  1. app.js meminta i18n.json TANPA penanda versi, jadi browser menyimpannya
     tanpa batas. Setelah rilis yang menambah kunci pg_tpl_*, halaman lama
     memakai kamus basi -> penjaga di bangunTemplat() menolak membangun.
  2. Penantiannya cuma timer 6 detik. Kalau kamus telat (jaringan lambat,
     atau fetch-nya memang selesai belakangan), timer menyerah diam-diam dan
     tombolnya hilang tanpa pesan galat apa pun.

Yang diuji di sini adalah kode asli dari app.js dan playground/index.html.
"""
import json
import pathlib
import re
import subprocess
import sys

REPO = pathlib.Path("/root/nandafa/Indonesiaku")
APP = REPO / "website/assets/app.js"
PG = REPO / "website/playground/index.html"
I18N = REPO / "website/assets/i18n.json"
TPL = REPO / "website/assets/templat-playground.js"


def potong_fungsi(teks, nama, asal):
    m = re.search(rf"function\s+{nama}\s*\(", teks)
    if not m:
        sys.exit(f"fungsi {nama} tidak ditemukan di {asal}")
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


def main():
    app = APP.read_text(encoding="utf8")
    pg = PG.read_text(encoding="utf8")
    kamus = json.loads(I18N.read_text(encoding="utf8"))
    salah = []

    # --- 1. i18n.json wajib membawa penanda versi -------------------------
    m = re.search(r"var I18N_URL\s*=\s*(.+?);", app, re.S)
    if not m:
        sys.exit("I18N_URL tidak ditemukan di app.js")
    ekspr = " ".join(m.group(1).split())
    if "VERSI" not in ekspr:
        salah.append("app.js: i18n.json diminta tanpa penanda versi -> "
                     "browser memakai kamus basi setelah rilis")

    # Jalankan ekspresi aslinya: URL harus benar-benar ber-?v=.
    harness = f"""
const SCRIPT_SRC = "https://idk-lang.web.id/assets/app.js?v=1.7.0";
const VERSI = (SCRIPT_SRC.match(/[?&]v=(\\d+\\.\\d+\\.\\d+)/) || [])[1] || "";
const I18N_URL = {m.group(1)};
const TANPA = (() => {{
  const SCRIPT_SRC = "https://idk-lang.web.id/assets/app.js";
  const VERSI = (SCRIPT_SRC.match(/[?&]v=(\\d+\\.\\d+\\.\\d+)/) || [])[1] || "";
  return {m.group(1)};
}})();
console.log(JSON.stringify({{ dengan: I18N_URL, tanpa: TANPA }}));
"""
    p = pathlib.Path("/tmp/_uji_i18n_url.mjs")
    p.write_text(harness, encoding="utf8")
    r = subprocess.run(["node", str(p)], capture_output=True, text=True,
                       timeout=60)
    if r.returncode != 0:
        print(r.stderr[:800])
        sys.exit("ekspresi I18N_URL gagal dijalankan")
    url = json.loads(r.stdout)
    if url["dengan"] != "https://idk-lang.web.id/assets/i18n.json?v=1.7.0":
        salah.append(f"I18N_URL salah -> {url['dengan']}")
    # Tanpa ?v= di app.js sendiri, jangan sampai menempel "?v=undefined".
    if url["tanpa"] != "https://idk-lang.web.id/assets/i18n.json":
        salah.append(f"I18N_URL tanpa versi jadi rusak -> {url['tanpa']}")

    # --- 2. kunci yang diminta tombol memang ada di kamus -----------------
    diminta = sorted(set(re.findall(r'label:\s*"([a-z_]+)"',
                                    TPL.read_text(encoding="utf8"))))
    if not diminta:
        salah.append("templat-playground.js: tidak ada label sama sekali")
    for bahasa in ("id", "en"):
        hilang = [k for k in diminta if k not in kamus.get(bahasa, {})]
        if hilang:
            salah.append(f"i18n.json[{bahasa}]: kunci hilang -> {hilang}")

    # --- 3. penantian kamus tidak boleh cuma timer ------------------------
    if 'addEventListener("indk:i18n"' not in pg:
        salah.append("playground: tidak menunggu sinyal indk:i18n -> "
                     "kalau kamus telat, tombol hilang diam-diam")
    if 'dispatchEvent' not in app or 'indk:i18n' not in app:
        salah.append("app.js: sinyal indk:i18n tidak pernah dipancarkan")

    # --- 4. bangunTemplat() asli benar-benar membangun 8 tombol ----------
    kode = potong_fungsi(pg, "bangunTemplat", "playground/index.html")
    harness2 = f"""
globalThis.window = {{}};
globalThis.document = {{ createElement: () => ({{
  dataset: {{}}, style: {{}}, setAttribute() {{}}, addEventListener() {{}},
}}) }};
{TPL.read_text(encoding='utf8')}
const KAMUS = {json.dumps(kamus['id'])};
const t = (k) => (KAMUS[k] !== undefined ? KAMUS[k] : k);
const KELAS_TPL_MATI = "mati", KELAS_TPL_HIDUP = "hidup";
let templatAktif = null;
function tandaiTemplat() {{}}
function pakaiTemplat() {{}}
const dibuat = [];
const templatEl = {{
  textContent: "",
  appendChild: (b) => dibuat.push(b.textContent),
  querySelectorAll: () => [],
}};
{kode}
const ok = bangunTemplat();
console.log(JSON.stringify({{ ok, jumlah: dibuat.length,
                             label: dibuat.slice(0, 3) }}));
"""
    p2 = pathlib.Path("/tmp/_uji_tpl.mjs")
    p2.write_text(harness2, encoding="utf8")
    r2 = subprocess.run(["node", str(p2)], capture_output=True, text=True,
                        timeout=60)
    if r2.returncode != 0:
        print(r2.stderr[:800])
        sys.exit("bangunTemplat() gagal dijalankan")
    h = json.loads(r2.stdout)
    if not h["ok"]:
        salah.append("bangunTemplat() menolak membangun padahal kamus lengkap")
    if h["jumlah"] != len(diminta):
        salah.append(f"tombol terbangun {h['jumlah']}, "
                     f"seharusnya {len(diminta)}")
    for lbl in h["label"]:
        if lbl.startswith("pg_tpl"):
            salah.append(f"label mentah bocor ke tombol -> {lbl}")

    print(f"  ok   I18N_URL = {url['dengan']}")
    print(f"  ok   {len(diminta)} kunci template ada di kamus id & en")
    print(f"  ok   bangunTemplat() -> {h['jumlah']} tombol: "
          f"{', '.join(h['label'])}…")
    print("  ok   playground menunggu sinyal indk:i18n, bukan timer saja")

    if salah:
        print(f"\nMELESET ({len(salah)}):")
        for s in salah:
            print("  -", s)
        return 1
    print("\nTOMBOL TEMPLATE PLAYGROUND TAHAN KAMUS LAMBAT DAN CACHE LAMA")
    return 0


if __name__ == "__main__":
    sys.exit(main())
