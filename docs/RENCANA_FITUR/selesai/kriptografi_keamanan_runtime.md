# Rencana Fitur: Kriptografi modern dan penguatan keamanan runtime (Fase 18, V1)

- **Status:** selesai (V1: langkah 1, sebagian 2-5 dan 10; langkah 6-9 ditunda)
- **Target versi:** 1.13.0-rc.1 (V1 boleh masuk rilis minor lebih awal, lihat §7)
- **Pemilik:** Teguh Rijanandi

## 1. Masalah

Sebelum V1, Indonesiaku hanya punya *hash* (`md5`, `sha1`, `sha256`,
`sha512`), `hmac_sha256`, `pbkdf2_sha256` dan `acak_bita`. Pengembang web yang
perlu menyimpan kata sandi, mengenkripsi cookie/token, atau menurunkan kunci
harus merakit sendiri dari bagian-bagian itu -- dan merakit kriptografi sendiri
adalah sumber kerentanan paling umum:

```indonesiaku
# Salah semua, tapi tidak ada yang mencegahnya hari ini:
simpan = sha256(sandi)                    # tanpa garam, cepat -> mudah di-brute force
token = ke_teks(acak())                   # rand(), bisa ditebak
sama = hmac_sha256(k, m) == tag_kiriman   # perbandingan berwaktu-tak-tetap
```

Tidak ada enkripsi terautentikasi (AEAD), tidak ada HKDF, tidak ada
perbandingan waktu-tetap, dan tidak ada format hash kata sandi yang
bisa diverifikasi ulang.

## 2. Model ancaman

Empat kelas pihak dipisahkan. V1 hanya **menambah primitif untuk (a)**;
batas untuk (b)-(d) dicatat agar tidak ada klaim berlebihan.

| Kelas | Siapa | Yang dilindungi | Status V1 |
|---|---|---|---|
| (a) Kode aplikasi tepercaya | pengembang yang menulis server `.idk` | kerahasiaan & integritas data aplikasi (sandi, cookie, token, berkas terenkripsi) terhadap penyerang jaringan/basis data | **ditangani**: CSPRNG, AEAD, HKDF, HMAC waktu-tetap, hash kata sandi |
| (b) Kode `.idk` tak tepercaya | pengguna playground, plugin | host, berkas, jaringan, CPU/memori | **ditunda** (langkah 7-8). `--sandbox` menolak lingkungan/proses/soket klien, tapi *bukan* batas keamanan; playground mengandalkan isolasi container (lihat `docker/playground/`) |
| (c) Paket pihak ketiga | penulis paket Fase 14 | rantai pasok | **ditunda** (langkah 9), berinteraksi dengan PR manajer paket |
| (d) Input jaringan/berkas | klien HTTP, berkas unggahan | parser, memori VM | **ditunda** (langkah 6); fuzz compiler & sanitizer CI sudah ada sebagai landasan |

Asumsi untuk (a): penyerang dapat membaca/mengubah *ciphertext*, basis data
hash sandi, dan waktu respons, tetapi tidak dapat membaca memori proses.
**Di luar jangkauan V1:** kunci di memori VM tidak bisa dihapus secara
terjamin (daftar bulat dikelola GC; salinan C di-*zeroize*), serangan
*side-channel* cache pada AES tanpa AES-NI, dan proses yang di-*fork*
(interpreter tidak pernah `fork`; `_jalankan_proses` memulai proses baru).

### Audit singkat batas C ↔ VM yang disentuh V1

- `acak_bita` (native.c): mbedTLS CTR_DRBG + entropi OS (`BCryptGenRandom`,
  `getrandom`/`/dev/urandom`); gagal seed/baca -> `GalatOS`, **tidak pernah**
  jatuh ke `rand()`. DRBG statis aman selama VM satu utas.
- `hashInputBytes` (native.c): menyalin teks/daftar bita ke buffer `malloc`
  sebelum memanggil kode C apa pun, jadi tidak ada pointer ke heap GC yang
  dipegang melintasi alokasi. V1 memakai ulang fungsi ini (kini non-`static`).
- Hasil daftar bita dibangun dengan `push` daftar ke stack VM selama diisi
  (aturan GC-safety di `AGENTS.md`).

## 3. Pilihan library (langkah 2)

