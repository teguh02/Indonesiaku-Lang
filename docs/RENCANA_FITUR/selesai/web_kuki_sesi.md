# Rencana Fitur: kuki dan sesi untuk aplikasi web

- **Status:** selesai
- **Target versi:** 1.6.0
- **Pemilik:** Teguh R

## 1. Masalah

Fase 13 (WEB-04) meminta cookie `HttpOnly`/`Secure`/`SameSite`, penyimpanan
sesi, rotasi ID saat login, dan invalidasi saat logout. Hari ini **tidak satu
pun bisa ditulis**, dan penyebabnya satu: respons HTTP tidak punya jalur untuk
header tambahan.

```python
fungsi masuk(req) {
    # tidak ada cara menambahkan Set-Cookie pada respons
    kembalikan web.teks(200, "halo")
}
```

`http_response(kode, tipe_konten, badan)` di `pustaka/http.idk` menyusun
respons dari empat header tetap (`Content-Type`, `Content-Length`,
`Connection`) lalu langsung menutup blok header. Tidak ada parameter untuk
header lain, dan ketiga pembungkusnya di `pustaka/web.idk` (`json`, `teks`,
`html`) mewarisi batas itu.

Akibatnya `Set-Cookie` mustahil dikirim, sehingga sesi, CSRF, dan login
seluruhnya terhalang oleh satu kekurangan yang sama. Inilah akar yang harus
dibuka lebih dulu; sisa butir Fase 13 menumpuk di atasnya.

Prasyarat acak aman sudah selesai: `acak_bita()` (CTR-DRBG mbedTLS) tersedia,
jadi ID sesi tidak perlu memakai `acak()` yang dapat ditebak.

## 2. Sintaks usulan

```python
impor web

fungsi masuk(req) {
    variabel res = web.teks(200, "selamat datang")
    kembalikan web.set_kuki(res, "sesi", "abc123", {
        "httponly": benar,
        "secure": benar,
        "samesite": "Lax",
        "path": "/",
        "max-age": 3600
    })
}

fungsi beranda(req) {
    variabel kuki = web.baca_kuki(req)
    jika "sesi" dalam kuki {
        kembalikan web.teks(200, "halo lagi")
    }
    kembalikan web.teks(401, "silakan masuk")
}
```

Sesi dibangun di atas keduanya:

```python
variabel toko = web.TokoSesi()          # dalam memori
variabel sid = toko.buat({"pengguna": "ani"})
variabel data = toko.ambil(sid)
toko.hapus(sid)
variabel sid_baru = toko.putar(sid)     # rotasi ID, data ikut pindah
```

Tidak ada kata kunci bahasa baru. Seluruhnya pustaka `.idk` — tidak ada
perubahan scanner, compiler, maupun VM.

## 3. Semantik

**`http_response(kode, tipe_konten, badan, header = kosong)`** — parameter
keempat opsional berupa daftar pasangan `[nama, nilai]`, disisipkan sebelum
baris kosong penutup. Bawaannya `kosong` sehingga seluruh pemanggil lama tetap
berjalan tanpa perubahan.

Daftar pasangan, bukan kamus, karena `Set-Cookie` **boleh muncul berkali-kali**
dalam satu respons (RFC 6265 §3) — kamus akan membuang semua kecuali yang
terakhir.

**`web.set_kuki(res, nama, nilai, opsi = kosong)`** menyisipkan satu baris
`Set-Cookie` ke respons yang sudah jadi, mengembalikan respons baru.
`Content-Length` tidak berubah karena hanya header yang bertambah.

Nilai kuki di-encode persen pada karakter yang dilarang RFC 6265
(`;`, `,`, spasi, kutip, backslash, kendali) supaya nilai tidak dapat
menyuntikkan atribut tambahan.

**`web.baca_kuki(req)`** mengurai header `Cookie` menjadi kamus. Nama ganda:
yang pertama menang (sesuai perilaku peramban).

