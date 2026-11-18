# Rencana Fitur: GUI (Fase 8, butir ditutup)

- **Status:** **DITOLAK — di luar ruang lingkup proyek secara permanen**
- **Keputusan:** Teguh R (pemilik proyek), lihat `docs/ROADMAP.md` §Visi
- **Pemilik:** Teguh R

## 1. Keputusan

Indonesiaku berfokus pada **web/server-side** (SSR, REST API, akses
database) — konsep yang sama dengan PHP. PHP sendiri tidak punya jalur
resmi untuk membangun aplikasi desktop atau mobile, dan Indonesiaku
sengaja mengikuti batas yang sama. GUI/windowing native (desktop) **tidak
akan dikerjakan**, bukan sekadar ditunda menunggu keputusan arsitektur.

Dokumen ini sebelumnya mencatat audit rujukan K18 (lihat §2 di bawah,
dipertahankan sebagai jejak sejarah) dan tiga opsi arsitektur (FFI umum /
binding native / vendoring toolkit tunggal). Opsi-opsi itu **tidak lagi
relevan untuk diputuskan** karena fiturnya sendiri sudah ditutup di luar
ruang lingkup, bukan karena salah satu opsi dipilih.

## 2. Jejak sejarah: audit rujukan K18 (sebelum keputusan penutupan)

Fase 8 asli (ANALISIS/04_TEKNIK_LANJUTAN.md §11) merencanakan akses GUI
lewat **FFI gaya `ctypes` via libffi**, supaya skrip `.idk` bisa memanggil
toolkit GUI native (GTK/Qt/dll.) tanpa menulis binding C khusus per
toolkit. Tabel Fase 8 di `docs/ROADMAP.md` sempat mencatat **FFI "❌
dibatalkan beralasan — K18"** untuk GUI juga, padahal K18 hanya menjelaskan
kenapa SQLite di-vendor sebagai amalgamation `.c` tunggal ketimbang
menunggu FFI (SQLite kebetulan punya distribusi "satu file C tanpa
dependensi"). K18 **tidak pernah menyatakan FFI dibatalkan secara umum**.
Audit ini tetap benar secara historis, tapi tidak lagi relevan untuk GUI
karena GUI sudah ditutup di luar ruang lingkup terlepas dari isu FFI.

## 3. Opsi yang SEMPAT dipertimbangkan (tidak lagi diproses)

| Opsi | Catatan |
|---|---|
| A. FFI umum (libffi) | Tidak diproses — fitur ditutup |
| B. Binding native per-toolkit | Tidak diproses — fitur ditutup |
| C. Toolkit tunggal minimal via vendoring | Tidak diproses — fitur ditutup |

## 4. Tindakan yang diambil

- `docs/ROADMAP.md` diperbarui: Fase 8 kini **selesai** (bukan "sebagian"),
  GUI ditandai ❌ di luar ruang lingkup permanen, bukan "blocked".
- `docs/ROADMAP.md` §Visi menambahkan pernyataan ruang lingkup eksplisit
  (web/server-side, bukan desktop/mobile).
- Dokumen ini tetap disimpan (bukan dihapus) sebagai jejak audit dan
  keputusan, supaya pertanyaan "kenapa GUI tidak ada" punya jawaban yang
  tertelusur.

## 5. Kalau ada yang minta GUI/desktop/mobile di masa depan

Jawabannya: di luar ruang lingkup proyek by design, sama seperti PHP tidak
punya jalur resmi bikin aplikasi desktop. Kalau pemilik proyek berubah
pikiran, itu perubahan visi besar yang perlu ditulis ulang dari
`docs/ROADMAP.md` §Visi dulu, bukan ditambal diam-diam di fase ini.
