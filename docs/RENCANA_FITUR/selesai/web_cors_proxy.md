# Rencana Fitur: CORS dan trusted-proxy

- **Status:** selesai
- **Target versi:** 1.6.0
- **Pemilik:** Teguh R

## 1. Masalah

Dua sisa terakhir WEB-05. Keduanya tentang **percaya kepada pihak luar**, dan
keduanya hari ini tidak dapat ditulis.

### CORS

Peramban memblokir pembacaan respons lintas asal kecuali server mengizinkan
lewat header `Access-Control-Allow-Origin`. Hari ini aplikasi hanya punya satu
jalan keluar, dan itu jalan yang salah:

```python
# satu-satunya yang mungkin hari ini: izinkan SEMUA
kembalikan web.set_header(res, "Access-Control-Allow-Origin", "*")
```

`*` melumpuhkan pertahanan bawaan peramban. Lebih buruk lagi, `*` **tidak
boleh** dipakai bersama kredensial — spesifikasi Fetch melarangnya, dan
peramban akan menolak respons itu. Jadi aplikasi yang memakai kuki sesi
(WEB-04) justru tidak terlayani oleh satu-satunya pilihan yang ada.

Yang juga belum ada: **preflight**. Peramban mengirim `OPTIONS` sebelum
permintaan non-sederhana; tanpa jawaban yang benar, seluruh panggilan
`PUT`/`DELETE`/ber-header-kustom gagal sebelum mencapai penangan.

### Trusted-proxy

Aplikasi di belakang Nginx melihat setiap koneksi datang dari `127.0.0.1`.
Alamat klien sesungguhnya ada di header `X-Forwarded-For` — tetapi header itu
**dikirim klien**, jadi siapa pun boleh memalsukannya:

```
X-Forwarded-For: 1.2.3.4
```

Memercayainya tanpa syarat berarti pembatas percobaan (WEB-06) dapat
dilewati total: penyerang cukup mengganti alamat palsunya setiap kali gagal.
Mengabaikannya berarti seluruh pengguna di belakang proxy berbagi satu alamat
dan saling mengunci.

Jalan keluarnya: percayai header itu **hanya** bila koneksinya benar-benar
datang dari proxy yang dikenal. Itu menuntut alamat peer, dan di sinilah
penghalangnya: `soket_terima()` memanggil `accept(fd, NULL, NULL)` — alamat
peer dibuang. Tidak ada satu pun cara di Indonesiaku untuk mengetahui siapa
yang menghubungi.

## 2. Sintaks usulan

```python
impor web

variabel app = web.Aplikasi()

# CORS: daftar asal yang diizinkan, bukan "*"
app.cors(["https://situs-saya.id"], benar)   # benar = izinkan kredensial

# Trusted-proxy: hanya proxy ini yang boleh menetapkan alamat klien
app.proxy_tepercaya(["127.0.0.1"])

app.rute("GET", "/x", fungsi(req) {
    kembalikan web.teks(200, "dari " + req["alamat"])
})
```

Builtin baru:

```python
soket_alamat_peer(soket)   # -> "127.0.0.1"
```

Tidak ada kata kunci bahasa baru.

## 3. Semantik

### `soket_alamat_peer(soket)` (builtin C)

`getpeername()` + `inet_ntop()`. Memakai `sockaddr_storage`, bukan
`sockaddr_in`: alamat IPv6 tidak muat di `sockaddr_in` dan akan terpotong
diam-diam. Soket tertutup atau keluarga alamat asing → galat, bukan nilai
menyesatkan.

### `app.cors(asal, kredensial = salah, metode, header)`

- `asal` adalah **daftar**, bukan `"*"`. Asal yang tidak terdaftar tidak
  mendapat header CORS sama sekali — peramban lalu memblokir pembacaannya.
- Respons memantulkan asal pemohon (`Access-Control-Allow-Origin: <asal>`),
  bukan `*`, karena hanya bentuk itu yang sah bersama kredensial.
