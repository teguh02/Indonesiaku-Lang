# Rencana Fitur: pengenal berawalan kata kunci `dan`

- **Status:** selesai
- **Target versi:** 1.6.1
- **Pemilik:** teguh02

## 1. Masalah

Setiap pengenal yang diawali `dan` ditolak kompiler:

```indonesiaku
variabel danau = 1
```

```
[baris 1, kolom 10] Kesalahan di 'danau': Harapkan nama variabel.
    variabel danau = 1
             ^^^^^
```

Berlaku juga untuk nama fungsi dan parameter (`fungsi dani()`,
`fungsi f(danau)`), serta untuk `dangkal` — kata yang dipakai di panduan
sendiri ("salinan dangkal", `07_PUSTAKA_STANDAR.md:1077`), sehingga pengguna
bisa menabraknya saat menamai variabel mengikuti istilah di dokumentasi.

Kata kunci lain **tidak** punya masalah ini: `dalamnya`, `jikalau`,
`untukmu`, `kelasku`, `benarkah` semuanya diterima. Jadi ini bukan keputusan
desain, melainkan satu cabang trie yang salah.

Ditemukan saat membandingkan daftar kata kunci di
`editors/vscode-indonesiaku/language-data.js` dengan kata kunci sejati VM.
Metodenya: tulis `variabel <kata> = 1` untuk tiap kandidat dan lihat mana
yang gagal dikompilasi. `dangkal` muncul sebagai "kata kunci" yang tidak ada
di ekstensi — ternyata memang bukan kata kunci, hanya korban pencocokan
prefiks.

## 2. Sebab

`identifierType()` di `src/scanner.c` adalah trie. Setiap daun memanggil
`checkKeyword(start, length, rest, type)` yang memastikan panjang token
**persis** sama dengan kata kuncinya. Satu daun tidak melakukannya:

```c
case 'n': return TOKEN_DAN;        // baris 197, sebelum perbaikan
```

Cabang ini langsung mengembalikan `TOKEN_DAN` begitu tiga huruf pertama
`d`, `a`, `n` cocok, tanpa memeriksa apakah token berhenti di situ. Semua
`dan*` karena itu menjadi operator `dan`.

Ini satu-satunya kemunculan pola `case 'x': return TOKEN_...;` tanpa
`checkKeyword` di seluruh berkas (`grep -nE "case '.': return TOKEN_[A-Z]+;"`
mengembalikan tepat satu baris), jadi tidak ada cabang lain yang perlu
diperbaiki.

## 3. Perubahan

```c
case 'n': return checkKeyword(3, 0, "", TOKEN_DAN);
```

`checkKeyword` dengan `length` 0 hanya memeriksa syarat panjang
(`scanner.current - scanner.start == 3`); `memcmp(..., 0)` terdefinisi dan
selalu mengembalikan 0. Tidak ada cabang khusus yang perlu ditambahkan.

Pilihan ini sengaja mengikuti bentuk tetangganya (`dalam`, `dari`) daripada
menulis perbandingan panjang sendiri, supaya seluruh daun trie terbaca
seragam.

## 4. Berkas tersentuh

| Berkas | Perubahan |
|---|---|
| `src/scanner.c` | satu baris, daun trie `dan` |
| `examples/pengenal_awalan_katakunci.idk` | contoh golden baru |
| `tests/expected/pengenal_awalan_katakunci.out` | keluaran golden |
| `CHANGELOG.md` | entri `Diperbaiki` |

## 5. Rencana uji

Golden baru menguji dua arah sekaligus, karena perbaikan yang terlalu
bersemangat bisa mematikan operatornya:

1. Pengenal berawalan `dan` diterima: `danau`, `dani`, `dangkal`,
   `dangkalx`, `dan2` — sebagai variabel, nama fungsi, dan parameter.
2. Awalan kata kunci lain tetap diterima: `dalamnya`, `jikalau`, `untukmu`,
   `kelasku`, `benarkah`, `atausaja`, `tidakpun`.
3. Kata kuncinya sendiri tetap berfungsi: `benar dan salah`, `salah atau
   benar`, `tidak salah`.
4. Tetangga satu cabang trie tidak rusak: `2 dalam angka`,
   `dari teks impor teks_pangkas`.

Butir 3 dan 4 adalah yang membuat tes ini **membedakan**, bukan sekadar
lewat: tanpa keduanya, perbaikan yang menghapus `TOKEN_DAN` seluruhnya juga
akan hijau.

**Sabotase:** mengembalikan `case 'n': return TOKEN_DAN;` membuat golden
merah di baris pertama (`danau`), exit 65. Diverifikasi.

Suite penuh tetap hijau sebelum perbaikan (331/331), yang berarti tidak ada
satu pun tes lama yang menjaga perilaku ini — justru alasan golden ini perlu
ada.
