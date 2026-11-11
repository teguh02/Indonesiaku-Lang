# Rencana Fitur: uninstaller Windows membuang entri PATH-nya (T-W13)

- **Status:** dikerjakan
- **Target versi:** 1.6.2
- **Pemilik:** pemeliharaan pemasang Windows

## 1. Masalah

Uji rilis 1.6.1 di Windows 11 asli (`docs/TESTING/WINDOWS.md`, WIN-15)
menemukan bahwa `unins000.exe` menghapus folder program, pintasan Start Menu,
dan entri Apps & features, tetapi **meninggalkan entri `PATH` sistem**
`C:\Program Files\Indonesiaku` di
`HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment`, nilai
`Path`.

Penyebabnya ada di `packaging/windows/indonesiaku.iss`. Bagian `[Registry]`
menambahkan `{olddata};{app}` (tugas `tambahpath`), tetapi tidak ada kode yang
membuangnya saat uninstall. Flag Inno `uninsdeletevalue` tidak bisa dipakai,
karena flag itu menghapus **seluruh** nilai `Path`.

Dampaknya: `PATH` menunjuk folder yang sudah tidak ada. Memasang ulang tidak
menggandakan entri (ada `BelumAdaDiPath`), tetapi pengguna yang menghapus
Indonesiaku tetap mendapati `PATH`-nya kotor.

## 2. Perubahan

Hanya pemasang; tidak ada sintaks atau perilaku bahasa yang berubah.

- `[Code]` mendapat `HapusDariPath(PathLama, Dir)` dan
  `CurUninstallStepChanged`. Pada langkah `usUninstall`, uninstaller membaca
  `Path` HKLM, membuang **tepat** entri `{app}`, lalu menulisnya kembali
  sebagai `REG_EXPAND_SZ`.
- `ChangesEnvironment=yes`, agar Windows menyiarkan `WM_SETTINGCHANGE` setelah
  pemasangan maupun uninstall. Terminal baru yang dibuka dari Explorer langsung
  melihat `PATH` yang benar tanpa logout.
- `AppId` dapat ditimpa saat kompilasi (`/DAppIdApp=...`). Nilai bawaannya
  tetap GUID lama, karena AppId harus sama antar versi. Ini hanya dipakai uji
  regresi agar pemasang uji tidak pernah menyentuh pemasangan Indonesiaku
  milik pengguna.

## 3. Semantik pembuangan

- Entri dibandingkan tanpa membedakan huruf besar/kecil, dengan spasi di tepi
  dan garis miring terbalik penutup diabaikan (`C:\Program Files\Indonesiaku\`
  sama dengan `C:\Program Files\Indonesiaku`).
- Semua kemunculan entri itu dibuang. Urutan dan isi entri lain, termasuk
  entri kosong yang sudah ada sebelumnya, dibiarkan apa adanya, dan pembuangan
  tidak meninggalkan `;;` baru.
- Entri peninggalan installer lawas (`C:\Program Files (x86)\Indonesiaku\bin`)
  **tidak** disentuh: itu bukan folder pasangan ini.
- Bila `Path` tidak memuat entri itu (tugas `tambahpath` tidak dipilih), nilai
  registri tidak ditulis ulang sama sekali.
- Entri yang sama yang ditambahkan pengguna sendiri secara manual ikut
  terbuang. Ini disengaja, karena foldernya memang dihapus uninstaller.

## 4. Berkas tersentuh

- `packaging/windows/indonesiaku.iss`
- `tests/pemasang_path.ps1` (baru) — uji regresi install → uninstall sungguhan
- `tests/aset_pemasang.ps1`, `tests/aset_pemasang.sh` — pemeriksaan statis
  bahwa `.iss` memuat kode pembuangan dan `ChangesEnvironment=yes`
- `.github/workflows/test.yml` — job `test-windows` menjalankan
  `tests/pemasang_path.ps1` (runner Windows adalah admin dan memiliki Inno
  Setup 6)
- `CHANGELOG.md`

## 5. Rencana uji

1. `tests/pemasang_path.ps1` mengompilasi `.iss` dengan `iscc`, memakai AppId
   uji dan folder keluaran sementara. Pemasang lalu dipasang secara senyap ke
   folder sementara dengan `/TASKS=tambahpath /NOICONS`. Uji memastikan
   folder itu ada di `Path` HKLM, menjalankan `unins000.exe /VERYSILENT`, lalu
   memastikan:
   - folder terhapus;
   - entri itu tidak ada lagi di `Path`;
   - `Path` HKLM **sama persis byte demi byte** dengan sebelum pemasangan,
     tanpa `;;` tambahan dan tanpa entri lain yang hilang.

   Bila uji gagal, blok `finally` mengembalikan `Path` ke nilai awal dan
   meng-uninstall pemasang uji.
2. Sabotase: dengan `CurUninstallStepChanged` dihapus, uji harus merah.
3. Pemeriksaan statis di `aset_pemasang.{ps1,sh}` berjalan juga di Linux dan
   macOS.
4. Uji manual di Windows 11 asli dengan pemasang hasil build: pasang,
   uninstall, lalu periksa `Path`, sesuai WIN-15.

Hasil uji rilis di `docs/TESTING/WINDOWS.md` (bagian 7 dan 8) **tidak**
diubah di sini. Bagian itu hanya diisi oleh orang yang menguji ulang paket
rilis berikutnya di Windows.
