# Perencanaan Bahasa Indonesiaku

Folder ini berisi **perencanaan bahasa** — apa yang akan dibangun dan kenapa.
Ini dokumentasi internal untuk pengembang, **bukan** panduan pengguna.

- Panduan pengguna (instalasi, sintaks, pustaka standar) ada di
  [`website/panduan/`](../website/panduan/README.md) dan dirender oleh situs di
  `website/`.
- Folder ini menjawab pertanyaan: fitur apa berikutnya, bagaimana semantiknya,
  file mana yang tersentuh, dan bagaimana mengujinya.

## Isi

| File | Isi |
|---|---|
| [`ROADMAP.md`](ROADMAP.md) | Satu-satunya sumber kebenaran roadmap dan status fitur |
| [`ANALISIS/`](ANALISIS/README.md) | Audit kondisi saat ini, peta fitur Python → Indonesiaku, keputusan desain, teknik lanjutan, katalog lengkap fungsi/kelas yang akan diduplikasi |
| [`RENCANA_FITUR/TEMPLATE.md`](RENCANA_FITUR/TEMPLATE.md) | Template rencana per fitur |
| `RENCANA_FITUR/<nama>.md` | Rencana satu fitur (dibuat saat fitur mulai dikerjakan) |
| `RENCANA_FITUR/selesai/` | Arsip rencana fitur yang sudah selesai (catatan keputusan dan semantiknya) |
| [`TESTING/`](TESTING/WINDOWS.md) | Rencana dan hasil uji paket rilis per OS: [`WINDOWS.md`](TESTING/WINDOWS.md), [`MACOS.md`](TESTING/MACOS.md), [`LINUX.md`](TESTING/LINUX.md). Wajib diperbarui setiap fase selesai |

## Aturan

1. **Roadmap hanya di `ROADMAP.md`.** `README.md` dan `CHANGELOG.md` boleh
   menautkannya, tapi jangan menyalin tabel roadmap (dulu ada empat versi yang
   saling bertentangan).
2. Satu fitur = satu file di `RENCANA_FITUR/`, dibuat dari `TEMPLATE.md`.
3. Sebutkan dampaknya ke kode nyata: `src/scanner.c` (token/kata kunci),
   `src/compiler.c` (parsing + codegen), `src/vm.c` (opcode + runtime),
   `src/native.c` (builtin), dan `src/value.c`/`src/object.c` (tipe baru).
4. Setiap fitur wajib punya rencana uji golden (`examples/*.idk` +
   `tests/expected/*.out`) dan catatan pembaruan dokumentasi.
5. Kalau fitur selesai: pindahkan ringkasannya ke `CHANGELOG.md`, perbarui
   `ROADMAP.md`, ubah statusnya menjadi "selesai", lalu pindahkan ke
   `RENCANA_FITUR/selesai/`.

## Konteks singkat

Implementasi: VM bytecode C11 (scanner → compiler Pratt satu-lintasan → chunk →
VM stack + GC mark-sweep). Versi saat ini: `INDK_VERSION` di `src/common.h`.
Arsitektur dan jebakan teknis dijelaskan di [`AGENTS.md`](../AGENTS.md) dan
[`knowledge.md`](../knowledge.md).
