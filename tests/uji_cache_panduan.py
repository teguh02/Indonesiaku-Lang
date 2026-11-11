#!/usr/bin/env python3
"""Verifikasi berkas panduan .md diambil dengan penanda versi.

Nginx menyajikan panduan/*.md tanpa penanda versi, sehingga browser
menyimpannya tanpa batas dan pembaca lama tetap melihat isi lama setelah
situs diperbarui — persis keluhan "kok Fitur Utama masih v1.0.0" padahal
server sudah menyajikan isi baru.

Fungsi fetchText() asli dipotong dari docs.html lalu dijalankan di Node,
supaya yang diuji adalah kode yang benar-benar dikirim ke browser.
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


def main():
    html = DOCS.read_text(encoding="utf8")
    kode = potong_fungsi(html, "fetchText")

    harness = f"""
{kode}
globalThis.window = {{ indkVersi: "1.7.0" }};
const window = globalThis.window;
const diminta = [];
globalThis.fetch = async (u) => {{
  diminta.push(u);
  return {{ ok: true, status: 200, text: async () => "x" }};
}};
(async () => {{
  await fetchText("panduan/01_PERKENALAN.md");
  await fetchText("panduan/README.md?x=1");
  await fetchText("https://raw.githubusercontent.com/a/b/main/CHANGELOG.md");
  // Versi kosong (app.js gagal dimuat) tidak boleh merusak pengambilan.
  window.indkVersi = "";
  await fetchText("panduan/02_INSTALASI.md");
  console.log(JSON.stringify(diminta));
}})();
"""
    tmp = pathlib.Path("/tmp/_uji_cache.mjs")
    tmp.write_text(harness, encoding="utf8")
    r = subprocess.run(["node", str(tmp)], capture_output=True, text=True,
                       timeout=60)
    if r.returncode != 0:
        print(r.stderr[:1200])
        sys.exit("kode JS gagal dijalankan")
    minta = json.loads(r.stdout)
    salah = []

    # 1. Dokumen lokal wajib ber-penanda versi.
    if minta[0] != "panduan/01_PERKENALAN.md?v=1.7.0":
        salah.append(f"panduan tanpa penanda versi: {minta[0]}")

    # 2. URL yang sudah punya query harus memakai &, bukan ? kedua.
    if minta[1] != "panduan/README.md?x=1&v=1.7.0":
        salah.append(f"query ganda salah bentuk: {minta[1]}")

    # 3. URL absolut (raw.github) dibiarkan utuh.
    if "v=" in minta[2]:
        salah.append(f"URL absolut ikut diubah: {minta[2]}")

    # 4. Tanpa versi, URL tetap bisa diambil (jangan kirim 'v=undefined').
    if minta[3] != "panduan/02_INSTALASI.md":
        salah.append(f"versi kosong merusak URL: {minta[3]}")

    for u in minta:
        print("  ok  ", u)

    if salah:
        print(f"\nMELESET ({len(salah)}):")
        for s in salah:
            print("  -", s)
        return 1
    print("\nPANDUAN .MD DIAMBIL DENGAN PENANDA VERSI")
    return 0


if __name__ == "__main__":
    sys.exit(main())
