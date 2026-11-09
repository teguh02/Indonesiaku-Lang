# Indonesiaku for Visual Studio Code

Dukungan untuk berkas `.idk`:

- syntax highlighting untuk komentar, string, angka, operator, kata kunci, dan fungsi bawaan;
- auto-closing braces, brackets, parentheses, dan tanda kutip;
- snippets untuk fungsi, kondisi, loop, kelas, impor, dan `coba/kecuali`;
- Language Server Protocol: diagnostics sintaks, autocomplete keyword/builtin,
  hover ringkas, dan go-to-definition lokal;
- **Format Document** yang memanggil `indk rapikan`.

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

## Instalasi lokal

```bash
cd editors/vscode-indonesiaku
npm run package
code --install-extension indonesiaku-1.6.0.vsix
```

Atau buka folder ini di VS Code lalu tekan `F5` untuk menjalankan Extension Development Host.

## Cakupan

LSP membutuhkan executable `indk` di `PATH`, atau setelan `indonesiaku.jalurIndk` ke lokasi binary. Server tidak mengeksekusi program untuk memberi diagnostics.
