// tls.c — klien TLS di atas mbedTLS yang di-vendor (src/vendor/mbedtls).
//
// Prinsip yang tidak boleh dilanggar di berkas ini: TIDAK ADA jalan untuk
// mematikan verifikasi sertifikat. Tidak ada parameter, tidak ada variabel
// lingkungan, tidak ada "mode pengembangan". Kalau pengguna perlu mempercayai
// penerbit sendiri, jalurnya adalah MENAMBAH CA lewat INDK_CA_BUNDLE, bukan
// melemahkan pemeriksaan.

#include "tls.h"

#include <stdio.h>
#include <stdlib.h>   // calloc, free, getenv
#include <string.h>

#include "mbedtls/ssl.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/error.h"

struct IndkTls {
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config cfg;
    mbedtls_x509_crt ca;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context drbg;
    mbedtls_net_context net;
};

// Jalur bundel CA yang dicoba berurutan bila INDK_CA_BUNDLE tidak disetel.
// Windows sengaja tidak ada di daftar: Certificate Store butuh kode Win32
// tersendiri, dan menebak jalur PEM di Windows hanya akan gagal dengan pesan
// yang membingungkan. Di sana INDK_CA_BUNDLE wajib (didokumentasikan).
static const char* JALUR_CA[] = {
    "/etc/ssl/certs/ca-certificates.crt",  // Debian, Ubuntu, Alpine
    "/etc/pki/tls/certs/ca-bundle.crt",    // Fedora, RHEL
    "/etc/ssl/cert.pem",                   // macOS, BSD
    NULL,
};

// Memuat bundel CA. Mengembalikan false bila tidak ada satu pun yang berhasil.
// PENTING: gagal memuat CA harus menggagalkan koneksi, bukan melanjutkan tanpa
// verifikasi -- itulah bedanya "aman" dan "kelihatan aman".
static bool muatCa(mbedtls_x509_crt* ca, char* galat, size_t galatUkuran) {
    // mbedtls_x509_crt_parse_file() mengembalikan 0 bila SEMUA sertifikat
    // terurai, nilai POSITIF berisi jumlah yang gagal bila sebagian terurai,
    // dan negatif bila berkasnya sendiri tak terbaca. Menuntut == 0 berarti
    // satu sertifikat usang di dalam bundel membuang seluruh bundel: itu yang
    // terjadi di macOS, dan akibatnya setiap host sah ditolak dengan
    // "penerbit tidak tepercaya" -- pesan yang menuduh server padahal
    // masalahnya ada di sisi kita.
    const char* dariEnv = getenv("INDK_CA_BUNDLE");
    if (dariEnv != NULL && dariEnv[0] != '\0') {
        if (mbedtls_x509_crt_parse_file(ca, dariEnv) >= 0 && ca->version != 0) return true;
        snprintf(galat, galatUkuran,
                 "INDK_CA_BUNDLE menunjuk '%s' tetapi berkas itu tidak bisa dibaca "
                 "sebagai bundel CA (PEM)", dariEnv);
        return false;
    }
    for (int i = 0; JALUR_CA[i] != NULL; i++) {
        // ca->version != 0 memastikan setidaknya SATU sertifikat benar-benar
        // masuk; tanpa itu bundel kosong akan lolos sebagai "berhasil".
        if (mbedtls_x509_crt_parse_file(ca, JALUR_CA[i]) >= 0 && ca->version != 0) {
            return true;
        }
    }
    snprintf(galat, galatUkuran,
             "tidak menemukan bundel CA sistem; setel INDK_CA_BUNDLE ke berkas PEM");
    return false;
}

// Menerjemahkan hasil verifikasi menjadi pesan yang memberi tahu penyebabnya.
// "handshake gagal" saja membuat pengguna menebak-nebak; menyebut sertifikat
// kedaluwarsa vs nama tidak cocok vs penerbit tak dikenal langsung menunjuk
// masalahnya.
static void jelaskanVerifikasi(uint32_t flag, const char* host,
                               char* galat, size_t galatUkuran) {
    if (flag & MBEDTLS_X509_BADCERT_EXPIRED) {
        snprintf(galat, galatUkuran, "sertifikat %s sudah kedaluwarsa", host);
    } else if (flag & MBEDTLS_X509_BADCERT_FUTURE) {
        snprintf(galat, galatUkuran, "sertifikat %s belum berlaku", host);
    } else if (flag & MBEDTLS_X509_BADCERT_CN_MISMATCH) {
        snprintf(galat, galatUkuran, "sertifikat tidak diterbitkan untuk %s", host);
    } else if (flag & MBEDTLS_X509_BADCERT_NOT_TRUSTED) {
        snprintf(galat, galatUkuran,
                 "penerbit sertifikat %s tidak tepercaya (self-signed atau akar tak dikenal)",
                 host);
    } else if (flag & MBEDTLS_X509_BADCERT_REVOKED) {
        snprintf(galat, galatUkuran, "sertifikat %s sudah dicabut", host);
    } else {
        snprintf(galat, galatUkuran,
                 "verifikasi sertifikat %s gagal (kode 0x%08x)", host, (unsigned)flag);
    }
}