- `Vary: Origin` selalu ikut. Tanpa itu, cache perantara dapat menyajikan
  respons ber-ACAO milik asal lain kepada pemohon berikutnya — kebocoran
  lintas-asal lewat cache.
- `OPTIONS` dijawab **204** lengkap dengan `Allow-Methods`, `Allow-Headers`,
  dan `Max-Age`, tanpa menyentuh penangan rute.

**`*` sengaja tidak didukung.** Memberi jalan pintas untuk melumpuhkan
pertahanan peramban bertentangan dengan tujuan WEB-05, dan aplikasi yang
benar-benar ingin terbuka dapat menuliskan headernya sendiri.

### `app.proxy_tepercaya(daftar)`

`req["alamat"]` berisi alamat klien:

1. koneksi datang dari alamat di `daftar` **dan** ada `X-Forwarded-For` →
   entri **paling kanan** yang bukan proxy tepercaya;
2. selain itu → alamat peer apa adanya, header diabaikan.

Paling kanan, bukan paling kiri: entri kiri ditulis klien dan dapat dipalsukan;
entri kanan ditambahkan proxy kita sendiri. Memakai yang paling kiri adalah
kesalahan klasik yang membuat pembatas laju dapat dilewati.

Tanpa `proxy_tepercaya()`, `req["alamat"]` selalu alamat peer — aman secara
bawaan.

### Yang sengaja TIDAK dikerjakan

- **Pola wildcard asal** (`https://*.situs.id`) — pencocokan subdomain mudah
  salah tulis dan menjadi lubang; daftar eksplisit cukup sampai ada kebutuhan
  nyata.
- **Rentang CIDR untuk proxy** — daftar alamat persis cukup untuk satu atau
  dua proxy; CIDR menuntut parser netmask sendiri.
- **`Access-Control-Expose-Headers`** — hanya perlu bila aplikasi membaca
  header respons kustom dari JavaScript; ditambahkan saat ada yang meminta.

## 4. Berkas tersentuh

| Berkas | Perubahan |
|---|---|
| `src/native.c`, `src/native.h`, `src/vm.c` | builtin `soket_alamat_peer` |
| `pustaka/web.idk` | `cors()`, `proxy_tepercaya()`, `req["alamat"]`, preflight |
| `examples/web_cors.idk` + golden | contoh |
| `tests/web_cors.sh` + `tests/bantu/server_cors.idk` | uji lewat HTTP nyata |
| `tests/test_all.sh`, `.github/workflows/test.yml` | daftarkan |
| `website/panduan/07_PUSTAKA_STANDAR.md` | dokumentasi |
| `editors/vscode-indonesiaku/` | builtin baru → `language-data.js` + tmLanguage |
| `docs/TESTING/*.md` | kasus uji ketiga OS |
| `CHANGELOG.md`, `docs/ROADMAP.md` | WEB-05 tuntas |

Builtin baru = **tidak** ada berkas `.c` baru, jadi Makefile dan empat daftar
sumber di `test.yml` tidak berubah.

## 5. Rencana uji

**Golden:** header CORS untuk asal terdaftar; tidak ada header untuk asal
asing; `Vary: Origin` selalu ada; preflight 204; pemilihan alamat dengan dan
tanpa proxy tepercaya.

**Tes HTTP:** permintaan `OPTIONS` sungguhan dijawab 204 tanpa memanggil
penangan; `X-Forwarded-For` dari peer **tidak** tepercaya diabaikan;
`soket_alamat_peer()` mengembalikan `127.0.0.1` pada koneksi lokal.

**Sabotase wajib:**

- `cors()` memantulkan asal apa pun (asal asing ikut diizinkan);
- `Vary: Origin` dihilangkan;
- `X-Forwarded-For` dipercaya tanpa memeriksa peer;
- entri **paling kiri** XFF yang diambil (dapat dipalsukan klien);
- preflight diteruskan ke penangan alih-alih dijawab 204.
