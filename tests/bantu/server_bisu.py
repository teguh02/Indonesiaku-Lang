#!/usr/bin/env python3
"""Server yang menerima koneksi TCP lalu DIAM.

Dipakai menguji batas waktu soket di driver database: tanpa batas waktu,
klien menunggu selamanya. Server yang menolak koneksi tidak bisa
membuktikannya -- penolakan langsung mengembalikan galat seketika, jadi
klien yang menggantung pun tetap terlihat "lulus". Harus diam.

Dipakai oleh tests/pg_driver.sh dan tests/my_driver.sh.
"""
import socket
import sys
import time

port = int(sys.argv[1])
diam_detik = int(sys.argv[2]) if len(sys.argv) > 2 else 30

srv = socket.socket()
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(("127.0.0.1", port))
srv.listen(1)
srv.settimeout(diam_detik)
print("siap", flush=True)

try:
    klien, _ = srv.accept()
except OSError:
    sys.exit(0)

# Koneksi diterima, lalu tidak ada sepatah byte pun yang dikirim.
time.sleep(diam_detik)
