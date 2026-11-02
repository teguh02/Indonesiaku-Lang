// tls.h — pembungkus tipis mbedTLS untuk klien TLS.
//
// Sengaja TIDAK memperkenalkan tipe objek baru di VM. Sebagai gantinya
// ObjSocket mendapat satu pointer opsional (`tls`): bila terisi, soket itu
// terenkripsi. Akibatnya soket_baca/soket_tulis/soket_tutup yang sudah ada
// bekerja apa adanya, dan pustaka/http.idk hampir tidak berubah.
//
// Alternatifnya -- ObjTls sebagai tipe tersendiri -- menuntut tiga suntingan
// berisiko (newTls, freeObject, blackenObject; lihat AGENTS.md) plus duplikat
// baca/tulis/tutup, demi perilaku yang sama persis.
#ifndef indk_tls_h
#define indk_tls_h

#include <stdbool.h>
#include <stddef.h>

typedef struct IndkTls IndkTls;

// Melakukan handshake TLS pada soket yang SUDAH tersambung.
// `host` dipakai untuk SNI sekaligus verifikasi nama -- keduanya wajib.
// Mengembalikan NULL bila gagal dan mengisi `galat` (pesan berbahasa
// Indonesia yang menjelaskan sebabnya, mis. sertifikat kedaluwarsa).
IndkTls* indkTlsMulai(long long fd, const char* host, char* galat, size_t galatUkuran);

// Mengembalikan jumlah bita yang dibaca, 0 bila koneksi ditutup rapi,
// -1 bila galat, -2 bila batas waktu terlampaui.
int indkTlsBaca(IndkTls* tls, char* buf, size_t ukuran);

// Mengembalikan jumlah bita tertulis, -1 bila galat, -2 bila batas waktu.
int indkTlsTulis(IndkTls* tls, const char* data, size_t panjang);

// Mengirim close_notify lalu membebaskan semuanya. Aman dipanggil dengan NULL.
void indkTlsTutup(IndkTls* tls);

#endif
