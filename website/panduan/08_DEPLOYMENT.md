# 8. Deployment

Bab ini tentang menaruh server Indonesiaku di internet tanpa membuat lubang.

Semua konfigurasi di sini ada di repo sebagai `docker/aplikasi/` dan sudah
dijalankan sungguhan: image dibangun, stack dinyalakan, permintaan dikirim,
dan isolasinya diperiksa. Angka-angka di bawah adalah hasil pengujian itu,
bukan perkiraan.

---

## 8.1 Batas yang harus disadari lebih dulu

`app.jalankan()` melayani **satu permintaan pada satu waktu**. VM Indonesiaku
satu utas dan belum punya thread, jadi:

- permintaan yang lambat **menahan** permintaan berikutnya;
- reverse proxy **tidak** mengubah itu — ia hanya menampung koneksi di depan;
- server ini **bukan** server produksi mandiri.

Yang masuk akal: aplikasi internal, prototipe, API dengan trafik kecil, atau
layanan di belakang proxy yang sudah membatasi laju. Yang tidak: situs publik
bertrafik tinggi.

Sisanya bab ini menjelaskan cara memperkecil akibat dari batasan itu.

---

## 8.2 Risiko membuka port

Jalur termudah adalah yang paling berbahaya:

```bash
# JANGAN. Server langsung menghadap internet.
indk server.idk
```

Bawaan `app.jalankan()` mengikat `127.0.0.1`, jadi di VPS biasa server itu
belum terbuka ke luar — ini disengaja. Masalahnya muncul ketika orang
"memperbaiki" keterbatasan itu dengan `"0.0.0.0"` supaya bisa diakses, lalu
server berada di internet tanpa TLS, tanpa batas laju, dan tanpa batas ukuran
permintaan.

### Docker menembus ufw

Ini jebakan yang memakan banyak orang:

```bash
ufw default deny incoming
ufw allow 22
docker run -p 8080:80 ...     # port 8080 TETAP terbuka ke seluruh internet
```

`ufw` hanya mengatur chain `INPUT`. Docker menulis aturan iptables-nya sendiri
di chain `DOCKER` yang digantung pada `FORWARD`/`PREROUTING`, dan paket untuk
port yang dipublikasikan dialihkan **sebelum** aturan `ufw` sempat dilihat.
Ini perilaku terdokumentasi Docker, bukan bug.

Cara menutupnya: **ikat ke localhost**, lalu biarkan hanya reverse proxy yang
menghadap keluar.

```yaml
ports:
  - "127.0.0.1:8080:80"     # hanya dari host ini
```

Bedakan dengan `"8080:80"` yang berarti seluruh internet.

---

## 8.3 Dockerfile aplikasi

`docker/aplikasi/Dockerfile` — dua tahap, supaya toolchain C tidak ikut ke
image akhir:

```dockerfile
FROM alpine:3.20 AS build
RUN apk add --no-cache build-base
WORKDIR /src
COPY src ./src
COPY Makefile .
RUN make clean && make && strip indk

FROM alpine:3.20
RUN addgroup -g 10001 -S app && adduser -u 10001 -S -G app app
COPY --from=build /src/indk /usr/local/bin/indk
COPY pustaka /usr/local/share/indonesiaku/pustaka
WORKDIR /app
COPY --chown=app:app docker/aplikasi/server.idk ./server.idk
USER app
EXPOSE 8080
ENTRYPOINT ["indk", "server.idk"]
```

Empat hal yang penting dan mudah terlewat:

- **`USER app` dengan UID tetap `10001`.** Proses tidak berjalan sebagai root;
  UID yang pasti membuat izin bind mount bisa diatur dari host tanpa menebak.
- **Pustaka di `/usr/local/share/indonesiaku/pustaka`.** Interpreter di
  `/usr/local/bin` menemukannya sendiri, jadi `impor web` bekerja dari
  direktori kerja mana pun.
