# ===============================
# Image: situs dokumentasi + playground Indonesiaku
#
# Dasarnya adalah image rilis resmi, jadi interpreter yang dipakai di sini
# BENAR-BENAR biner yang sama dengan yang diuji dan dirilis — tidak
# dikompilasi ulang. Image rilis sudah menyediakan:
#   /usr/local/bin/indk        → dipakai server.py (INDK_BIN bawaan)
#   /opt/indonesiaku/pustaka   → dipakai server.py (PUSTAKA_DIR bawaan);
#                                symlink ke /usr/local/share/indonesiaku/pustaka
#
# Satu image, dua peran (lihat docker-compose.yml):
#   web    → nginx menyajikan website/ dan mem-proxy /api/ ke runner
#   runner → python menjalankan indk di sandbox
#
# Naikkan tag dasar bersamaan dengan INDK_VERSION saat rilis baru.
# ===============================
FROM teguh02/indonesiaku:1.7.0

# Image rilis tidak membawa nginx/python3 (sengaja ramping).
RUN apk add --no-cache nginx python3 \
 && adduser -D -H -u 10001 runner

LABEL maintainer="Teguh Rijanandi <teguhrijanandi02@gmail.com>"
LABEL description="Dokumentasi + playground bahasa pemrograman Indonesiaku"
LABEL version="1.7.0"

# Pustaka standar tidak boleh ditulisi oleh kode pengunjung.
# Isinya ada di /usr/local/share/indonesiaku/pustaka (/opt/... hanya symlink).
RUN chmod -R a-w /usr/local/share/indonesiaku/pustaka

# Situs statis (dokumentasi, panduan, playground).
COPY website /usr/share/nginx/html

# Layanan playground.
COPY docker/playground/server.py /opt/playground/server.py
COPY docker/playground/nginx.conf /etc/nginx/nginx.conf

RUN mkdir -p /var/cache/nginx /var/run /tmp \
 && chown -R runner:runner /tmp

# Image dasar memakai ENTRYPOINT ["indk"]; di sini peran containernya berbeda
# (nginx / python), jadi entrypoint itu harus dikosongkan.
ENTRYPOINT []

EXPOSE 80
# -e /dev/stderr: sebelum membaca konfigurasi, nginx membuka error log bawaan
# di /var/lib/nginx/logs/ yang read-only di container ini.
CMD ["nginx", "-e", "/dev/stderr", "-g", "daemon off;"]