**mbedTLS 3.6.2** yang sudah di-vendor untuk TLS (`src/vendor/mbedtls`,
Apache-2.0, dirawat Trusted Firmware, punya proses CVE). Tidak ada dependensi
baru, tidak ada perubahan build lintas OS. Modul yang dipakai sudah aktif di
konfigurasi bawaan: `MBEDTLS_GCM_C`, `MBEDTLS_CHACHAPOLY_C`, `MBEDTLS_HKDF_C`,
`MBEDTLS_PKCS5_C`, `MBEDTLS_CTR_DRBG_C`; `mbedtls_ct_memcmp` untuk
perbandingan waktu-tetap. **Tidak ada algoritme yang ditulis sendiri.**

**Argon2id tidak tersedia** di mbedTLS (juga tidak di PSA 3.6). Pilihan
terkuat yang didukung: **PBKDF2-HMAC-SHA-256, 600.000 iterasi** (rekomendasi
OWASP Password Storage Cheat Sheet 2023 dan sesuai FIPS/NIST SP 800-132),
garam acak 16 bita, keluaran 32 bita. Hash disimpan dalam format string PHC
yang memuat nama algoritme dan iterasi:

```
$pbkdf2-sha256$i=600000$<garam base64 tanpa padding>$<hash base64 tanpa padding>
```

sehingga `perlu_hash_ulang()` dapat memigrasikan hash ke iterasi lebih tinggi
-- atau ke Argon2id bila kelak di-vendor (mis. libargon2 referensi, CC0) --
tanpa memutus hash lama. Kekurangan PBKDF2 dibanding Argon2id (tidak
*memory-hard*, lebih murah di GPU) dicatat di panduan.

## 4. API V1 (modul `kripto`, `pustaka/kripto.idk`)

Nama publik berbahasa Indonesia; native internal ber-awalan `_kripto_` di
`src/kripto.c`. Bita = daftar bulat 0..255 (konvensi `pustaka/bita.idk`);
setiap masukan "data" juga menerima teks (dienkode UTF-8).

| Kelompok | API | Catatan |
|---|---|---|
| CSPRNG | `acak_aman(n)`, `token_heksa(n=32)`, `token_url(n=32)`, `bulat_acak_aman(batas)` | bungkus `acak_bita`; `bulat_acak_aman` memakai *rejection sampling* (tanpa bias modulo) |
| Kunci | `buat_kunci()`, `Kunci(bita)`, `k.bita()` | `cetak(k)` -> `<Kunci 256-bit tersembunyi>`; kunci tidak bocor ke log |
| AEAD mudah | `enkripsi(kunci, pesan, aad="")`, `dekripsi(...)`, `dekripsi_teks(...)` | AES-256-GCM, **nonce acak dibuat sendiri** (pengguna tak bisa memakai ulang); format `01 ‖ nonce(12) ‖ sandi ‖ tag(16)`, bita versi ikut diautentikasi |
| AEAD eksplisit | `aead_enkripsi(alg, kunci, nonce, pesan, aad="")` -> `sandi ‖ tag`; `aead_dekripsi(...)`; `AES_256_GCM`, `CHACHA20_POLY1305`; `nonce_acak()`, `PenghitungNonce()` | untuk interoperabilitas/KAT; panduan menegaskan satu nonce = satu pesan per kunci |
| KDF / MAC | `hkdf_sha256(ikm, panjang, garam="", info="")`, `hmac_sha256(k, m)`, `verifikasi_hmac(k, m, tag)`, `sama_aman(a, b)` | perbandingan waktu-tetap via `mbedtls_ct_memcmp` |
| Kata sandi | `hash_kata_sandi(sandi, iterasi=600000)`, `verifikasi_kata_sandi(sandi, tersimpan)`, `perlu_hash_ulang(tersimpan, iterasi=600000)` | iterasi minimum 10.000; format PHC di atas |
| Galat | `GalatKripto` | tag salah / kunci salah / nonce salah / data rusak -> pesan tunggal yang **tidak** membedakan penyebab (gagal tertutup) |

Native internal: `_kripto_aead_segel`, `_kripto_aead_buka` (mengembalikan
`kosong` saat autentikasi gagal; wrapper menaikkan `GalatKripto`),
`_kripto_hkdf_sha256`, `_kripto_sama`. Panjang kunci (32), nonce (12) dan
data (maks 64 MiB) divalidasi di C -> `GalatNilai`/`GalatTipe`.

