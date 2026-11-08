# Rencana: penjaga cakupan tes lintas-OS

- **Status:** selesai
- **Target versi:** 1.6.0
- **Jenis:** test / ci

## Masalah

AGENTS.md menyatakan cakupan CI lintas-OS sebagai syarat rilis: setiap OS yang
didukung wajib menjalankan tes atas semua fitur bahasa. Kenyataannya ada dua
celah yang **tidak pernah memerahkan apa pun**, jadi keduanya bisa bertahan
tanpa batas waktu.

### Celah 1: berkas tes yang tidak pernah dipanggil

`tests/test_todo_cli.ps1` dan `tests/test_todo_server.ps1` ada di repo
berbulan-bulan tanpa pernah dieksekusi harness mana pun. Keduanya sudah
diperbaiki di PR sebelumnya, tetapi tidak ada yang mencegah kejadian itu
terulang — berkas tes yang menganggur *terlihat* seperti cakupan.

Audit ulang menemukan satu lagi: `tests/test_output.ps1`.

### Celah 2: sub-suite `.sh` baru otomatis tidak jalan di Windows

`tests/test_all.sh` memanggil 46 sub-suite; daftar Git Bash di
`.github/workflows/test.yml` adalah daftar **eksplisit**, bukan glob. Sebuah
tes portabel yang lupa didaftarkan akan hijau selamanya tanpa pernah
dieksekusi di Windows.

Saat audit: 22 dari 46 tidak jalan di Windows.

## Yang dikerjakan

### 1. Daftarkan yang memang portabel

Dari 22 itu, sebagian besar memang Linux-only karena perkakasnya (docker,
`/proc`, `dpkg`, valgrind, `md5sum`, `mktemp -d`, `python3`). Yang bersih dari
semua itu dan memang kurang:

| Tes | Alasan portabel |
|---|---|
| `tests/versi_konsisten.sh` | hanya `grep`/`sed`, tanpa jaringan |
| `tests/http_pengalihan_server.sh` | `/dev/tcp` + `seq`, tanpa curl, tanpa jalur tertanam |

`tests/test_baca_sumber.sh` sempat masuk kandidat lalu **dicabut**: ia memberi
`/dev/stdin` sebagai argumen path ke interpreter, dan `indk.exe` Windows asli
tidak mengenal jalur itu. Mendaftarkannya akan membuat CI Windows merah karena
tesnya memang tidak berlaku di sana, bukan karena ada regresi.

### 2. Hapus `tests/test_output.ps1`

Memeriksa 5 contoh dengan pencocokan substring. Dua di antaranya
(`prima.idk`, `kalkulator.idk`) **sudah tidak ada** dan di-`continue` diam-diam;
tiga sisanya sudah diuji golden suite secara byte-exact — jauh lebih ketat.
Nilainya nol, jadi dihapus daripada didaftarkan.

### 3. Penjaga mekanis: `tests/cakupan_lintas_os.sh`

Memeriksa tiga hal:

1. tidak ada `.ps1` yatim (dikecualikan: `test_all.ps1` sebagai harness dan
   `benchmark.ps1` yang memang alat ukur manual, bukan gerbang lulus/gagal);
2. setiap tes di daftar izin ikut jalan di Windows;
3. setiap tes itu juga terdaftar di `tests/test_all.sh`.

Sengaja **tidak** menuntut semua 46 sub-suite jalan di Windows — itu akan
memaksa menandai ~20 tes Linux-only dan menghasilkan daftar pengecualian yang
lebih panjang daripada daftar izinnya. Menambah entri ke daftar izin adalah
keputusan sadar; lupa mendaftarkan tes bukan.

## Uji

`bash tests/cakupan_lintas_os.sh` — 32 pemeriksaan.

Sabotase (4 skenario, semuanya **merah**):

| Sabotase | Hasil |
|---|---|
| cabut `versi_konsisten.sh` dari daftar Windows | merah |
| cabut `keamanan_app.sh` dari daftar Windows | merah |
| hentikan panggilan ke `test_todo_cli.ps1` | merah |
| cabut `web_csrf.sh` dari daftar Windows | merah |

## Batasan yang diketahui

Penjaga ini memverifikasi **pendaftaran**, bukan bahwa tesnya benar-benar lulus
di Windows — itu pekerjaan CI. Sebuah tes bisa terdaftar dan tetap merah di
sana; yang dicegah adalah tes yang tak pernah dijalankan sama sekali.

Daftar izin ditulis tangan. Tes portabel baru tetap harus ditambahkan ke sana
secara sadar; penjaga tidak bisa menebak apakah sebuah tes baru portabel.
