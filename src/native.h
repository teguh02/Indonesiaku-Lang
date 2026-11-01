#ifndef indk_native_h
#define indk_native_h

#include "common.h"
#include "value.h"
#include "object.h"

typedef struct {
    const char* name;
    NativeFn function;
} NativeFunction;

// ============================================================================
// FUNGSI MATEMATIKA
// ============================================================================

// min(a, b, ...) - Mengembalikan nilai terkecil
Value nativeFnMin(int argCount, Value* args);

// max(a, b, ...) - Mengembalikan nilai terbesar
Value nativeFnMax(int argCount, Value* args);

// abs(x) - Nilai absolut dari x
Value nativeFnAbs(int argCount, Value* args);

// akar(x) - Akar kuadrat dari x
Value nativeFnAkar(int argCount, Value* args);
Value nativeFnSin(int argCount, Value* args);
Value nativeFnCos(int argCount, Value* args);
Value nativeFnTan(int argCount, Value* args);
Value nativeFnAsin(int argCount, Value* args);
Value nativeFnAcos(int argCount, Value* args);
Value nativeFnAtan(int argCount, Value* args);
Value nativeFnAtan2(int argCount, Value* args);
Value nativeFnRadian(int argCount, Value* args);
Value nativeFnDerajat(int argCount, Value* args);

// pangkat(a, b) - a pangkat b
Value nativeFnPangkat(int argCount, Value* args);

// bulat(x) - Membulatkan bilangan
Value nativeFnBulat(int argCount, Value* args);

// lantai(x) - Membulatkan ke bawah
Value nativeFnLantai(int argCount, Value* args);

// atap(x) - Membulatkan ke atas
Value nativeFnAtap(int argCount, Value* args);

// acak() - Bilangan acak 0-1
Value nativeFnAcak(int argCount, Value* args);

// ============================================================================
// ANGKA & TIPE
// ============================================================================

Value nativeFnBulatkan(int argCount, Value* args);        // bulatkan(x[, digit])
Value nativeFnBagiSisa(int argCount, Value* args);        // bagi_sisa(a, b)
Value nativeFnBiner(int argCount, Value* args);           // biner(n)
Value nativeFnOktal(int argCount, Value* args);           // oktal(n)
Value nativeFnHeksa(int argCount, Value* args);           // heksa(n)
Value nativeFnKeBulat(int argCount, Value* args);         // ke_bulat(x)
Value nativeFnKeLogika(int argCount, Value* args);        // ke_logika(x)
Value nativeFnAdalahInstansi(int argCount, Value* args);  // adalah_instansi(o, K)
Value nativeFnBisaDipanggil(int argCount, Value* args);   // bisa_dipanggil(x)
Value nativeFnRepr(int argCount, Value* args);            // repr(x)
Value nativeFnId(int argCount, Value* args);              // id(x)

// ============================================================================
// FUNGSI STRING
// ============================================================================

// panjang(teks) - Mengembalikan panjang string
Value nativeFnPanjang(int argCount, Value* args);
Value nativeFnPanjangByte(int argCount, Value* args);

// huruf_besar(teks) - Ubah ke huruf besar
Value nativeFnHurofBesar(int argCount, Value* args);

// huruf_kecil(teks) - Ubah ke huruf kecil
Value nativeFnHurofKecil(int argCount, Value* args);

// ganti(teks, cari, ganti) - Ganti substring
Value nativeFnGanti(int argCount, Value* args);

// ============================================================================
// FUNGSI LIST
// ============================================================================

// tambah(list, nilai) - Menambahkan nilai ke akhir list
Value nativeFnTambah(int argCount, Value* args);

// hapus(list) - Menghapus dan mengembalikan elemen terakhir list
Value nativeFnHapus(int argCount, Value* args);

// ============================================================================
// FUNGSI KAMUS (DICTIONARY)
// ============================================================================

// kamus() - Membuat kamus kosong
Value nativeFnKamus(int argCount, Value* args);