**`web.TokoSesi`** menyimpan sesi di memori proses:

- `buat(data)` → ID 32 byte dari `acak_bita()`, dikodekan heks (64 karakter).
- `ambil(sid)` → data, atau `kosong` bila tidak ada/kedaluwarsa.
- `hapus(sid)` → invalidasi (logout).
- `putar(sid)` → ID baru, data pindah, ID lama dihapus (anti session fixation).
- `bersihkan()` → buang yang kedaluwarsa.

Umur sesi bawaan 3600 detik, dapat diatur lewat `TokoSesi(umur)`.

### Yang sengaja TIDAK dikerjakan di sini

- **CSRF, login, role, rate limit** (langkah 3–4 Fase 13) — menunggu fondasi
  ini mendarat lebih dulu.
- **Penyimpanan sesi di database/berkas** — toko memori cukup untuk satu
  proses, dan server ini memang melayani satu permintaan pada satu waktu.
  Penyimpanan bersama baru berguna setelah ada pekerja paralel (Fase 20).
- **Penandatanganan kuki (signed cookie)** — tidak perlu selama nilai yang
  dikirim hanyalah ID acak 256 bit; data sesungguhnya tidak pernah
  meninggalkan server.

## 4. Berkas tersentuh

| Berkas | Perubahan |
|---|---|
| `pustaka/http.idk` | `http_response` menerima `header` opsional |
| `pustaka/web.idk` | `set_kuki`, `baca_kuki`, `_kuki_encode`, kelas `TokoSesi` |
| `examples/web_kuki_sesi.idk` + `tests/expected/web_kuki_sesi.out` | golden |
| `tests/web_kuki_sesi.sh` | tes server nyata: Set-Cookie, flag, rotasi |
| `tests/test_all.sh`, `.github/workflows/test.yml` | daftarkan sub-suite |
| `tests/test_all.ps1` | jalur Windows |
| `website/panduan/07_PUSTAKA_STANDAR.md` | dokumentasi fungsi baru |
| `docs/TESTING/*.md` | kasus uji ketiga OS |
| `CHANGELOG.md`, `docs/ROADMAP.md` | status WEB-04 |

Tidak ada berkas `.c` baru, jadi Makefile dan empat daftar sumber mentah di
`test.yml` tidak tersentuh.

## 5. Rencana uji

**Golden** (`examples/web_kuki_sesi.idk`): bentuk header `Set-Cookie`, urai
kuki, siklus sesi buat → ambil → putar → hapus. Deterministik — ID sesi acak
tidak pernah dicetak, hanya panjang dan hasil perbandingannya.

**Tes server** (`tests/web_kuki_sesi.sh`), lewat `/dev/tcp` bukan curl supaya
jalan di Git Bash:

1. Respons memuat `Set-Cookie` dengan `HttpOnly`, `Secure`, `SameSite=Lax`.
2. Dua `Set-Cookie` dalam satu respons sama-sama terkirim (inilah alasan
   memakai daftar pasangan, bukan kamus).
3. Permintaan dengan header `Cookie` terbaca penangan.
4. `Content-Length` tetap benar sesudah header bertambah — dihitung dari
   berkas, bukan dari `$(...)` yang memangkas newline.
5. Nilai berisi `;` ter-encode, tidak memecah atribut.

**Sabotase wajib** (setiap satu harus memerahkan tes):

- buang `HttpOnly` dari penyusun atribut;
- `putar()` mengembalikan ID lama alih-alih membuat yang baru;
- `putar()` tidak menghapus ID lama (sesi lama masih hidup);
- `_kuki_encode` dilewati (nilai dengan `;` menyuntikkan atribut);
- `ambil()` tidak memeriksa kedaluwarsa.

**Pemeriksaan tambahan:** `tests/test_panduan.sh` mengeksekusi setiap blok
```indonesiaku di panduan, jadi contoh yang ditulis harus benar-benar jalan.
