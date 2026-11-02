# Image untuk menjalankan SERVER yang ditulis dengan Indonesiaku.
#
# Berbeda dari docker/images/Dockerfile yang menyediakan interpreter untuk
# menjalankan skrip sekali jalan: image ini menjalankan satu aplikasi server
# terus-menerus.
#
# Build dari akar repo:
#     docker build -f docker/aplikasi/Dockerfile -t indonesiaku-aplikasi .

FROM alpine:3.20 AS build

RUN apk add --no-cache build-base

WORKDIR /src
COPY src ./src
COPY Makefile .
RUN make clean && make && strip indk

# ---- Runtime ----
FROM alpine:3.20

# Pengguna non-root dengan UID yang PASTI, bukan sekadar "nobody": UID tetap
# membuat izin bind mount bisa diatur dari host tanpa menebak.
RUN addgroup -g 10001 -S app && adduser -u 10001 -S -G app app

COPY --from=build /src/indk /usr/local/bin/indk

# Pustaka standar di lokasi PREFIX/share: interpreter di /usr/local/bin
# menemukannya sendiri, sehingga `impor web` bekerja dari direktori kerja mana
# pun.
COPY pustaka /usr/local/share/indonesiaku/pustaka

WORKDIR /app
COPY --chown=app:app docker/aplikasi/server.idk ./server.idk

USER app

# Hanya sebagai dokumentasi; EXPOSE tidak membuka port apa pun ke host.
EXPOSE 8080

# exec form (bukan shell form) supaya indk menjadi PID 1 dan menerima SIGTERM
# langsung dari `docker stop`, tanpa shell di tengah yang menelannya.
ENTRYPOINT ["indk", "server.idk"]