// punya(kamus, kunci) - Apakah kamus memiliki kunci tersebut
Value nativeFnPunya(int argCount, Value* args);

// hapus_kunci(kamus, kunci) - Menghapus pasangan kunci-nilai
Value nativeFnHapusKunci(int argCount, Value* args);

// enumerasi(urutan[, awal]) - Pasangan [indeks, nilai]
Value nativeFnEnumerasi(int argCount, Value* args);
Value nativeFnTupel(int argCount, Value* args);      // tupel([urutan])
Value nativeFnHimpunan(int argCount, Value* args);   // himpunan([urutan])
Value nativeFnHimpunanHapus(int argCount, Value* args); // s.hapus(x)

// kunci(kamus) - Mengembalikan list berisi semua kunci
Value nativeFnKunci(int argCount, Value* args);

// ============================================================================
// FUNGSI UTILITAS
// ============================================================================

// jenis(obj) - Mengembalikan tipe data dari objek
Value nativeFnJenis(int argCount, Value* args);

// input(prompt) - Membaca input dari pengguna
Value nativeFnInput(int argCount, Value* args);

// henti(sek) - Menunda eksekusi dalam detik
Value nativeFnHenti(int argCount, Value* args);

// ============================================================================
// FUNGSI KONVERSI & FORMAT
// ============================================================================

// ke_angka(teks) - Konversi string ke angka
Value nativeFnKeAngka(int argCount, Value* args);

// ke_teks(nilai) - Konversi nilai apa pun ke string
Value nativeFnKeTeks(int argCount, Value* args);

// format(pola, ...) - Ganti setiap {} di pola dengan argumen
Value nativeFnFormat(int argCount, Value* args);

// acak_bulat(min, maks) - Bilangan bulat acak dalam [min, maks]
Value nativeFnAcakBulat(int argCount, Value* args);

// ============================================================================
// FUNGSI BERKAS (FILE I/O)
// ============================================================================

Value nativeFnBukaBerkas(int argCount, Value* args);   // buka_berkas(path, mode)
Value nativeFnTutup(int argCount, Value* args);         // tutup(berkas)
Value nativeFnBaca(int argCount, Value* args);          // baca(berkas)
Value nativeFnBacaBaris(int argCount, Value* args);     // baca_baris(berkas)
Value nativeFnTulis(int argCount, Value* args);         // tulis(berkas, teks)
Value nativeFnBacaSemua(int argCount, Value* args);     // baca_semua(path)
Value nativeFnTulisSemua(int argCount, Value* args);    // tulis_semua(path, teks)
Value nativeFnTambahBerkas(int argCount, Value* args);  // tambah_berkas(path, teks)
Value nativeFnAdaBerkas(int argCount, Value* args);     // ada_berkas(path)
Value nativeFnHapusBerkas(int argCount, Value* args);   // hapus_berkas(path)

// ============================================================================
// FUNGSI WAKTU & TANGGAL
// ============================================================================

Value nativeFnWaktu(int argCount, Value* args);          // waktu()
Value nativeFnTanggal(int argCount, Value* args);        // tanggal()
Value nativeFnFormatTanggal(int argCount, Value* args);  // format_tanggal(pola)

// ============================================================================
// PRIMITIF TEKS (untuk pustaka teks .idk)
// ============================================================================

Value nativeFnKarakter(int argCount, Value* args);  // karakter(teks, i)
Value nativeFnPotong(int argCount, Value* args);    // potong(teks, mulai, jumlah)
Value nativeFnOrd(int argCount, Value* args);       // ord(karakter)
Value nativeFnChr(int argCount, Value* args);
Value nativeFnCetak(int argCount, Value* args);       // chr(kode)
Value nativeFnHapusPada(int argCount, Value* args); // hapus_pada(list, i)
Value nativeFnSisip(int argCount, Value* args);     // sisip(list, i, nilai)
Value nativeFnRentang(int argCount, Value* args);   // rentang([awal,] akhir[, langkah])

