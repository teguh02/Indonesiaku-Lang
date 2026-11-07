# Rencana Fitur — API embedding C (`libindk`)

**Status:** selesai
**Fase:** 8 (Ekosistem)
**Target rilis:** `v1.7.0`
**Penulis:** Teguh Rijanandi
**Tanggal:** 2026-11-07

## Masalah

Fase 8 menyebut "API embedding C" sebagai salah satu kapabilitas ekosistem,
tetapi sampai `66dc678` kapabilitas itu **belum ada dan belum punya rencana**,
sehingga tidak terjadwal. Audit PR #81 mencatatnya sebagai salah satu dari tiga
butir yang benar-benar kosong (bersama GUI dan AOT).

Saat ini satu-satunya cara menjalankan Indonesiaku adalah lewat biner `indk`.
Program C lain tidak bisa memakai bahasa ini sebagai bahasa skrip di dalam
prosesnya sendiri — padahal justru itu yang membuat sebuah bahasa kecil
berguna sebagai bahasa konfigurasi atau *plugin* aplikasi lain.

Semua potongan yang dibutuhkan **sudah ada** dan terbukti jalan; yang belum ada
hanyalah permukaan publik yang stabil di atasnya:

| Kebutuhan | Sudah tersedia |
|---|---|
| Siapkan/robohkan runtime | `initVM()`, `freeVM()` (`src/vm.h`) |
| Jalankan sumber | `interpret(const char*)` |
| Daftarkan fungsi C | `defineNative()` — **masih `static`** di `src/vm.c:315` |
| Tipe fungsi native | `NativeFn` (`src/object.h:107`) |
| Baca variabel global modul | `getGlobalValue()` — **masih `static`** |
| Panggil fungsi Indonesiaku | `callFromNative()` (sudah publik) |

Jadi pekerjaannya adalah **membuka dan membungkus**, bukan membangun mesin baru.

## Batasan yang menentukan bentuk API

`src/vm.c:20` mendeklarasikan `VM vm;` sebagai **variabel global tunggal**, dan
seluruh VM mengaksesnya langsung. Konsekuensinya jujur disebut di muka:

- **Satu runtime per proses.** Tidak ada `indk_State*` seperti `lua_State*`.
  Mengubahnya menuntut mengalirkan pointer VM ke hampir setiap fungsi di
  `vm.c`/`compiler.c`/`native.c` — perombakan besar yang tidak dibenarkan
  hanya demi fitur ini.
- **Tidak aman untuk banyak utas.** VM ini satu utas dan tidak punya primitif
  kunci. API embedding tidak boleh menjanjikan sebaliknya.

Keduanya **didokumentasikan sebagai batas**, bukan disembunyikan, dan dicatat
sebagai K22 di `docs/ANALISIS/03_KEPUTUSAN_DESAIN.md`.

## Permukaan API yang diusulkan

Berkas baru `src/indk.h` (satu-satunya header yang perlu di-`#include` oleh
pemakai) dan `src/indk_api.c`:

```c
#include "indk.h"

typedef enum {
    INDK_OK = 0,
    INDK_GALAT_KOMPILASI,
    INDK_GALAT_RUNTIME
} IndkHasil;

void      indk_mulai(void);                     /* initVM  */
void      indk_selesai(void);                   /* freeVM  */
const char* indk_versi(void);                   /* INDK_VERSION */

IndkHasil indk_jalankan(const char* sumber);
IndkHasil indk_jalankan_berkas(const char* jalur);

/* Mendaftarkan fungsi C sebagai builtin Indonesiaku. */
void      indk_daftar_fungsi(const char* nama, IndkFungsiC fungsi);

/* Mengambil nilai global sebagai teks; kosong bila tidak ada. */
bool      indk_ambil_global_teks(const char* nama, char* keluar, size_t maks);
```

`IndkFungsiC` adalah alias `NativeFn` agar tidak ada tipe duplikat:

```c
typedef Value (*IndkFungsiC)(int jumlah, Value* argumen);
```

### Contoh pemakaian (akan ditulis sebagai `examples/embed/halo.c`)

```c
#include "indk.h"
#include <stdio.h>

static Value tambah_dua(int jumlah, Value* argumen) {
    (void)jumlah;
    return NUMBER_VAL(AS_NUMBER(argumen[0]) + 2);
}

int main(void) {
    indk_mulai();
    indk_daftar_fungsi("tambah_dua", tambah_dua);
    indk_jalankan("cetak(tambah_dua(40))");   /* 42 */
    indk_selesai();
    return 0;
}
```

## Berkas yang tersentuh

| Berkas | Perubahan |
|---|---|
| `src/indk.h` | **baru** — satu-satunya header publik |
| `src/indk_api.c` | **baru** — implementasi pembungkus |
| `src/vm.c` | `defineNative` dan `getGlobalValue` dari `static` → publik |
| `src/vm.h` | deklarasi keduanya |
| `Makefile` | `src/indk_api.c` ke `SOURCES`; target `libindk.a` |
| `.github/workflows/test.yml` | **4 daftar sumber mentah** (asan, debug, coverage, fuzz) |
| `tests/fuzz_compile.c` | komentar daftar sumber |
| `website/panduan/02_INSTALASI.md` | perintah build manual |
| `examples/embed/halo.c` | contoh yang benar-benar dikompilasi |
| `tests/embedding.sh` | tes: kompilasi + jalankan + sabotase |
| `website/panduan/09_EMBEDDING.md` | **baru** — bab panduan |
| `docs/ANALISIS/03_KEPUTUSAN_DESAIN.md` | K22 (runtime tunggal, satu utas) |
| `CHANGELOG.md`, `docs/ROADMAP.md` | status |

## Rencana uji

`tests/embedding.sh`, didaftarkan di `tests/test_all.sh` **dan** jalur Windows
di `.github/workflows/test.yml`:

1. `libindk.a` terbentuk dan contoh C tertaut terhadapnya.
2. Program C menjalankan sumber Indonesiaku, stdout **persis** `42`.
3. Fungsi C terdaftar benar-benar terpanggil dari kode Indonesiaku.
4. Sumber yang salah → `INDK_GALAT_KOMPILASI` (bukan crash, bukan 0).
5. Runtime error → `INDK_GALAT_RUNTIME`.
6. `indk_ambil_global_teks` membaca variabel yang di-set skrip.
7. `indk_mulai`/`indk_selesai` dua siklus berurutan tidak bocor (ASan).

**Sabotase wajib** (tiap satu harus membuat tes MERAH):
- `indk_jalankan` selalu mengembalikan `INDK_OK` → uji 4 & 5 merah.
- `indk_daftar_fungsi` jadi no-op → uji 3 merah.
- `indk_ambil_global_teks` selalu `false` → uji 6 merah.

Sesuai pelajaran yang sudah tercatat: setiap pagar disabotase, dan pagar yang
tetap hijau setelah dicabut dihapus karena berarti mati.

## Yang sengaja TIDAK dikerjakan

- **Banyak runtime dalam satu proses** — menuntut membongkar `VM vm` global.
- **Keamanan utas.**
- **Pembungkus Python/Node/Rust** — di luar lingkup "API embedding C".
- **API manipulasi `Value` yang kaya** (bikin daftar/kamus dari C). Versi
  pertama cukup angka, teks, dan boolean; ditambah bila ada pemakai nyata.
