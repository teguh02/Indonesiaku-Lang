# Rencana Fitur: GUI (Fase 8, butir tersisa)

- **Status:** usulan — **BLOCKED pada keputusan arsitektur, lihat §2**
- **Target versi:** belum dijadwalkan (lihat alasan di §2)
- **Pemilik:** Teguh R

## 1. Masalah

Fase 8 asli (ANALISIS/04_TEKNIK_LANJUTAN.md §11) merencanakan akses GUI
lewat **FFI gaya `ctypes` via libffi**, supaya skrip `.idk` bisa memanggil
toolkit GUI native (GTK/Qt/dll.) tanpa menulis binding C khusus per
toolkit. Tabel Fase 8 di `docs/ROADMAP.md` mencatat **FFI "❌ dibatalkan
beralasan — K18"**.

## 2. Temuan audit: rujukan K18 TIDAK berlaku untuk pembatalan ini

Dibaca ulang `docs/ANALISIS/03_KEPUTUSAN_DESAIN.md` K18: isinya **hanya**
alasan kenapa SQLite di-vendor sebagai amalgamation `.c` tunggal ketimbang
menunggu FFI -- karena SQLite kebetulan punya distribusi "satu file C
tanpa dependensi" yang bisa disalin apa adanya (presedenan `src/vendor/`
sudah ada untuk mbedTLS). **K18 tidak pernah menyatakan FFI dibatalkan
secara umum** untuk SEMUA kebutuhan; itu argumen khusus kasus SQLite.

Akibatnya: tabel roadmap Fase 8 salah mengutip K18 sebagai alasan GUI
tidak bisa jalan. GUI **tidak** punya opsi vendor "satu file C" seperti
SQLite -- toolkit GUI (GTK, Qt, native Win32/Cocoa) semuanya butuh
linking dinamis ke pustaka sistem yang besar dan berbeda per OS, jadi
alasan vendoring K18 memang tidak berlaku untuk kasus ini (beda dari
kesalahan kutipnya), tapi untuk alasan yang beda sama sekali.

**Perlu keputusan baru** (bukan sekadar baca ulang K18) sebelum rencana
GUI bisa ditulis konkret: apakah proyek ini membuka FFI umum (libffi)
lagi untuk kasus GUI, atau membatasi diri ke binding native per-toolkit
yang ditulis manual di C (pola sama seperti driver MySQL/PostgreSQL --
protokol/API ditulis langsung di `src/`, bukan generic FFI)?

## 3. Opsi yang dipertimbangkan (belum dipilih)

| Opsi | Plus | Minus |
|---|---|---|
| **A. FFI umum (libffi)** | Satu implementasi, skrip `.idk` bisa panggil toolkit APA PUN tanpa nunggu binding ditulis manual | Permukaan serang besar (panggil fungsi C sembarang dari skrip = tidak ada batas tipe/memori), dependency pihak ketiga, GC-safety antara heap Indonesiaku dan memori C arbitrary jadi sangat rawan use-after-free |
| **B. Binding native per-toolkit** (pola driver DB) | Aman terkontrol (API publik yang di-whitelist, bukan panggilan C bebas), konsisten pola `pustaka/pg.idk`/`my.idk` | Tiap toolkit (GTK/Qt/native) butuh binding terpisah ditulis manual, kerja besar per platform |
| **C. Toolkit tunggal minimal via vendoring** (mis. vendor `nuklear` atau `raylib` single-header, dikompilasi langsung seperti SQLite) | Konsisten K18 (vendoring murni, bukan FFI), satu toolkit lintas-OS, cocok untuk kebutuhan sekolah (gambar sederhana, bukan aplikasi desktop penuh) | Fitur GUI sangat terbatas dibanding GTK/Qt asli, tidak native-looking di tiap OS |

**Rekomendasi awal (bisa didiskusikan)**: Opsi C untuk v1 -- selaras
dengan audiens utama bahasa ini (sekolah/edukasi, lihat `--sandbox` untuk
playground), dan konsisten pola vendoring yang sudah terbukti jalan untuk
SQLite dan mbedTLS. Opsi A (FFI umum) ditunda permanen kecuali ada
permintaan nyata untuk akses toolkit sistem penuh -- risikonya (GC-safety
+ permukaan serang) sepadan dengan risiko yang membuat FFI ditahan di
Fase 15 untuk alasan serupa.

## 4. Kenapa belum ditulis sebagai rencana implementasi penuh

Sesuai `AGENTS.md` ("Plan in docs/ first"), rencana implementasi konkret
(API target, opcode/native baru, tes) baru bisa ditulis SETELAH opsi di
§3 diputuskan oleh pemilik proyek -- menuliskannya sebelum keputusan
berarti menebak arah yang bisa dibuang begitu keputusan beda.

## 5. Tindakan yang DIAMBIL sekarang

- Dokumen ini dibuat sebagai pencatatan audit + opsi, BUKAN rencana
  implementasi final (beda dari `aot_bundel_executable.md` yang sudah
  konkret karena tidak punya blocker arsitektur).
- `docs/ROADMAP.md` tabel Fase 8 diperbaiki: rujukan "K18" untuk FFI
  dihapus (tidak akurat), diganti catatan bahwa GUI menunggu keputusan
  opsi A/B/C di dokumen ini.

## 6. Risiko kalau dipaksa jalan tanpa keputusan ini

Menulis binding GUI apa pun sekarang (misal langsung bikin binding GTK
manual) berisiko dibuang kalau pemilik proyek memilih opsi lain setelah
direview -- lebih mahal daripada menunggu satu keputusan arah dulu.
