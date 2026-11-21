# Indonesiaku for Visual Studio Code

Dukungan untuk berkas `.idk`:

- syntax highlighting untuk komentar, string, angka, operator, kata kunci, dan fungsi bawaan;
- auto-closing braces, brackets, parentheses, dan tanda kutip;
- snippets untuk fungsi, kondisi, loop, kelas, impor, dan `coba/kecuali`;
- Language Server Protocol: diagnostics sintaks, autocomplete keyword/builtin,
  hover ringkas, dan go-to-definition lokal;
- **Format Document** yang memanggil `indk rapikan`;
- **debugger** (F5): breakpoint, step, call stack, dan variabel lewat `indk awakutu`.

Server LSP dijalankan sebagai proses Node.js terpisah dan menggunakan
`indk --dis <berkas sementara>` untuk menganalisis sintaks. Kode pengguna
tidak dieksekusi. Diagnostics berlaku saat compiler dapat menunjukkan lokasi;
go-to-definition versi awal hanya mencakup deklarasi dalam dokumen yang sama.

## Formatter

`Format Document` (`Shift+Alt+F`) menjalankan `indk rapikan <berkas>` dan
mengganti isi editor dengan hasilnya. Interpreter dipanggil **tanpa**
`--tulis`, jadi berkas di disk tidak disentuh — VS Code sendiri yang
menerapkan edit, sehingga undo tetap berfungsi.

Berkas yang belum disimpan (`untitled` atau masih kotor) dilewati karena
`indk rapikan` membaca dari disk.

Jika `indk` tidak ada di `PATH`, atur jalurnya:

```json
{ "indonesiaku.jalurIndk": "/jalur/ke/indk" }
```

Uji formatter tanpa membuka editor:

```bash
node editors/vscode-indonesiaku/test-format.js
```

Uji JSON-RPC server tanpa membuka VS Code:

```bash
node editors/vscode-indonesiaku/test-lsp.js [jalur/ke/indk]
```

## Debugger (F5)

Buka berkas `.idk`, klik di gutter untuk memasang breakpoint, lalu tekan
`F5` (atau **Run and Debug → Indonesiaku (awakutu)**). Tanpa `launch.json`
berkas yang sedang terbuka langsung di-debug. Untuk konfigurasi tetap:

```json
{
  "type": "indonesiaku",
  "request": "launch",
  "name": "Debug berkas Indonesiaku",
  "program": "${file}",
  "args": [],
  "stopOnEntry": false
}
```

Debugger ini adalah adapter Debug Adapter Protocol (`debug-adapter.js`,
tanpa dependensi npm) di atas `indk awakutu`, debugger CLI bawaan
interpreter. Interpreter dipilih dari atribut `jalurIndk` atau setelan
`indonesiaku.jalurIndk`. Yang didukung: breakpoint baris, Continue, Step
Over, Step Into, Step Out, Call Stack, panel Variables (Parameter dan
Global), hover/Watch untuk nama variabel, keluaran program di Debug
Console, dan galat runtime beserta kode keluarnya.

Batas yang diwarisi dari `indk awakutu`:

- breakpoint hanya untuk **berkas yang dijalankan**; breakpoint di modul
  yang di-`impor` ditolak (abu-abu), dan nomor barisnya bisa ikut berhenti
  di baris yang sama milik modul lain;
- Variables hanya menampilkan **global dan parameter** fungsi yang sedang
  berhenti, bukan `variabel` lokal di badan fungsi; nama global ditebak
  dari teks sumber, nilai tidak bisa dibuka (tanpa anak);
- Watch/hover hanya menerima **nama**, bukan ekspresi;
- **Pause tidak ada**: program yang sedang berjalan hanya berhenti di
  breakpoint;
- Step Into dan Step Out ditiru adapter (breakpoint sementara di baris
  pertama setiap fungsi; `langkah` berulang sampai tumpukan lebih
  dangkal), sehingga Step Out di dalam loop panjang bisa lambat;
- program yang membaca `input()` bersaing dengan debugger di stdin;
- breakpoint yang diubah saat program berjalan baru berlaku pada henti
  berikutnya.

Uji adapter tanpa membuka VS Code:

```bash
node editors/vscode-indonesiaku/test-dap.js [jalur/ke/indk]
```

## Instalasi lokal

```bash
cd editors/vscode-indonesiaku
npm test        # LSP, formatter, debugger
npm run package
code --install-extension indonesiaku-1.6.0.vsix
```

Atau buka folder ini di VS Code lalu tekan `F5` untuk menjalankan Extension Development Host.

## Cakupan

LSP membutuhkan executable `indk` di `PATH`, atau setelan `indonesiaku.jalurIndk` ke lokasi binary. Server tidak mengeksekusi program untuk memberi diagnostics.
