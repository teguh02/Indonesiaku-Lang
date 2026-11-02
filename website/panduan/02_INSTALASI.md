# 2. Panduan Instalasi - Bahasa Pemrograman Indonesiaku

Panduan lengkap instalasi Bahasa Pemrograman Indonesiaku di berbagai sistem operasi: Windows, Linux, dan macOS.

---

## 📋 Daftar Isi

1. [Persiapan Umum](#persiapan-umum)
2. [Instalasi di Windows](#instalasi-di-windows)
3. [Instalasi dengan Docker](#-instalasi-dengan-docker)
4. [Instalasi di Linux](#instalasi-di-linux)
5. [Instalasi di macOS](#instalasi-di-macos)
6. [Verifikasi Instalasi](#verifikasi-instalasi)
7. [Troubleshooting](#pemecahan-masalah)
8. [Build dari Source](#bangun-dari-kode-sumber)
9. [Perbandingan Instalasi Methods](#perbandingan-metode-instalasi)
10. [Ekstensi VS Code & LSP](#ekstensi-vs-code--lsp)

---

## Cara Tercepat: Paket Siap Pasang

Sejak 1.4.0 setiap rilis menyertakan paket yang bisa langsung dipasang, tanpa
perlu kompilasi atau mengatur PATH manual.

| Sistem | Berkas | Perintah |
|---|---|---|
| Debian/Ubuntu | `indonesiaku_<versi>_amd64.deb` | `sudo dpkg -i indonesiaku_<versi>_amd64.deb` |
| Windows | `Indonesiaku-Setup-<versi>.exe` | klik dua kali |
| Linux lain, macOS | `indk-<os>.tar.gz` | ekstrak lalu salin `indk` |

Unduh di [halaman rilis](https://github.com/teguh02/Indonesiaku-Lang/releases/latest).

### Memperbarui tanpa menghapus versi lama

**Tidak perlu meng-uninstall versi lama.** Paket baru menimpa yang lama:

- `.deb` — `sudo dpkg -i` versi baru; dpkg melaporkan `Unpacking ... over (versi lama)`
- Windows — installer memakai AppId yang sama, jadi memasang di tempat yang sama
- `.tar.gz` — salin `indk` yang baru menimpa yang lama

Cek apakah ada versi lebih baru:

```bash
indk --periksa-pembaruan
```

Perintah itu menampilkan versi terpasang, versi terbaru, dan perintah
pemutakhiran yang sesuai sistem operasi Anda. Perintah ini **hanya berjalan
saat Anda memintanya** — menjalankan skrip biasa tidak pernah menghubungi
jaringan.

> Versi sebelum 1.4.0 belum memiliki `--periksa-pembaruan`. Pengguna versi
> lama perlu memeriksa halaman rilis secara manual satu kali; setelah
> diperbarui, perintah di atas tersedia.

**Catatan:** `--periksa-pembaruan` memerlukan `curl` (sudah tersedia di
Windows 10+, macOS, dan hampir semua distribusi Linux). Bila `curl` tidak ada
atau jaringan mati, perintah ini menampilkan URL halaman rilis, bukan gagal.

---

## Persiapan Umum

Sebelum melakukan instalasi, siapkan beberapa tools yang diperlukan:

### Tools yang Dibutuhkan

1. **GCC Compiler** - Untuk compile source code dari C
2. **Git** - Untuk clone repository
3. **Text Editor/IDE** - Untuk menulis program
4. **PowerShell/Bash** - Terminal untuk menjalankan commands

### Verifikasi Koneksi Internet

Pastikan komputer Anda terhubung ke internet untuk:
- Download repository dari GitHub
- Download dependencies (jika diperlukan)
- Mengakses dokumentasi online

---

## Ekstensi VS Code & LSP

Ekstensi di [`editors/vscode-indonesiaku`](https://github.com/teguh02/Indonesiaku/tree/main/editors/vscode-indonesiaku)
menyediakan syntax highlighting, snippets, formatter, serta language server
untuk diagnostics, completion, hover, dan go-to-definition lokal.

### Instalasi lokal

```bash
cd editors/vscode-indonesiaku
npm run package
code --install-extension indonesiaku-0.2.0.vsix
```

Pastikan interpreter `indk` tersedia di `PATH`. Jika tidak, atur path executable
di pengaturan VS Code:

```json
{ "indonesiaku.jalurIndk": "C:/jalur/ke/indk.exe" }
```

Language server menggunakan `indk --dis` untuk menganalisis dokumen tanpa
menjalankan program. Fitur diagnostics tetap bekerja pada dokumen belum
tersimpan; hasil analisis usang dibatalkan saat dokumen berubah.

Detail extension dan tes lokal: [README ekstensi](https://github.com/teguh02/Indonesiaku/blob/main/editors/vscode-indonesiaku/README.md).

---

## Instalasi di Windows

### 🚀 CARA MUDAH - Unduh Paket Siap Pakai (Disarankan)

**Untuk pengguna Windows yang ingin langsung jalan:**

1. **Unduh paket**
   - Buka: https://github.com/teguh02/Indonesiaku-Lang/releases/latest
   - Unduh `indk-windows.zip`

2. **Ekstrak**
   - Klik kanan → *Extract All…*, misalnya ke `C:\Indonesiaku`
   - Di dalamnya ada `indk.exe` beserta folder `pustaka\` dan `examples\`

3. **Verifikasi**
   - Buka **PowerShell** di folder itu
   - Ketik: `.\indk.exe -v` → tampil `1.0.0`
   - Coba contoh: `.\indk.exe examples\hello.idk`

4. **(Opsional) Tambahkan ke PATH**
   - Agar `indk` bisa dipanggil dari mana saja, tambahkan folder tersebut ke
     *Environment Variables → Path*

> **Catatan:** installer `.exe` berwizard (`Indonesiaku-Setup-*.exe`) belum
> tersedia untuk 1.0.0. Skripnya ada di `compiler/indonesiaku_installer.iss`
> tetapi belum dibangun otomatis, jadi untuk sekarang gunakan paket ZIP di atas.

**Isi paket:**
- ✅ `indk.exe` — interpreter, tanpa perlu dependensi tambahan
- ✅ `pustaka/` — pustaka standar, agar `impor matematika` langsung jalan
- ✅ `examples/` + `SPESIFIKASI.md`
- ✅ Start Menu shortcuts
- ✅ Easy uninstall

---

### 🔧 CARA MANUAL — Bangun dari Kode Sumber
**Untuk developer atau yang ingin build dari source code:**

#### Langkah 1: Pasang Git
**Opsi A - Menggunakan Official Installer:**

1. Buka https://git-scm.com/download/win
2. Download Git untuk Windows
3. Jalankan installer (`.exe`)
4. Ikuti wizard instalasi dengan default settings
5. Finish

**Opsi B - Menggunakan Chocolatey (jika sudah installed):**

```powershell
choco install git
```

**Verifikasi:**

```powershell
git --version
# Output: git version 2.x.x...
```

#### Langkah 2: Pasang GCC (MinGW)
**Opsi A - Menggunakan MinGW-w64 Installer:**

1. Buka https://www.mingw-w64.org/
2. Download installer (Online atau Offline)
3. Jalankan installer
4. Pilih settings:
   - Version: Latest (e.g., 11.2.0)
   - Architecture: x86_64 (untuk 64-bit) atau i686 (untuk 32-bit)
   - Threads: posix
   - Exception: seh (untuk 64-bit) atau dwarf (untuk 32-bit)
   - Build revision: Latest
5. Finish dan remember installation path

**Opsi B - Menggunakan Chocolatey:**

```powershell
choco install mingw
```

**Langkah Tambahan - Set Environment Variable:**

Setelah install MinGW, tambahkan ke PATH:

1. Tekan `Win + X` → pilih "System"
2. Klik "Advanced system settings"
3. Klik "Environment Variables"
4. Di "System variables", cari "Path" dan klik "Edit"
5. Klik "New" dan tambahkan path ke MinGW bin folder:
   ```
   C:\Program Files\mingw-w64\x86_64-11.2.0-posix-seh-rt_v9-rev0\mingw64\bin
   ```
   (Sesuaikan dengan path instalasi Anda)
6. Klik OK dan tutup windows

**Verifikasi:**

Buka PowerShell baru dan cek:

```powershell
gcc --version
# Output: gcc (Rev5, Built by MSYS2 project) 11.2.0...

g++ --version
# Output: g++ (Rev5, Built by MSYS2 project) 11.2.0...

make --version
# Output: GNU Make 4.x.x...
```

#### Langkah 3: Clone Repository Indonesiaku

Pilih folder tempat Anda ingin menyimpan project:

```powershell
# Navigasi ke folder yang diinginkan
cd C:\Users\YourName\Documents

# Clone repository
git clone https://github.com/teguh02/Indonesiaku-Lang.git
cd Indonesiaku
```

**Verifikasi struktur:**

```powershell
ls -la
# Seharusnya melihat: src/, examples/, pustaka/, website/, docs/, tests/, dll
```

#### Langkah 4: Build Project

**Opsi A - Menggunakan Make (Recommended):**

```powershell
make
```

**Opsi B - Menggunakan GCC langsung:**

```powershell
gcc -Wall -Wextra -std=c11 -O2 -Isrc -Isrc/vendor/mbedtls/include -o indk.exe `
    -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_DQS=0 -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_MEMSTATUS=0 `
    src/main.c src/chunk.c src/compiler.c src/debug.c src/format.c src/memory.c `
    src/native.c src/db.c src/pola.c src/sistem.c src/tls.c src/sqlite3.c src/object.c `
    src/vendor/mbedtls/library/*.c `
    src/scanner.c src/table.c src/value.c src/vm.c -lm -lws2_32
```

**Output:**

```
indk.exe  # Executable yang berhasil dibuild
```

#### Langkah 5: Uji Hasil Pemasangan
```powershell
# Run REPL
./indk.exe

# Ketik beberapa commands
cetak("Halo, Windows!")
x = 10
cetak(x)

# Exit dengan Ctrl+C
```

#### Langkah 6 (Opsional): Add ke PATH

Agar bisa run `indk` dari mana saja:

1. Tekan `Win + X` → "System"
2. "Advanced system settings" → "Environment Variables"
3. "System variables" → "Path" → "Edit"
4. "New" dan tambahkan path ke folder Indonesiaku:
   ```
   C:\Users\YourName\Documents\Indonesiaku
   ```
5. OK dan restart terminal

---

## 🐳 Instalasi dengan Docker

### Persiapan

Sebelum menggunakan Docker, pastikan:

1. **Install Docker Desktop**
   - Windows: https://www.docker.com/products/docker-desktop
   - macOS: https://www.docker.com/products/docker-desktop
   - Linux: `sudo apt install docker.io` (Ubuntu/Debian)

2. **Verifikasi Docker Installation**
   ```bash
   docker --version
   # Output: Docker version XX.XX.XX
   ```

### Langkah 1: Pull Docker Image

Ambil image Indonesiaku dari Docker Hub:

```bash
docker pull teguh02/indonesiaku:latest
```

**Atau gunakan versi specific:**

```bash
docker pull teguh02/indonesiaku:v1.0.0
```

**Verifikasi image sudah di-pull:**

```bash
docker images
# Output:
# REPOSITORY                TAG       IMAGE ID      CREATED       SIZE
# teguh02/indonesiaku       latest    abc123def456  2 weeks ago    95MB
```

### Langkah 2: Run Container - Interactive (REPL)

**Opsi A - Run REPL dengan Interactive Mode:**

```bash
docker run -it teguh02/indonesiaku:latest
```

Sekarang Anda dalam container dan bisa langsung gunakan REPL:

```
> cetak("Halo dari Docker!")
Halo dari Docker!
> x = 10
> cetak(x)
10
> (tekan Ctrl+D untuk keluar)
```

**Opsi B - Run dengan Custom Name:**

```bash
docker run -it --name indonesiaku-app teguh02/indonesiaku:latest
```

**Exit dari container:**

```
Ctrl+C atau Ctrl+D
```

### Langkah 3: Run Container - File Execution

Untuk menjalankan file `.idk` dari host machine:

**Opsi A - Mount Volume Lokal:**

```bash
# Buat folder project lokal
mkdir -p ~/indonesiaku-projects
cd ~/indonesiaku-projects

# Buat file program
cat > hello.idk << 'EOF'
cetak("Halo dari file di host!")
x = 100
cetak("Nilai x:", x)
EOF

# Run file melalui Docker
docker run -it -v $(pwd):/app teguh02/indonesiaku:latest /usr/local/bin/indk /app/hello.idk
```

**Output:**
```
Halo dari file di host!
Nilai x: 100
```

**Opsi B - Windows PowerShell:**

```powershell
# Buat folder project
New-Item -ItemType Directory -Path "$env:USERPROFILE\indonesiaku-projects" -Force
cd "$env:USERPROFILE\indonesiaku-projects"

# Buat file program
@"
cetak("Halo dari Windows Docker!")
hasil = 5 + 10
cetak(hasil)
"@ | Out-File -Encoding UTF8 hello.idk

# Run file
docker run -it -v ${PWD}:/app teguh02/indonesiaku:latest /usr/local/bin/indk /app/hello.idk
```

### Langkah 4: Multiple Files

Untuk menjalankan beberapa file program:

```bash
# Buat beberapa file
cat > program1.idk << 'EOF'
cetak("Program 1")
EOF

cat > program2.idk << 'EOF'
cetak("Program 2")
EOF

# Run program 1
docker run -it -v $(pwd):/app teguh02/indonesiaku:latest /usr/local/bin/indk /app/program1.idk

# Run program 2
docker run -it -v $(pwd):/app teguh02/indonesiaku:latest /usr/local/bin/indk /app/program2.idk
```

### Langkah 5: Persistent Container

Untuk container yang dapat digunakan kembali:

**Opsi A - Create Named Container:**

```bash
# Create container (tidak running)
docker create -it --name my-indonesiaku -v $(pwd):/app teguh02/indonesiaku:latest

# Start container untuk REPL
docker start -i my-indonesiaku

# Gunakan lagi nanti
docker start -i my-indonesiaku
```

**Opsi B - Run Detached (Background):**

```bash
# Run di background
docker run -d --name indonesiaku-bg -v $(pwd):/app teguh02/indonesiaku:latest sleep 1000

# Execute command di container yang sudah running
docker exec -it indonesiaku-bg /usr/local/bin/indk /app/hello.idk

# Stop container
docker stop indonesiaku-bg

# Remove container
docker rm indonesiaku-bg
```

### Langkah 6: Verifikasi Pemasangan dalam Docker
**Check binary version:**

```bash
docker run --rm teguh02/indonesiaku:latest indk -v
# Output: Indonesiaku v1.0.0
```

**Check binary location:**

```bash
docker run --rm teguh02/indonesiaku:latest which indk
# Output: /usr/local/bin/indk
```

**List available tools dalam image:**

```bash
docker run --rm teguh02/indonesiaku:latest apk list --installed
# Menampilkan semua packages terinstall
```

---

## Lokasi Pustaka Standar

Modul standar (`impor pola`, `dari koleksi impor Penghitung`, ...) ada di
folder `pustaka/`. Interpreter mencarinya **relatif terhadap lokasinya
sendiri**, jadi program bisa dijalankan dari folder mana pun tanpa
pengaturan tambahan. Tata letak yang dikenali:

| Pemasangan | `indk` | `pustaka/` |
|---|---|---|
| Build dari source | `Indonesiaku/indk` | `Indonesiaku/pustaka/` (di sebelahnya) |
| Installer Windows | `{app}\bin\indonesiaku.exe` | `{app}\pustaka\` |
| Docker / Linux | `/usr/local/bin/indk` | `/usr/local/share/indonesiaku/pustaka/` |

Bila memasang sendiri di tempat lain, letakkan `pustaka/` mengikuti salah
satu tata letak di atas, atau arahkan variabel lingkungan `INDK_PATH` ke
foldernya.

---

## Perbandingan Metode Instalasi
| Method | Pros | Cons | Cocok untuk |
|--------|------|------|-------------|
| **Installer (Windows)** | Mudah, auto PATH, 1-click | Windows only | Pengguna casual |
| **Build dari Source** | Full control, bisa customize | Butuh tools, lebih kompleks | Developer, contributor |
| **Docker** | Isolated, portable, konsisten | Butuh Docker, sedikit overhead | Testing, CI/CD, cross-platform |

---

## Perintah Docker yang Berguna
```bash
# List running containers
docker ps

# List all containers (including stopped)
docker ps -a

# View container logs
docker logs <container_name>

# Remove container
docker rm <container_name>

# Remove image
docker rmi teguh02/indonesiaku:latest

# View image info
docker inspect teguh02/indonesiaku:latest

# Tag image with custom name
docker tag teguh02/indonesiaku:latest my-indonesiaku:custom

# Save image to tar
docker save teguh02/indonesiaku:latest > indonesiaku.tar

# Load image from tar
docker load < indonesiaku.tar

# Export container as image
docker commit <container_id> my-indonesiaku:snapshot
```

---

## Pemecahan Masalah Docker
### Image Tidak Ditemukan

```bash
# Search image
docker search indonesiaku

# Pull dengan tag spesifik
docker pull teguh02/indonesiaku:v1.0.0

# List local images
docker images
```

### Izin Ditolak (Linux)

```bash
# Option 1: Use sudo
sudo docker run -it teguh02/indonesiaku:latest

# Option 2: Add user to docker group
sudo usermod -aG docker $USER
newgrp docker
docker run -it teguh02/indonesiaku:latest  # Sekarang bisa tanpa sudo
```

### Kontainer Tidak Bisa Mengakses Berkas Host

```bash
# Pastikan volume mounting syntax benar
docker run -it -v /host/path:/container/path teguh02/indonesiaku:latest

# Windows: gunakan full path
docker run -it -v C:\Users\YourName\projects:/app teguh02/indonesiaku:latest
```

### Image Terlalu Besar / Unduhan Lambat

```bash
# Check image size
docker images

# Gunakan lighter base image (sudah digunakan Alpine 3.20)
# Alpine ~ 95MB, jauh lebih kecil dari Ubuntu ~1GB

# Pull hanya specific layer
docker pull --no-parallel teguh02/indonesiaku:latest
```

---

## Instalasi di Linux

### Langkah 1: Pasang Perkakas Pembangun

**Ubuntu/Debian:**

```bash
sudo apt update
sudo apt install -y build-essential git
```

**Fedora/RHEL/CentOS:**

```bash
sudo dnf install -y gcc gcc-c++ make git
```

**Arch Linux:**

```bash
sudo pacman -S base-devel git
```

**Verifikasi:**

```bash
gcc --version
make --version
git --version
```

### Langkah 2: Clone Repository

```bash
# Navigasi ke folder yang diinginkan
cd ~/

# Clone repository
git clone https://github.com/teguh02/Indonesiaku-Lang.git
cd Indonesiaku
```

**Verifikasi struktur:**

```bash
ls -la
# Seharusnya melihat: src/, examples/, pustaka/, website/, docs/, tests/, dll
```

### Langkah 3: Build Project

**Opsi A - Menggunakan Make:**

```bash
make
```

**Opsi B - Menggunakan GCC langsung:**

```bash
gcc -Wall -Wextra -std=c11 -O2 -Isrc -Isrc/vendor/mbedtls/include -o indk \
    -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_DQS=0 -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_MEMSTATUS=0 \
    src/main.c src/chunk.c src/compiler.c src/debug.c src/format.c src/memory.c \
    src/native.c src/db.c src/pola.c src/sistem.c src/tls.c src/sqlite3.c src/object.c \
    src/vendor/mbedtls/library/*.c \
    src/scanner.c src/table.c src/value.c src/vm.c -lm
```

**Output:**

```
indk  # Executable yang berhasil dibuild
```

### Langkah 4: Uji Hasil Pemasangan
```bash
# Run REPL
./indk

# Ketik beberapa commands
cetak("Halo, Linux!")
x = 10
cetak(x)

# Exit dengan Ctrl+C
```

### Langkah 5 (Opsional): Add ke PATH

Agar bisa run `indk` dari mana saja:

```bash
# Copy executable ke /usr/local/bin
sudo cp indk /usr/local/bin/indk

# Verifikasi
which indk  # Seharusnya: /usr/local/bin/indk

# Sekarang bisa run dari mana saja
indk
```

### Langkah 6 (Opsional): Create Symlink

Atau gunakan symlink (jika ingin keep original di folder project):

```bash
# Create symlink
sudo ln -s ~/Indonesiaku/indk /usr/local/bin/indk

# Verify
indk --help
```

---

## Instalasi di macOS

### Langkah 1: Pasang Xcode Command Line Tools
```bash
xcode-select --install
```

Tunggu instalasi selesai (bisa 5-10 menit).

**Verifikasi:**

```bash
gcc --version
make --version
```

### Langkah 2: Pasang Git
Git sudah included di Xcode Command Line Tools.

**Verifikasi:**

```bash
git --version
# Output: git version 2.x.x...
```

Jika ingin Git yang lebih baru, gunakan Homebrew:

```bash
# Install Homebrew terlebih dahulu jika belum
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install Git
brew install git
```

### Langkah 3: Clone Repository

```bash
# Navigasi ke folder yang diinginkan
cd ~/

# Clone repository
git clone https://github.com/teguh02/Indonesiaku-Lang.git
cd Indonesiaku
```

**Verifikasi struktur:**

```bash
ls -la
# Seharusnya melihat: src/, examples/, pustaka/, website/, docs/, tests/, dll
```

### Langkah 4: Build Project

**Opsi A - Menggunakan Make:**

```bash
make
```

**Opsi B - Menggunakan GCC langsung:**

```bash
gcc -Wall -Wextra -std=c11 -O2 -Isrc -Isrc/vendor/mbedtls/include -o indk \
    -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_DQS=0 -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_MEMSTATUS=0 \
    src/main.c src/chunk.c src/compiler.c src/debug.c src/format.c src/memory.c \
    src/native.c src/db.c src/pola.c src/sistem.c src/tls.c src/sqlite3.c src/object.c \
    src/vendor/mbedtls/library/*.c \
    src/scanner.c src/table.c src/value.c src/vm.c -lm
```

**Output:**

```
indk  # Executable yang berhasil dibuild
```

### Langkah 5: Uji Hasil Pemasangan
```bash
# Run REPL
./indk

# Ketik beberapa commands
cetak("Halo, macOS!")
x = 10
cetak(x)

# Exit dengan Ctrl+C
```

### Langkah 6 (Opsional): Add ke PATH

Agar bisa run `indk` dari mana saja:

**Opsi A - Copy ke /usr/local/bin:**

```bash
# Copy executable
sudo cp indk /usr/local/bin/indk

# Verifikasi
which indk

# Run dari mana saja
indk
```

**Opsi B - Gunakan Homebrew (Advanced):**

Jika Anda ingin create Homebrew formula, ikuti dokumentasi Homebrew.

---

## Verifikasi Instalasi

### Test 1: Check Compiler

```bash
gcc --version
# Output: gcc (...) version X.X.X
```

### Test 2: Check Git

```bash
git --version
# Output: git version X.X.X
```

### Test 3: Check Indonesiaku Binary

```bash
# Windows
.\indk.exe

# Linux/macOS
./indk
```

Output seharusnya:
```
> 
```
(REPL prompt yang siap untuk input)

### Test 4: Hello World

```indonesiaku
cetak("Halo, dunia!")
```

Expected output:
```
Halo, dunia!
```

### Test 5: Simple Calculation

```indonesiaku
x = 10
y = 5
hasil = x + y
cetak("Hasil:", hasil)
```

Expected output:
```
Hasil: 15
```

### Uji 6: Jalankan Berkas Contoh
**Windows:**
```powershell
.\indk.exe examples/hello.idk
```

**Linux/macOS:**
```bash
./indk examples/hello.idk
```

Expected output:
```
Halo, dunia!
```

---

## Pemecahan Masalah
### Kendala di Windows
#### Galat: "gcc: command not found"

**Solusi:**
- Verifikasi MinGW sudah installed (Run `gcc --version`)
- Check PATH environment variable (Control Panel → System → Environment Variables)
- Restart terminal setelah mengubah PATH
- Reinstall MinGW jika perlu

#### Galat: "cannot open source file"

**Solusi:**
```powershell
# Pastikan Anda di folder Indonesiaku
cd C:\path\to\Indonesiaku

# Coba build lagi
make clean
make
```

#### Executable sudah dibuat tapi tidak bisa run

**Solusi:**
```powershell
# Check apakah executable ada
ls indk.exe

# Run dengan full path
.\indk.exe

# Atau dari mana saja jika sudah di PATH
indk.exe
```

### Kendala di Linux/macOS
#### Galat: "make: command not found"

**Solusi:**
```bash
# Install build-essential (Linux)
sudo apt install build-essential

# Atau gunakan GCC langsung (tanpa make)
gcc -Wall -Wextra -std=c11 -O2 -Isrc -Isrc/vendor/mbedtls/include -o indk \
    -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_DQS=0 -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_MEMSTATUS=0 \
    src/main.c src/chunk.c src/compiler.c src/debug.c src/format.c src/memory.c \
    src/native.c src/db.c src/pola.c src/sistem.c src/tls.c src/sqlite3.c src/object.c \
    src/vendor/mbedtls/library/*.c \
    src/scanner.c src/table.c src/value.c src/vm.c -lm
```

#### Galat: "git: command not found"

**Solusi:**
```bash
# Linux
sudo apt install git

# macOS - Install Xcode Command Line Tools
xcode-select --install
```

#### Izin ditolak saat menjalankan ./indk

**Solusi:**
```bash
# Make it executable
chmod +x indk

# Run
./indk
```

#### Galat: "libm.so.6: cannot open shared object file"

**Solusi:**
```bash
# Install required libraries
sudo apt install libm  # atau libc6

# Atau rebuild dengan static linking
gcc -static -Wall -Wextra -std=c11 -O2 -Isrc -o indk \
    src/main.c src/chunk.c src/compiler.c src/debug.c src/format.c \
    src/memory.c src/native.c src/db.c src/pola.c src/sistem.c \
    src/sqlite3.c -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_DQS=0 -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_MEMSTATUS=0 \
    src/object.c src/scanner.c src/table.c src/value.c src/vm.c -lm
```

### Kendala Umum
#### REPL opens tapi tidak bisa input

**Solusi:**
- Check apakah terminal sudah properly configured
- Try input sederhana: `cetak("test")`
- Exit dengan `Ctrl+C`
- Report issue di GitHub jika persist

#### Test file tidak ketemu

**Solusi:**
```bash
# Pastikan Anda di folder Indonesiaku
pwd  # atau cd untuk verifikasi lokasi

# Check examples folder ada
ls examples/

# Run dengan full path
./indk examples/hello.idk
```

#### Build error dengan specific warning

**Solusi:**
- Baca error message carefully
- Check file yang di-mention di error
- Try `make clean` sebelum build lagi
- Update compiler ke versi terbaru

---

## Bangun dari Kode Sumber
Jika ingin build manual atau customize:

### 1. Compile Individual Files

```bash
# Windows (PowerShell)
gcc -Wall -Wextra -std=c11 -c -Isrc src/main.c -o src/main.o
gcc -Wall -Wextra -std=c11 -c -Isrc src/vm.c -o src/vm.o
# ... (repeat untuk semua file)

# Linux/macOS
gcc -Wall -Wextra -std=c11 -c -Isrc src/main.c -o src/main.o
gcc -Wall -Wextra -std=c11 -c -Isrc src/vm.c -o src/vm.o
# ... (repeat untuk semua file)
```

### 2. Link Object Files

```bash
# Windows
gcc -o indk.exe src/main.o src/chunk.o src/compiler.o src/debug.o ^
    src/memory.o src/object.o src/scanner.o src/table.o ^
    src/value.o src/vm.o -lm

# Linux/macOS
gcc -o indk src/main.o src/chunk.o src/compiler.o src/debug.o \
    src/memory.o src/object.o src/scanner.o src/table.o \
    src/value.o src/vm.o -lm
```

### 3. Clean Compilation

```bash
# Linux/macOS
make clean
make

# Windows
del /Q src\*.o indk.exe
gcc -Wall -Wextra -std=c11 -O2 -Isrc -Isrc/vendor/mbedtls/include -o indk.exe `
    -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_DQS=0 -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_MEMSTATUS=0 `
    src/main.c src/chunk.c src/compiler.c src/debug.c src/format.c src/memory.c `
    src/native.c src/db.c src/pola.c src/sistem.c src/tls.c src/sqlite3.c src/object.c `
    src/vendor/mbedtls/library/*.c `
    src/scanner.c src/table.c src/value.c src/vm.c -lm -lws2_32
```

### 4. Debug Build

Tanpa optimization untuk debugging:

```bash
gcc -Wall -Wextra -std=c11 -g -Isrc -o indk \
    src/main.c src/chunk.c src/compiler.c src/debug.c src/format.c \
    src/memory.c src/native.c src/db.c src/pola.c src/sistem.c \
    src/sqlite3.c -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_DQS=0 -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_MEMSTATUS=0 \
    src/object.c src/scanner.c src/table.c src/value.c src/vm.c -lm
```

### 5. Build Rilis
Dengan optimization untuk performance:

```bash
gcc -Wall -Wextra -std=c11 -O3 -Isrc -o indk \
    src/main.c src/chunk.c src/compiler.c src/debug.c src/format.c \
    src/memory.c src/native.c src/db.c src/pola.c src/sistem.c \
    src/sqlite3.c -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_DQS=0 -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_MEMSTATUS=0 \
    src/object.c src/scanner.c src/table.c src/value.c src/vm.c -lm
```

---

## Langkah Berikutnya Setelah Pemasangan
1. **Read Documentation**
   - Baca `website/panduan/01_PERKENALAN.md` untuk overview
   - Baca `website/panduan/03_PANDUAN_SINTAKS.md` untuk learn sintaks

2. **Run Contoh**
   ```bash
   ./indk examples/hello.idk
   ./indk examples/fibonacci.idk
   ./indk examples/fungsi.idk
   ```

3. **Write Your First Program**
   - Create file: `my_first.idk`
   - Write code
   - Run: `./indk my_first.idk`

4. **Join Community**
   - Star repository di GitHub
   - Follow project updates
   - Share masukan

5. **Consider Contributing**
   - Read `website/panduan/04_KONTRIBUSI.md`
   - Submit perbaikan

---

## Rujukan Ringkas — Perbandingan Sistem Operasi
| Aspek | Windows | Linux | macOS |
|-------|---------|-------|-------|
| **Compiler** | MinGW-w64 | GCC (apt) | Xcode CLT |
| **Package Mgr** | Chocolatey | apt/dnf | Homebrew |
| **Executable** | `indk.exe` | `indk` | `indk` |
| **Run REPL** | `.\indk.exe` | `./indk` | `./indk` |
| **Run File** | `.\indk.exe file.idk` | `./indk file.idk` | `./indk file.idk` |
| **Add to PATH** | Environment Vars | ~/.bashrc | ~/.zprofile |
| **Make** | MinGW make atau choco | apt install make | Xcode CLT |

---

## Dukungan
Jika mengalami masalah:

1. **Check Dokumentasi**
   - Baca section Troubleshooting di atas
   - Cek `website/panduan/` folder

2. **GitHub Issues**
   - Search existing issues
   - Create new issue dengan detail:
     - OS & version
     - GCC version
     - Error message lengkap
     - Steps to reproduce

3. **Community**
   - GitHub Discussions
   - Email: teguhrijanandi02@gmail.com

---

**Selamat! Anda sudah berhasil install Bahasa Pemrograman Indonesiaku!**

Mari mulai programming dengan Bahasa Indonesia! 🚀

---

Versi: 1.0  
Terakhir diperbarui: 28 Oktober 2025  
Platform: Windows, Linux, macOS