IndkTls* indkTlsMulai(long long fd, const char* host, char* galat, size_t galatUkuran) {
    IndkTls* t = (IndkTls*)calloc(1, sizeof(IndkTls));
    if (t == NULL) {
        snprintf(galat, galatUkuran, "kehabisan memori saat menyiapkan TLS");
        return NULL;
    }

    mbedtls_ssl_init(&t->ssl);
    mbedtls_ssl_config_init(&t->cfg);
    mbedtls_x509_crt_init(&t->ca);
    mbedtls_entropy_init(&t->entropy);
    mbedtls_ctr_drbg_init(&t->drbg);
    t->net.fd = (int)fd;

    const char* benih = "indonesiaku-tls";
    if (mbedtls_ctr_drbg_seed(&t->drbg, mbedtls_entropy_func, &t->entropy,
                              (const unsigned char*)benih, strlen(benih)) != 0) {
        snprintf(galat, galatUkuran, "gagal menyiapkan sumber acak untuk TLS");
        indkTlsTutup(t);
        return NULL;
    }

    if (!muatCa(&t->ca, galat, galatUkuran)) {
        indkTlsTutup(t);
        return NULL;
    }

    if (mbedtls_ssl_config_defaults(&t->cfg, MBEDTLS_SSL_IS_CLIENT,
                                    MBEDTLS_SSL_TRANSPORT_STREAM,
                                    MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
        snprintf(galat, galatUkuran, "gagal menyiapkan konfigurasi TLS");
        indkTlsTutup(t);
        return NULL;
    }

    // VERIFY_REQUIRED: handshake berhenti bila sertifikat tidak sah.
    // Jangan pernah menggantinya dengan VERIFY_OPTIONAL/NONE.
    mbedtls_ssl_conf_authmode(&t->cfg, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&t->cfg, &t->ca, NULL);
    mbedtls_ssl_conf_rng(&t->cfg, mbedtls_ctr_drbg_random, &t->drbg);

    if (mbedtls_ssl_setup(&t->ssl, &t->cfg) != 0) {
        snprintf(galat, galatUkuran, "gagal menyiapkan sesi TLS");
        indkTlsTutup(t);
        return NULL;
    }

    // SNI. Tanpa ini host yang berbagi IP mengirim sertifikat default, dan
    // verifikasi nama kehilangan maknanya.
    if (mbedtls_ssl_set_hostname(&t->ssl, host) != 0) {
        snprintf(galat, galatUkuran, "nama host '%s' tidak sah untuk TLS", host);
        indkTlsTutup(t);
        return NULL;
    }

    mbedtls_ssl_set_bio(&t->ssl, &t->net, mbedtls_net_send, mbedtls_net_recv, NULL);

    int rc;
    while ((rc = mbedtls_ssl_handshake(&t->ssl)) != 0) {
        if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
            continue;
        }
        if (rc == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED) {
            jelaskanVerifikasi(mbedtls_ssl_get_verify_result(&t->ssl), host,
                               galat, galatUkuran);
        } else if (rc == MBEDTLS_ERR_SSL_TIMEOUT) {
            snprintf(galat, galatUkuran, "batas waktu terlampaui saat handshake TLS ke %s", host);
        } else {
            char buf[128];
            mbedtls_strerror(rc, buf, sizeof(buf));
            snprintf(galat, galatUkuran, "handshake TLS ke %s gagal: %s", host, buf);
        }
        indkTlsTutup(t);
        return NULL;
    }

    // Jaring pengaman: handshake sukses tetapi verifikasi menyisakan temuan
    // seharusnya mustahil dengan VERIFY_REQUIRED. Diperiksa juga supaya
    // perubahan konfigurasi di masa depan tidak diam-diam melemahkannya.
    uint32_t flag = mbedtls_ssl_get_verify_result(&t->ssl);
    if (flag != 0) {
        jelaskanVerifikasi(flag, host, galat, galatUkuran);
        indkTlsTutup(t);
        return NULL;
    }

    return t;
}

int indkTlsBaca(IndkTls* tls, char* buf, size_t ukuran) {
    if (tls == NULL) return -1;
    int rc = mbedtls_ssl_read(&tls->ssl, (unsigned char*)buf, ukuran);
    if (rc == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) return 0;
    if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE) return -2;
    if (rc == MBEDTLS_ERR_SSL_TIMEOUT) return -2;
    if (rc < 0) return -1;
    return rc;
}

int indkTlsTulis(IndkTls* tls, const char* data, size_t panjang) {
    if (tls == NULL) return -1;
    size_t terkirim = 0;
    while (terkirim < panjang) {
        int rc = mbedtls_ssl_write(&tls->ssl, (const unsigned char*)data + terkirim,
                                   panjang - terkirim);
        if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
        if (rc == MBEDTLS_ERR_SSL_TIMEOUT) return -2;
        if (rc <= 0) return -1;
        terkirim += (size_t)rc;
    }
    return (int)terkirim;
}

void indkTlsTutup(IndkTls* tls) {
    if (tls == NULL) return;
    // close_notify memberi tahu lawan bicara bahwa ini akhir yang disengaja,
    // bukan koneksi putus. Gagal mengirimnya bukan alasan menolak membebaskan.
    mbedtls_ssl_close_notify(&tls->ssl);
    mbedtls_ssl_free(&tls->ssl);
    mbedtls_ssl_config_free(&tls->cfg);
    mbedtls_x509_crt_free(&tls->ca);
    mbedtls_ctr_drbg_free(&tls->drbg);
    mbedtls_entropy_free(&tls->entropy);
    free(tls);
}
