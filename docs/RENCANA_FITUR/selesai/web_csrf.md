# Rencana Fitur: proteksi CSRF

- **Status:** selesai
- **Target versi:** 1.6.0
- **Pemilik:** Teguh R

## 1. Masalah

Sesi kini bekerja (WEB-04), dan justru itulah yang membuka lubang ini.
Peramban melampirkan kuki sesi **secara otomatis pada setiap permintaan ke
domain tersebut**, tidak peduli dari halaman mana permintaan itu berasal.

Jadi halaman jahat mana pun dapat memuat:

```html
<form action="https://aplikasi-kita/hapus-akun" method="POST">
<body onload="document.forms[0].submit()">
```

Korban yang sedang masuk membuka halaman itu, peramban mengirim kuki sesinya,
dan server memproses permintaan sebagai tindakan sah korban. Server tidak bisa
membedakannya dari klik tombol asli, karena memang identik — kecuali satu hal:
penyerang **tidak dapat membaca** isi sesi korban.

Di situlah token CSRF bekerja: tindakan yang mengubah keadaan wajib menyertakan
nilai rahasia yang hanya ada di dalam sesi korban.

`SameSite=Lax` yang sudah dikirim `set_kuki()` memang menahan sebagian besar
POST lintas situs, tetapi ia **bukan pengganti token**: nilainya dapat
dilonggarkan aplikasi lain (`SameSite=None` untuk kebutuhan embed), tidak
berlaku pada subdomain yang dikuasai penyerang, dan perilakunya berbeda
antarperamban lama. Pertahanan berlapis, bukan satu lapis.

## 2. Sintaks usulan

```python
impor web

variabel toko = web.TokoSesi()

fungsi formulir(req) {
    variabel sid = web.baca_kuki(req)["sesi"]
    variabel token = web.token_csrf(toko, sid)
    kembalikan web.html(200, "<form method=POST>" +
        "<input type=hidden name=_csrf value=" + token + ">" +
        "</form>")
}

fungsi simpan(req) {
    jika web.periksa_csrf(toko, req) tidak benar {
        kembalikan web.teks(403, "token CSRF tidak sah")
    }
    kembalikan web.teks(200, "tersimpan")
}
```

Tidak ada kata kunci bahasa baru — seluruhnya pustaka `.idk`.

## 3. Semantik

**`web.token_csrf(toko, sid)`** mengembalikan token sesi tersebut, membuatnya
sekali bila belum ada lalu menyimpannya **di dalam data sesi** (kunci
`_csrf`). Token sama selama sesi hidup; `putar()` yang sudah ada ikut
memindahkannya, dan logout otomatis membatalkannya karena sesinya hilang.

Token 32 byte dari `acak_bita()`, sama seperti ID sesi.

**`web.periksa_csrf(toko, req)`** mengembalikan `benar`/`salah`:

1. Metode aman (`GET`, `HEAD`, `OPTIONS`) → `benar` tanpa pemeriksaan.
   Metode ini menurut definisi tidak mengubah keadaan.
2. Tidak ada kuki sesi, atau sesinya mati → `salah`.
3. Token dari permintaan diambil dari `req["form"]["_csrf"]`, atau header
   `X-CSRF-Token` bila permintaannya JSON/AJAX.
4. Dibandingkan dengan token di sesi memakai **perbandingan waktu-tetap**.

**Perbandingan waktu-tetap, bukan `==`.** `==` pada teks berhenti di karakter
pertama yang berbeda, sehingga lama perbandingan membocorkan berapa banyak
awalan yang sudah benar. Dengan cukup percobaan, token dapat ditebak
karakter demi karakter. Karena itu seluruh panjang selalu ditelusuri dan
hasilnya diakumulasi dengan OR.

**`web.wajib_csrf(toko)`** mengembalikan fungsi penjaga agar penangan tidak
perlu menulis cabang 403 berulang-ulang.

### Yang sengaja TIDAK dikerjakan di sini

- **CORS dan trusted-proxy** (sisa WEB-05) — masalah berbeda (siapa boleh
  membaca respons vs siapa boleh memicu aksi), dan masing-masing layak satu PR
  sendiri sesuai aturan repo.
- **Token per-formulir sekali pakai** — token per-sesi sudah menutup serangan
  ini; token sekali pakai memecah tab ganda dan tombol *back*, dan baru
  berguna bila ada kebutuhan anti-replay yang nyata.
- **Double-submit cookie** — tidak perlu karena kita sudah punya penyimpanan
  sesi di server, yang lebih kuat.

## 4. Berkas tersentuh

| Berkas | Perubahan |
|---|---|
| `pustaka/web.idk` | `token_csrf`, `periksa_csrf`, `wajib_csrf`, `_web_banding_tetap` |
| `examples/web_csrf.idk` + `tests/expected/web_csrf.out` | golden |
| `tests/web_csrf.sh` | serangan lintas situs nyata lewat soket |
| `tests/bantu/server_csrf.idk` | server uji |
| `tests/test_all.sh`, `.github/workflows/test.yml` | daftarkan sub-suite |
| `website/panduan/07_PUSTAKA_STANDAR.md` | dokumentasi |
| `docs/TESTING/*.md` | kasus uji ketiga OS |
| `CHANGELOG.md`, `docs/ROADMAP.md` | status WEB-05 (sebagian) |

## 5. Rencana uji

**Golden:** token stabil dalam satu sesi, berubah sesudah `putar()`, metode
aman lolos tanpa token, token salah ditolak, panjang token benar.

**Tes serangan** (`tests/web_csrf.sh`), lewat `/dev/tcp`:

1. POST **dengan kuki sesi tapi tanpa token** → 403. Inilah bentuk persis
   serangan CSRF, dan tanpa fitur ini akan 200.
2. POST dengan token sesi **lain** → 403 (token tidak boleh berlaku lintas
   sesi).
3. POST dengan token benar → 200.
4. GET tanpa token → 200 (metode aman tidak terhalang).
5. Token lewat header `X-CSRF-Token` → 200 (jalur AJAX).
6. Sesudah `putar()`, token lama → 403.

**Sabotase wajib** (masing-masing harus memerahkan tes):

- `periksa_csrf` selalu mengembalikan `benar`;
- perbandingan waktu-tetap diganti `==` **dan** dibuat hanya membandingkan
  awalan (membuktikan tesnya benar-benar menguji seluruh panjang);
- metode `POST` ikut dianggap aman;
- token tidak diikat ke sesi (token sesi lain diterima).