- **`ENTRYPOINT` bentuk exec**, bukan bentuk shell. Dengan bentuk exec `indk`
  menjadi PID 1 dan menerima `SIGTERM` dari `docker stop` langsung; dengan
  bentuk shell, shell menelan sinyalnya dan container baru mati setelah
  timeout.
- **`EXPOSE` tidak membuka apa pun.** Itu hanya dokumentasi di metadata image.

---

## 8.4 Mengikat `0.0.0.0` di dalam container

Di dalam container, bawaan `127.0.0.1` **tidak cukup**: loopback di sana
adalah loopback milik container itu sendiri, jadi reverse proxy di container
lain mendapat `Connection refused` padahal server jelas hidup dan log-nya
menulis "server siap".

```indonesiaku-takjalan
# Di dalam container, argumen kelima wajib "0.0.0.0".
app.jalankan(8080, 10, 0, benar, "0.0.0.0")
```

`soket_dengar()` juga menerima argumen kedua yang sama:

```indonesiaku-takjalan
variabel server = soket_dengar(8080, "0.0.0.0")
```

Hanya `"127.0.0.1"` dan `"0.0.0.0"` yang diterima; teks lain ditolak dengan
galat, bukan diam-diam jatuh ke loopback — salah ketik yang didiamkan membuat
server tampak hidup tapi tidak bisa dihubungi.

Yang membuat `"0.0.0.0"` tetap aman di sini adalah lapisan compose: aplikasi
tidak punya `ports:` dan berada di jaringan `internal`, jadi "semua
antarmuka" berarti jaringan Docker internal — bukan internet.

---

## 8.5 Compose: aplikasi + reverse proxy

`docker/aplikasi/docker-compose.yml`. Pembagiannya sederhana: **hanya proxy
yang menghadap keluar.**

```yaml
services:
  proxy:
    image: nginx:1.27-alpine
    ports:
      - "127.0.0.1:8080:80"      # hanya localhost
    volumes:
      - ./nginx.conf:/etc/nginx/nginx.conf:ro
    networks: [publik, internal]
    read_only: true
    tmpfs:
      - /var/cache/nginx:size=32m
      - /var/run:size=1m
    cap_drop: [ALL]
    cap_add: [CHOWN, SETUID, SETGID, DAC_OVERRIDE, NET_BIND_SERVICE]
    security_opt: [no-new-privileges:true]
    pids_limit: 128
    mem_limit: 128m
    cpus: 0.50

  app:
    build:
      context: ../..
      dockerfile: docker/aplikasi/Dockerfile
    # TIDAK ADA ports: sama sekali
    networks: [internal]
    user: "10001:10001"
    read_only: true
    tmpfs: [/tmp:size=16m]
    cap_drop: [ALL]
    security_opt: [no-new-privileges:true]
    pids_limit: 64
    mem_limit: 128m
    cpus: 0.50

networks:
  publik:
    driver: bridge
  internal:
    driver: bridge
    internal: true     # tanpa gateway → tidak bisa keluar internet
```

Jalankan:

```bash
docker compose -f docker/aplikasi/docker-compose.yml up -d --build
curl http://127.0.0.1:8080/
```

### Apa arti tiap pembatas

| Pengaturan | Akibatnya |
|---|---|
| `read_only: true` | Rootfs tidak bisa ditulis. Penyerang tidak bisa menaruh berkas. |
| `tmpfs` | Satu-satunya tempat tulis, di memori, hilang saat restart. |
| `cap_drop: ALL` | Semua privilege Linux dilepas. |
| `no-new-privileges` | Proses tidak bisa menaikkan privilege lewat setuid. |
| `user: "10001:10001"` | Bukan root di dalam container. |
| `pids_limit` | Fork bomb tidak menjatuhkan host. |
| `mem_limit`, `cpus` | Satu container tidak menghabiskan mesin. |
| `internal: true` | Aplikasi tidak bisa menghubungi internet keluar. |

Isolasi ini diperiksa, bukan diasumsikan:

```
app dari host        : tidak terjangkau
app ke internet      : wget: bad address 'example.com'
user di container    : 10001
tulis ke /app        : Read-only file system
```

---

## 8.6 Reverse proxy

`docker/aplikasi/nginx.conf` — proxy menyelesaikan yang tidak dilakukan
aplikasi:

```nginx
client_max_body_size 1m;
limit_req_zone $binary_remote_addr zone=perip:10m rate=10r/s;
client_body_timeout 10s;
client_header_timeout 10s;
server_tokens off;

upstream aplikasi {
    server app:8080;
    keepalive 16;
}

server {
    listen 80;
    location / {
        limit_req zone=perip burst=20 nodelay;
        proxy_pass http://aplikasi;
        proxy_http_version 1.1;
        proxy_set_header Connection "";
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        proxy_connect_timeout 3s;
        proxy_read_timeout 15s;
    }
}
```

**Batas laju bukan hiasan di sini.** Karena aplikasi melayani satu permintaan
pada satu waktu, satu klien yang membanjiri membuat semua yang lain menunggu.
Dengan konfigurasi di atas, 40 permintaan beruntun menghasilkan 24 × `200` dan
16 × `503` — yang berlebih ditolak proxy sebelum menyentuh aplikasi.

Batas ukuran juga ditegakkan sebelum aplikasi membaca sebita pun: unggahan 2 MB
dijawab `413`.

---

## 8.7 TLS

Stack bawaan jalan di HTTP karena sertifikat butuh domain. Untuk produksi,
ubah blok server menjadi:

```nginx
server {
    listen 443 ssl;
    http2 on;
    server_name contoh.id;

    ssl_certificate     /etc/nginx/tls/fullchain.pem;
    ssl_certificate_key /etc/nginx/tls/privkey.pem;
    ssl_protocols       TLSv1.2 TLSv1.3;
    ssl_prefer_server_ciphers off;
    ssl_session_cache   shared:SSL:10m;
    ssl_session_timeout 1d;

    add_header Strict-Transport-Security "max-age=31536000" always;

    location / {
        proxy_pass http://aplikasi;
        # ... header proxy seperti di atas
    }
}

# HTTP hanya untuk mengalihkan.
server {
    listen 80;
    server_name contoh.id;
    return 301 https://$host$request_uri;
}
```

Sertifikat gratis dengan Let's Encrypt:

```bash
docker run --rm -it \
  -v "$PWD/tls:/etc/letsencrypt" \
  -p 80:80 \
  certbot/certbot certonly --standalone -d contoh.id
```

Lalu pasang `./tls/live/contoh.id` sebagai volume ke `/etc/nginx/tls` dan
jadwalkan `certbot renew` — sertifikat Let's Encrypt berlaku 90 hari.

> **TLS berhenti di proxy.** Lalu lintas proxy → aplikasi tetap HTTP di dalam
> jaringan Docker internal. Itu memadai selama keduanya di host yang sama;
> bila berbeda host, jalur itu harus diamankan sendiri.

---

## 8.8 Daftar periksa sebelum publik

- [ ] Port diikat `127.0.0.1:...`, bukan `0.0.0.0` — ingat ufw ditembus Docker
- [ ] TLS aktif, HTTP dialihkan ke HTTPS
- [ ] `read_only`, `cap_drop: ALL`, `no-new-privileges`, `user:` non-root
- [ ] `mem_limit`, `cpus`, `pids_limit` terpasang
- [ ] Aplikasi di jaringan `internal`, tanpa `ports:`
- [ ] Batas laju dan `client_max_body_size` di proxy
- [ ] `server_tokens off`
- [ ] Log aplikasi tidak memuat badan atau header (bawaan `app.jalankan()`)
- [ ] Rencana pembaruan sertifikat
- [ ] Disadari: satu permintaan pada satu waktu

---

Bab sebelumnya: [7. Referensi Pustaka Standar](07_PUSTAKA_STANDAR.md)
