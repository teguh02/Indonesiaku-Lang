#!/usr/bin/env python3
"""Server PostgreSQL palsu untuk menguji penolakan SCRAM di sisi KLIEN.

Server PostgreSQL sungguhan jujur, jadi ia tidak bisa membuktikan bahwa klien
benar-benar memeriksa nonce dan tanda tangan server: melewatkan kedua cek itu
tetap menghasilkan koneksi yang sukses. Yang membuktikannya adalah server yang
curang.

Jabat tangan diselesaikan SAMPAI TUNTAS (termasuk AuthenticationOk dan
ReadyForQuery) supaya kegagalan yang terlihat memang berasal dari cek klien,
bukan dari koneksi yang kebetulan terputus lebih dulu. Itu perbedaan yang
sempat menipu saya saat menulis tes ini.

mode nonce : nonce server tidak mengawali nonce klien (replay tantangan lama)
mode tanda : tanda tangan server palsu (server tidak benar-benar tahu sandi)
"""
import base64
import hashlib
import hmac
import socket
import sys
import time

SANDI = b"x"
GARAM = b"garamgaram"
ITERASI = 4096


def kirim(c, tipe, isi):
    c.sendall(tipe + (len(isi) + 4).to_bytes(4, "big") + isi)


def layani(port, mode):
    srv = socket.socket()
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("127.0.0.1", port))
    srv.listen(1)
    srv.settimeout(20)
    print("siap", flush=True)
    c, _ = srv.accept()
    try:
        c.recv(65536)  # StartupMessage
        kirim(c, b"R", b"\x00\x00\x00\x0a" + b"SCRAM-SHA-256\x00\x00")

        data = c.recv(65536)
        awal = data[data.index(b"n,,"):].decode(errors="replace")
        bare = awal[3:]
        nonce_klien = bare.split("r=")[1]

        nonce_server = ("XXXXTIDAKCOCOKXXXX" if mode == "nonce"
                        else nonce_klien + "srv")
        tantangan = (f"r={nonce_server},"
                     f"s={base64.b64encode(GARAM).decode()},i={ITERASI}")
        kirim(c, b"R", b"\x00\x00\x00\x0b" + tantangan.encode())

        c.recv(65536)  # ClientFinal

        akhir_tanpa_bukti = f"c=biws,r={nonce_server}"
        auth = f"{bare},{tantangan},{akhir_tanpa_bukti}"
        asin = hashlib.pbkdf2_hmac("sha256", SANDI, GARAM, ITERASI, 32)
        if mode == "tanda":
            v = base64.b64encode(b"T" * 32).decode()
        else:
            kunci_server = hmac.new(asin, b"Server Key", hashlib.sha256).digest()
            v = base64.b64encode(
                hmac.new(kunci_server, auth.encode(), hashlib.sha256).digest()
            ).decode()
        kirim(c, b"R", b"\x00\x00\x00\x0c" + f"v={v}".encode())
        kirim(c, b"R", b"\x00\x00\x00\x00")       # AuthenticationOk
        kirim(c, b"Z", b"I")                      # ReadyForQuery
        time.sleep(1.0)
    except Exception:
        pass
    finally:
        c.close()
        srv.close()


if __name__ == "__main__":
    layani(int(sys.argv[1]), sys.argv[2])