Builtin lama (`acak_bita`, `hmac_sha256`, `pbkdf2_sha256`, hash) **tidak
berubah**.

## 5. Pengujian (langkah 10, bagian V1)

- KAT golden `examples/kripto_kat.idk`: HMAC-SHA-256 RFC 4231 TC2,
  HKDF-SHA-256 RFC 5869 TC1 & TC3, PBKDF2-HMAC-SHA-256 RFC 7914 §11,
  AES-256-GCM (spesifikasi GCM McGrew-Viega TC13/14/16),
  ChaCha20-Poly1305 RFC 8439 §2.8.2. Nilai dicocokkan silang dengan
  Python `cryptography`.
- Golden perilaku `examples/kripto_dasar.idk`: putaran enkripsi/dekripsi,
  *tamper* (balik satu bit di nonce/sandi/tag/versi), kunci salah, AAD salah,
  nonce salah, data terpotong, `PenghitungNonce` unik, CSPRNG panjang &
  rentang, kunci tersembunyi saat dicetak, hash sandi benar/salah/rusak,
  `perlu_hash_ulang`.
- `tests/galat/kripto_tag_salah.idk` (+`.err`): kegagalan autentikasi yang
  tidak ditangkap berakhir dengan exit non-nol dan pesan stabil.
- `tests/galat/kripto_kunci_pendek.idk`: panjang kunci salah ditolak.
- Build `-DDEBUG_STRESS_GC -fsanitize=address,undefined` bila toolchain
  mendukung; hasil dicatat di PR.
- **Belum**: injeksi kegagalan RNG (butuh *hook* uji di native), fuzz,
  traversal, sandbox breakout, resource exhaustion (langkah 6-8).

## 6. Dampak dokumentasi & tooling

- [x] `website/panduan/07_PUSTAKA_STANDAR.md` (bagian `kripto`)
- [x] `editors/vscode-indonesiaku/` -- tidak ada builtin publik baru (hanya
      native `_kripto_*` internal), jadi tidak perlu diubah
- [x] `CHANGELOG.md` + `docs/ROADMAP.md`
- [x] `docs/TESTING/{WINDOWS,MACOS,LINUX}.md` blok BERSAMA
- [x] Berkas rencana ini -> `selesai/`

## 7. Ditunda ke V2+ (langkah 6-9 dan sisa 2-5, 10)

- **6. Memory safety**: audit menyeluruh compiler/VM/native, fuzz parser
  protokol (HTTP, JSON, PG/MySQL wire), Valgrind lintas OS.
- **7. Model sandbox/capability**: allowlist eksplisit per profil eksekusi,
  dokumentasi bahwa `--sandbox` bukan batas keamanan, seccomp/AppArmor.
- **8. Resource limits & cancellation**: batas instruksi/memori/ukuran,
  proteksi DoS input eksternal.
- **9. Supply chain**: pinning, pemindaian, checksum/penandatanganan artefak,
  `SECURITY.md` -- dikoordinasikan dengan Fase 14 dan Fase 22.
- **Sisa 2-5**: Argon2id (vendor libargon2), tanda tangan Ed25519
  (mbedTLS 3.6 belum punya Ed25519; opsi: ECDSA P-256 yang sudah ada, atau
  vendor implementasi teraudit), XChaCha20 untuk nonce acak tanpa batas
  2^32 pesan per kunci, tipe `bita` asli agar kunci bisa di-*zeroize*.
- **Sisa 10**: tes kegagalan RNG, fuzz, dan regresi keamanan tiga OS.

## 8. Risiko & alternatif

- **Batas nonce acak 96-bit**: aman sampai ~2^32 pesan per kunci (NIST SP
  800-38D). Panduan menyarankan rotasi kunci; `PenghitungNonce` tersedia
  untuk volume besar.
- **Daftar bulat sebagai bita** lambat (16 bita per elemen) dan tidak bisa
  di-*zeroize*; data dibatasi 64 MiB per panggilan. Tipe `bita` asli adalah
  pekerjaan terpisah.
- **Alternatif ditolak**: OpenSSL/libsodium (dependensi baru, build Windows
  lebih berat); menulis Argon2 sendiri (melanggar "jangan bangun kripto
  sendiri").