// ============================================================================
// SOKET (TCP) — untuk server HTTP sederhana
// ============================================================================

Value nativeFnSoketDengar(int argCount, Value* args);  // soket_dengar(port)
Value nativeFnSoketTerima(int argCount, Value* args);  // soket_terima(server)
Value nativeFnSoketBaca(int argCount, Value* args);    // soket_baca(klien)
Value nativeFnSoketTulis(int argCount, Value* args);   // soket_tulis(klien, teks)
Value nativeFnSoketTutup(int argCount, Value* args);   // soket_tutup(soket)
Value nativeFnSoketBatasWaktu(int argCount, Value* args); // soket_batas_waktu(soket, detik)

// Close a raw socket fd (used by the GC to reclaim leaked sockets).
void indkCloseSocket(long long fd);

// ============================================================================
// AGREGASI & KOLEKSI (daftar, rentang, teks sebagai iterable)
// ============================================================================

Value nativeFnJumlah(int argCount, Value* args);     // jumlah(d[, awal])
Value nativeFnSemua(int argCount, Value* args);      // semua(d)
Value nativeFnAdaYang(int argCount, Value* args);    // ada_yang(d)
Value nativeFnTerurut(int argCount, Value* args);    // terurut(d[, menurun])
Value nativeFnTerbalik(int argCount, Value* args);   // terbalik(d)
Value nativeFnPasangkan(int argCount, Value* args);  // pasangkan(a, b, ...)
Value nativeFnPeta(int argCount, Value* args);       // peta(f, d)
Value nativeFnSaring(int argCount, Value* args);     // saring(f, d)
Value nativeFnDbBuka(int argCount, Value* args);
Value nativeFnDbTutup(int argCount, Value* args);
Value nativeFnDbBatasTunggu(int argCount, Value* args);
Value nativeFnDbEksekusi(int argCount, Value* args);
Value nativeFnDbAmbilSemua(int argCount, Value* args);
Value nativeFnDbAmbilSatu(int argCount, Value* args);
Value nativeFnDbBarisTerpengaruh(int argCount, Value* args);
Value nativeFnDbIdTerakhir(int argCount, Value* args);
Value nativeFnDbMulai(int argCount, Value* args);
Value nativeFnDbKomit(int argCount, Value* args);
Value nativeFnDbBatal(int argCount, Value* args);
// Sisa Fase 2
Value nativeFnHash(int argCount, Value* args);           // hash(x)
Value nativeFnTurunanDari(int argCount, Value* args);    // turunan_dari(K, Induk)
Value nativeFnKeDesimal(int argCount, Value* args);      // ke_desimal(x)
Value nativeFnDaftar(int argCount, Value* args);         // daftar([urutan])
Value nativeFnHimpunanBeku(int argCount, Value* args);   // himpunan_beku([urutan])
Value nativeFnIter(int argCount, Value* args);           // iter(urutan)
Value nativeFnLanjutkan(int argCount, Value* args);      // lanjutkan(it[, bawaan])

// ============================================================================
// HASH KRIPTOGRAFIS (Fase 6) — mengembalikan teks heksa huruf kecil
// ============================================================================

Value nativeFnMd5(int argCount, Value* args);      // md5(teks | daftar byte)
Value nativeFnSha1(int argCount, Value* args);     // sha1(...)
Value nativeFnSha256(int argCount, Value* args);   // sha256(...)
Value nativeFnSha512(int argCount, Value* args);   // sha512(...)

Value nativeFnSoketSambung(int argCount, Value* args);   // soket_sambung(host, port)

// Atribut dinamis (Fase 6): setattr / getattr / hasattr.
Value nativeFnAturAtribut(int argCount, Value* args);    // atur_atribut(objek, nama, nilai)
Value nativeFnAmbilAtribut(int argCount, Value* args);   // ambil_atribut(objek, nama[, bawaan])
Value nativeFnPunyaAtribut(int argCount, Value* args);   // punya_atribut(objek, nama)

#endif
