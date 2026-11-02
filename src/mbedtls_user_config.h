// mbedtls_user_config.h — penyesuaian konfigurasi mbedTLS milik Indonesiaku.
//
// Disertakan SESUDAH mbedtls_config.h bawaan (lewat -DMBEDTLS_USER_CONFIG_FILE),
// jadi di sini kita hanya menambah atau mencabut modul.
//
// KENAPA BERKAS INI ADA DI LUAR src/vendor/: tools/vendor_mbedtls.sh menghapus
// dan menulis ulang seluruh src/vendor/mbedtls saat versi dinaikkan. Setiap
// suntingan di dalam direktori vendor akan hilang tanpa jejak pada pembaruan
// CVE berikutnya -- persis saat kita paling tidak ingin kehilangan konfigurasi
// keamanan.
#ifndef INDK_MBEDTLS_USER_CONFIG_H
#define INDK_MBEDTLS_USER_CONFIG_H

// CCM (Counter with CBC-MAC) tidak dipakai klien HTTPS kita: TLS 1.3 memakai
// AES-GCM atau ChaCha20-Poly1305, dan ciphersuite CCM di TLS 1.2 praktis hanya
// dipakai perangkat tertanam. Mencabutnya mengecilkan permukaan audit --
// alasan yang sama yang membuat kita memilih mbedTLS daripada OpenSSL.
//
// Efek sampingnya: server yang HANYA menawarkan ciphersuite CCM tidak bisa
// dihubungi. Belum pernah ditemui di web publik; bila suatu saat perlu, hapus
// baris ini dan jalankan kembali tests/https_klien.sh.
#undef MBEDTLS_CCM_C

#endif // INDK_MBEDTLS_USER_CONFIG_H
