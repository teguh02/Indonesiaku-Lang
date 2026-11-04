# Lampiran — Uji ekstensif Windows, Indonesiaku 1.4.0

Hasil lengkap uji multi-agen pada 2026-11-03 terhadap `indk.exe` dari installer
`Indonesiaku-Setup-1.4.0.exe` (Windows 11, AMD64). Ringkasan dan cara membaca
ada di [`../WINDOWS.md`](../WINDOWS.md) bagian 7.

Metode: delapan agen penguji, masing-masing satu area, menulis dan menjalankan
program `.idk` lalu membandingkan stdout, stderr, dan kode keluar dengan
panduan v1.4.0 (`website/panduan/03`, `07`) dan perilaku Python 3.14. Setiap
temuan lalu diulang secara independen oleh agen verifikator yang berusaha
membantahnya. Hanya yang **terkonfirmasi** dicantumkan di bawah. Temuan yang
sama bisa muncul di lebih dari satu area karena area saling beririsan.
Kebanyakan temuan **tidak khas Windows** (bug bahasa yang kemungkinan sama di
macOS/Linux); yang khas Windows ditandai.

Total temuan terkonfirmasi (dengan duplikat antar-area): **140**.

## Sintaks, angka, teks, operator

Pemeriksaan: **252/292 lulus**; temuan terkonfirmasi: **23**.

| Sub-area | Lulus |
|---|---|
| Literal angka (bulat/desimal, 0x/0b/0o, _, eksponen, batas int64) | 36/42 |
| Aritmetika // % ** dengan negatif, desimal, inf/nan vs Python | 35/40 |
| Luapan bulat -> GalatLuapan, bagi nol, geser | 21/21 |
| Bitwise dan prioritas operator | 10/10 |
| Perbandingan, rantai perbandingan, adalah/==, kesetaraan struktural | 24/30 |
| Truthiness / ke_logika | 13/14 |
| Teks: indeks, indeks negatif, irisan + langkah, Unicode | 26/30 |
| Escape, string tiga kutip, repr teks dalam koleksi | 13/14 |
| f-string | 16/18 |
| Metode/fungsi teks (huruf_besar, ganti, potong, format, *, dalam) | 20/24 |
| Konversi (ke_angka, ke_teks, ke_bulat, ke_desimal, bulat, bulatkan, lantai, atap, biner/oktal/heksa) | 34/40 |
| Sintaks: komentar, rata kiri, indentasi campur, newline dalam () [] {}, ;, CRLF, berkas kosong, ekspresi kondisional | 26/30 |
| Stres/kompilator (nesting dalam, baris panjang, batas konstanta) | 12/19 |

### SINTAKS-01 — [tinggi] `nama = a jika c selain b` tanpa `variabel` dikompilasi sebagai `(nama = a) jika c selain b`

Reproduced. Docs (Ekspresi Kondisional) say this is the same as Python's a if c else b. Plain assignment is the most common form, and when the condition is false the assignment is silently skipped: at top level the variable is undefined (exit 70), an existing variable keeps its old value, and inside a function it is kosong. The cause is in the compiler (precedence of assignment vs. the conditional), so it is OS-independent.

Reproduksi:

```text
x = 1 jika salah selain 2
cetak("A", x)
```

Keluaran 1.4.0:

```text
Variabel tidak terdefinisi 'x'. exit 70. A variable already set to 99 stays 99. Inside a function, `y = 1 jika salah selain 2; kembalikan y` -> kosong
```

### SINTAKS-02 — [sedang] `==` pada struktur bersarang dalam (> ~64 tingkat) diam-diam menghasilkan salah

Reproduced: two identical linked lists, depth 60 -> benar, depth 100 -> salah, and != -> benar. AGENTS documents the EQUAL_MAX_DEPTH cap as a guard against cycles, but when the cap is hit the result is a wrong answer instead of an error (Python raises RecursionError). Severity lowered: such deep nesting is rare.

Reproduksi:

```text
fungsi buat(n) {
    h = []
    untuk i dalam rentang(n) { h = [i, h] }
    kembalikan h
}
cetak(buat(60) == buat(60), buat(100) == buat(100), buat(100) != buat(100))
a = [1]
a.tambah(a)
b = [1]
b.tambah(b)
cetak(a == b)
```

Keluaran 1.4.0:

```text
benar salah benar
salah
```

### SINTAKS-03 — [sedang] lantai/atap/bulat(nan) -> -9223372036854775808; bulatkan(nan/inf) mengembalikan desimal

Reproduced. Docs 1.1 say lantai/atap return bulat and bulatkan without digit returns bulat. NaN is silently turned into INT64_MIN (a converted-NaN artefact, as from a C cast) and bulatkan(inf) returns takhingga, while ke_bulat(nan) and lantai(inf) correctly raise. This is a silent wrong value; the inputs are rare.

Reproduksi:

```text
n = ke_angka("nan")
cetak(lantai(n), atap(n), bulat(n))
cetak(bulatkan(n))
cetak(bulatkan(ke_angka("inf")))
```

Keluaran 1.4.0:

```text
-9223372036854775808 -9223372036854775808 -9223372036854775808
nan
takhingga
```

### SINTAKS-04 — [sedang] Crash stack overflow di compiler pada ekspresi bersarang sangat dalam

Reproduced: 3000 nested parentheses -> exit -1073741571 (STATUS_STACK_OVERFLOW) with empty stderr; 2000 still prints 1. The root cause is that the recursive Pratt parser has no nesting limit, which is OS-independent. Only the threshold is lower on Windows (1 MB stack vs 8 MB on Linux), so the bug itself is not Windows-specific. Severity lowered to sedang: it needs pathological input, though it is still a silent crash.

Reproduksi:

```text
(generated) cetak( + '(' * 3000 + 1 + ')' * 3000 + )
```

Keluaran 1.4.0:

```text
exit -1073741571, stdout/stderr empty (2000 levels -> '1', exit 0)
```

### SINTAKS-05 — [sedang] Indeks desimal dipotong diam-diam: "halo"[1.5] -> "a", [10,20,30][1.5] -> 20

Reproduced. Python raises TypeError for a float index, and the docs reject fractions for repetition. A non-integer index silently picking an element is a semantic bug that hides calculation errors.

Reproduksi:

```text
cetak("halo"[1.5], [10,20,30][1.5])
a = [10,20,30]
cetak(a[0.5:2.5])
```

Keluaran 1.4.0:

```text
a 20
[10, 20]  (exit 0)
```

### SINTAKS-06 — [sedang] Indeks di luar jangkauan pada teks/rentang menimbulkan Galat biasa, bukan GalatIndeks

Reproduced: `kecuali GalatIndeks` catches daftar[5] but not "ha"[5] or rentang(2)[5]; those fall through to the generic kecuali. GalatIndeks is documented as a built-in class, and in Python this is IndexError.

Reproduksi:

```text
coba { x = "ha"[5] } kecuali GalatIndeks sebagai e { cetak("teks GI") } kecuali e { cetak("teks lain", e) }
coba { x = rentang(2)[5] } kecuali GalatIndeks sebagai e { cetak("rentang GI") } kecuali e { cetak("rentang lain", e) }
coba { x = [1][5] } kecuali GalatIndeks sebagai e { cetak("daftar GI") } kecuali e { cetak("daftar lain", e) }
```

Keluaran 1.4.0:

```text
teks lain Indeks string di luar jangkauan (indeks 5, panjang 2).
rentang lain Indeks rentang di luar jangkauan (indeks 5, panjang 2).
daftar GI
```

### SINTAKS-07 — [sedang] Ekspresi kondisional berantai `a jika c selain b jika d selain e` galat kompilasi

Reproduced. The docs say it behaves like Python's a if c else b, and in Python it is right-associative and chainable. The selain operand is evidently parsed at too high a precedence.

Reproduksi:

```text
cetak(1 jika salah selain 2 jika benar selain 3)
```

Keluaran 1.4.0:

```text
[baris 1, kolom 29] Kesalahan di 'jika': Harapkan ')' setelah argumen. exit 65
```

### SINTAKS-08 — [rendah] f-string hanya untuk kutip ganda satu baris; f'..' dan f"""..""" gagal dengan pesan menyesatkan

Reproduced. The f-string section only shows f"...", so f'..' is not explicitly promised, but single-quoted strings are valid and Python accepts both forms. The error message ("Harapkan ')'") is misleading, and for the triple-quoted form it points at the last line of the token. Severity lowered because the docs do not promise these forms.

Reproduksi:

```text
nama = "Budi"
cetak(f'Halo {nama}')
```

Keluaran 1.4.0:

```text
[baris 2, kolom 8] Kesalahan di ''Halo {nama}'': Harapkan ')' setelah argumen. exit 65
```

### SINTAKS-09 — [rendah] bulatkan(x, 0) mengembalikan bulat padahal dokumentasi: dengan digit hasilnya desimal

Reproduced. The docs say 'Tanpa digit hasilnya bulat; dengan digit hasilnya desimal', and Python's round(2.5, 0) is 2.0. The implementation evidently cannot tell an explicit 0 from the default digit=0.

Reproduksi:

```text
cetak(bulatkan(2.5, 0), bulatkan(3.7, 0), bulatkan(2.567, 1))
```

Keluaran 1.4.0:

```text
2 4 2.6
```

### SINTAKS-10 — [rendah] Perbandingan urutan daftar/tupel tidak didukung; pesan 'Operand harus berupa angka.' salah

Reproduced. Python compares sequences lexicographically. Even if this is a missing feature rather than a bug, the error message is wrong because strings are also valid operands for <.

Reproduksi:

```text
coba { cetak([1,2] < [1,3]) } kecuali e { cetak("E1", e) }
coba { cetak(1 < "a") } kecuali e { cetak("E2", e) }
```

Keluaran 1.4.0:

```text
E1 Operand harus berupa angka.
E2 Operand harus berupa angka.
```

### SINTAKS-11 — [rendah] format() dengan argumen lebih sedikit dari {} diam-diam membiarkan '{}'

Reproduced. Python raises IndexError here. The docs do not say what should happen; silently leaving the placeholder hides the mistake.

Reproduksi:

```text
cetak(format("{} dan {}", 1))
```

Keluaran 1.4.0:

```text
1 dan {}  (exit 0)
```

### SINTAKS-12 — [rendah] huruf_besar/huruf_kecil hanya mengubah huruf ASCII

Reproduced (é, É, ß unchanged). The docs only promise Unicode awareness for character counting, not case mapping, so this is a deviation from Python rather than a broken documented promise.

Reproduksi:

```text
cetak(huruf_besar("éa"), huruf_kecil("ÉA"), huruf_besar("ß"))
```

Keluaran 1.4.0:

```text
éA Éa ß
```

### SINTAKS-13 — [rendah] Waktu kompilasi kuadratik terhadap panjang satu baris sumber

Reproduced: a one-line function with 20000 terms takes 2.05 s, with 40000 terms 10.6 s, so doubling the input costs about 5x. It only shows up with extremely long lines.

Reproduksi:

```text
(generated) fungsi f(a) {\n    kembalikan a + a + ... (40000 terms on one line)\n}\ncetak("ok")
```

Keluaran 1.4.0:

```text
ok; 20000 terms 2.05 s, 40000 terms 10.56 s
```

### SINTAKS-14 — [rendah] Kurawal pembuka di baris berikutnya ditolak dan galat melaporkan nomor baris salah

The rejection is consistent with design B16 (a newline ends the statement), so that part is a documentation gap, not a behaviour bug. The error location is wrong, though: it says baris 2 while the snippet shows line 1 ('jika benar'), and it cascades into two 'Harapkan }' errors.

Reproduksi:

```text
jika benar
{
    cetak("a")
}
```

Keluaran 1.4.0:

```text
[baris 2, kolom 11] Kesalahan di akhir baris: Harapkan '{' setelah kondisi 'jika'.
    jika benar
(+ 2x Harapkan '}' setelah blok) exit 65
```

### SINTAKS-15 — [rendah] Nol negatif kehilangan tanda: -0.0 dicetak 0.0

Reproduced. Python's repr prints -0.0, and the docs promise a shortest round-trip repr; 0.0 does not round-trip the sign.

Reproduksi:

```text
cetak(-0.0, 0.0 * -1, [-0.0])
```

Keluaran 1.4.0:

```text
0.0 0.0 [0.0]
```

### SINTAKS-16 — [rendah] Plus unary tidak didukung

Reproduced. Python supports +x. The docs do not list unary plus, so this is a missing feature with low impact.

Reproduksi:

```text
x = 5
cetak(+x)
```

Keluaran 1.4.0:

```text
[baris 2, kolom 7] Kesalahan di '+': Harapkan ekspresi. exit 65
```

### SINTAKS-17 — [rendah] Lexing literal angka menyimpang (010, 1_e3, 1.e5, -9223372036854775808)

Reproduced. 1_e3 is accepted, which contradicts the docs ('garis bawah hanya di antara digit'). 010 is accepted, where Python gives a SyntaxError. 1.e5 gives a confusing property error at runtime. -9223372036854775808 is rejected as a literal because the minus sign is unary; that one is a natural consequence of the design, but the docs list the range as including that value.

Reproduksi:

```text
cetak(010, 007, 1_e3)
cetak(1.e5)
```

Keluaran 1.4.0:

```text
10 7 1000.0 ; cetak(1.e5) -> 'Hanya objek yang memiliki properti.' exit 70 ; cetak(-9223372036854775808) -> 'Angka bulat melebihi jangkauan' exit 65
```

### SINTAKS-18 — [rendah] repr/tampilan koleksi mencetak karakter kontrol mentah

Confirmed with od -c: raw bytes 001, 033 and 177. The docs (Teks di Dalam Koleksi) say 'Karakter khusus ikut di-escape, jadi isinya tidak menyesatkan', but only \n, \t, " and similar are escaped. A raw ESC can emit terminal escape sequences.

Reproduksi:

```text
cetak(repr(chr(1)), [chr(27)], repr(chr(127)))
```

Keluaran 1.4.0:

```text
bytes: " 001 "  [ " 033 " ]  " 177 "
```

### SINTAKS-19 — [rendah] ganti() dengan pencarian kosong mengembalikan teks tak berubah

Reproduced. This deviates from Python's str.replace semantics, and the docs do not cover the edge case.

Reproduksi:

```text
cetak(ganti("abc", "", "-"))
```

Keluaran 1.4.0:

```text
abc
```

### SINTAKS-20 — [rendah] ke_angka menolak spasi di sekitar angka

Reproduced. Python's int()/float() strip surrounding whitespace, and this case is common when reading input().

Reproduksi:

```text
cetak(ke_angka(" 42 "))
```

Keluaran 1.4.0:

```text
ke_angka() gagal: ' 42 ' bukan angka valid, exit 70
```

### SINTAKS-21 — [rendah] logika tidak numerik secara tidak konsisten (1 == benar salah, benar + benar galat, benar & 3 == 1)

Reproduced. Bitwise operators treat logika as 0/1 (documented), but == and + do not. Treating logika as non-numeric may be a deliberate design choice, but the inconsistency is undocumented and differs from Python.

Reproduksi:

```text
cetak(1 == benar, benar & 3)
cetak(benar + benar)
```

Keluaran 1.4.0:

```text
salah 1
Operand '+' harus dua angka, dua teks, dua daftar, atau dua tupel. exit 70
```

### SINTAKS-22 — [rendah] Kasus tepi float berbeda dari Python (inf//1, 2.0**10000, (-8)**0.5, "ab"*2.0)

Reproduced. These are minor deviations. "ab" * 2.0 is accepted because the multiplier is a whole number, so it only weakly contradicts 'Pengali harus bilangan bulat'; Python raises TypeError for it.

Reproduksi:

```text
inf = ke_angka("inf")
cetak(inf // 1, 2.0 ** 10000, (-8) ** 0.5, "ab" * 2.0)
```

Keluaran 1.4.0:

```text
takhingga takhingga nan abab
```

### SINTAKS-23 — [rendah] Catatan dok 'akhir baris belum mengakhiri pernyataan' di Operator Logika usang/kontradiktif

Documentation bug. Line ~403 of 03_PANDUAN_SINTAKS.md says a line starting with ( or - joins the previous line, but the program shows that a newline ends the statement (B16), as the bitwise-section note says.

Reproduksi:

```text
x = 5
(1)
cetak("x", x)
y = benar
-1
cetak(y)
```

Keluaran 1.4.0:

```text
x 5
benar
```


## Percabangan, perulangan, fungsi

Pemeriksaan: **239/262 lulus**; temuan terkonfirmasi: **14**; ditolak verifikator: 1.

| Sub-area | Lulus |
|---|---|
| jika/jika_lain/selain, truthiness, ekspresi kondisional, and/or | 40/42 |
| cocokkan/kasus (literal, _, binding, Kelas(), atau, guard, nested, break/continue) | 30/30 |
| selagi/untuk over rentang/daftar/teks/kamus/tupel/himpunan, nested hentikan/lanjut, inside coba/akhirnya | 31/36 |
| fungsi: defaults K15, keyword args, arg-count errors, closures, nonlokal/global, K3 | 41/46 |
| rekursi depth limit (FRAMES_MAX) and nesting stress | 20/26 |
| anonim, dekorator | 20/20 |
| generator (hasilkan), generator expression, list/dict/set comprehension | 30/34 |
| unpacking, buang | 27/28 |

### ALUR-FUNGSI-01 — [tinggi] Comprehension silently ignores tokens after the first filter: a second jika, selain, junk tokens, or a second untuk

Reproduced. The guide (03 'List comprehension') says only one untuk and one optional jika are allowed, so anything more should be a compile error. Instead the parser silently drops the trailing tokens and the program exits 0 with a wrong result. In my run a second untuk whose variable is never used also gives [1] silently; it only fails at runtime when y is referenced. The fault is in the parser (scanComprehension/listComprehension), so it is OS-independent.

Reproduksi:

```text
cetak([x untuk x dalam rentang(10) jika x > 2 jika x < 5])
cetak([x untuk x dalam [1, 2] selain 5])
cetak([x untuk x dalam [0, 1] jika x lalu])
cetak([x untuk x dalam [1] untuk y dalam [2]])
```

Keluaran 1.4.0:

```text
[3, 4, 5, 6, 7, 8, 9] / [1, 2] / [1] / [1], each run separately with exit 0 and no compile error
```

### ALUR-FUNGSI-02 — [tinggi] untuk loop variable is block-scoped: an existing variable with the same name is left unchanged and the name is undefined after the loop

Reproduced at script level and inside a function. The guide says Indonesiaku copies Python's scoping behaviour, and its scoped-loop-variable note applies only to comprehensions. In Python a for loop rebinds the enclosing name. The shadowing case silently gives the old value, a wrong result with no error. This comes from the compiler's scoping, so it is OS-independent.

Reproduksi:

```text
k = 99
untuk k dalam rentang(3) { }
cetak(k)
fungsi f() {
    j = 7
    untuk j dalam rentang(3) { }
    kembalikan j
}
cetak(f())
untuk q dalam rentang(3) { }
cetak(q)
```

Keluaran 1.4.0:

```text
99
7
Variabel tidak terdefinisi 'q'.  exit 70   (Python: 2, 2, 2)
```

### ALUR-FUNGSI-03 — [tinggi · khas Windows] Compiler stack overflow (0xC00000FD) on 32 nested fungsi or about 2500 nested parentheses

Reproduced: 31 nested fungsi works, 32 crashes with exit -1073741571 (STATUS_STACK_OVERFLOW) and no output. 1500 nested parentheses work and 2500 crash. Each nested Compiler struct sits on the C stack and holds several [UINT8_COUNT] arrays (locals, upvalues, globals, nonlocals, finallies), roughly 30 KB each, which exhausts Windows' 1 MB main-thread stack. The unbounded recursion is a bug on every OS, but Linux's 8 MB stack moves the threshold to roughly 250 levels, so the low limit is Windows-specific. It should fail cleanly with exit 65.

Reproduksi:

```text
(32 lines 'fungsi gN() {' for N=0..31, then 32 lines '}', then:)
cetak("ok")
```

Keluaran 1.4.0:

```text
no output, exit code -1073741571 (0xC00000FD); the same file with 31 levels prints ok
```

### ALUR-FUNGSI-04 — [sedang] Mutating a list during untuk: appended items are never visited, and shrinking the list raises an index error

Reproduced. untuk reads the length once at loop start (OP_LIST_LEN), unlike Python's list iterator, which re-checks the length on every step. Growing the list silently skips the new items. Shrinking it raises 'Indeks list di luar jangkauan' with exit 70, where Python finishes the loop. The guide promises Python behaviour in general, but this is an edge case that Python also discourages, so I lowered it to sedang. It follows from the VM's untuk desugaring, so it is OS-independent.

Reproduksi:

```text
a = [1, 2, 3]
n = 0
untuk x dalam a {
    n += 1
    jika x < 3 { tambah(a, x + 10) }
}
cetak(a, n)
b = [1, 2, 3, 4]
untuk x dalam b {
    jika x == 2 { buang b[0] }
}
cetak(b)
```

Keluaran 1.4.0:

```text
[1, 2, 3, 11, 12] 3   (Python: 5)
Indeks list di luar jangkauan (indeks 3, panjang 3).  exit 70   (Python: [2, 3, 4])
```

### ALUR-FUNGSI-05 — [sedang] Only 64 active coba handlers in total (TRY_MAX): recursive generators and recursion with a coba per frame fail at depth 64

Reproduced. A recursive generator at depth 100 fails with 'Terlalu banyak blok 'coba' bersarang' even though the program contains no coba, which also makes the message misleading. Recursion with a coba per frame is worse: the user's own kecuali catches the limit error, so r(70) silently returns 62 instead of 70. FRAMES_MAX is 1000 and TRY_MAX 64 is not documented. Both are fixed constants, so this is OS-independent.

Reproduksi:

```text
fungsi pohon(n) {
    jika n == 0 { hasilkan 0 } selain {
        untuk v dalam pohon(n - 1) { hasilkan v }
        hasilkan n
    }
}
cetak(panjang(daftar(pohon(100))))
```

Keluaran 1.4.0:

```text
Terlalu banyak blok 'coba' bersarang.  exit 70. Also: fungsi r(n) with coba/kecuali per frame gives r(70) -> 62 silently
```

### ALUR-FUNGSI-06 — [sedang] Chained conditional expression 'a jika c selain b jika d selain e' is rejected

Reproduced. The guide's 'Ekspresi Kondisional' section says it works 'sama seperti a if c else b di Python', and Python's conditional expression is right-associative. The else branch is apparently parsed at a precedence that excludes another conditional expression. It works when the else branch is wrapped in parentheses. This is a parser issue, so OS-independent.

Reproduksi:

```text
cetak("a" jika salah selain "b" jika salah selain "c")
```

Keluaran 1.4.0:

```text
[baris 1, kolom 33] Kesalahan di 'jika': Harapkan ')' setelah argumen.  exit 65 (Python: c)
```

### ALUR-FUNGSI-07 — [sedang] Reading a function-local name before its assignment silently gives kosong

Reproduced. Under K3, documented in 'Lingkup Assignment', g is a local because the function assigns to it. Python raises UnboundLocalError here. Indonesiaku returns kosong silently because prescanFunctionBody pre-fills the slot with OP_KOSONG. The language already has a 'belum diisi' check for names removed with buang, but it does not apply here. OS-independent.

Reproduksi:

```text
g = 10
fungsi f() {
    y = g
    g = 5
    kembalikan y
}
cetak(f())
```

Keluaran 1.4.0:

```text
kosong  exit 0
```

### ALUR-FUNGSI-08 — [sedang] Too many positional arguments raises an untyped error, not GalatTipe

Reproduced. A missing argument and an unknown keyword both raise GalatTipe, but f(1, 2) and panjang() raise plain string errors that kecuali GalatTipe does not catch. That is inconsistent with the other call errors and with Python's TypeError. OS-independent.

Reproduksi:

```text
fungsi f(a) { kembalikan a }
coba { f(1, 2) } kecuali GalatTipe sebagai e { cetak("GalatTipe", e.pesan) } kecuali e { cetak("bukan", e) }
coba { f() } kecuali GalatTipe sebagai e { cetak("GalatTipe", e.pesan) } kecuali e { cetak("bukan", e) }
coba { panjang() } kecuali GalatTipe sebagai e { cetak("GalatTipe", e.pesan) } kecuali e { cetak("bukan", e) }
```

Keluaran 1.4.0:

```text
bukan Harapkan paling banyak 1 argumen tetapi mendapat 2.
GalatTipe Parameter wajib 'a' tidak diisi.
bukan panjang() memerlukan 1 argumen
```

### ALUR-FUNGSI-09 — [sedang] Reading a name removed with buang, or any undefined global, is not GalatNama

Reproduced. The guide's 'Menghapus dengan buang' section says reading the name again raises GalatNama, but kecuali GalatNama does not catch it, for a removed name or for any other undefined name. This directly contradicts the guide. OS-independent.

Reproduksi:

```text
x = 5
buang x
coba { cetak(x) } kecuali GalatNama sebagai e { cetak("GalatNama", e.pesan) } kecuali e { cetak("bukan", e) }
```

Keluaran 1.4.0:

```text
bukan Variabel tidak terdefinisi 'x'.
```

### ALUR-FUNGSI-10 — [sedang] Filling a parameter twice (f(a=1, a=2) or f(1, a=2)) gives a misleading argument-count error that is not GalatTipe

Reproduced. The guide's 'Argumen Bernama' section says a parameter filled twice raises GalatTipe. Both the duplicate-keyword case and the positional-plus-keyword case instead raise an untyped 'Harapkan paling banyak 1 argumen tetapi mendapat 2', which does not mention the duplicate. Python rejects f(a=1, a=2) at compile time. OS-independent.

Reproduksi:

```text
fungsi f(a) { kembalikan a }
coba { f(a=1, a=2) } kecuali GalatTipe sebagai e { cetak("GalatTipe", e.pesan) } kecuali e { cetak("bukan", e) }
coba { f(1, a=2) } kecuali GalatTipe sebagai e { cetak("GalatTipe", e.pesan) } kecuali e { cetak("bukan", e) }
```

Keluaran 1.4.0:

```text
bukan Harapkan paling banyak 1 argumen tetapi mendapat 2.
bukan Harapkan paling banyak 1 argumen tetapi mendapat 2.
```

### ALUR-FUNGSI-11 — [rendah] lanjutkan() rejects a generator object unless it is wrapped in iter() first

Reproduced. lanjutkan(iter(g())) works, but lanjutkan(g()) fails. The guide says 'iter() dan lanjutkan() juga bekerja pada generator', and in Python a generator is its own iterator. The guide's wording is a little ambiguous, so I kept this rendah. OS-independent.

Reproduksi:

```text
fungsi g() { hasilkan 1 }
cetak(lanjutkan(iter(g())))
cetak(lanjutkan(g()))
```

Keluaran 1.4.0:

```text
1
lanjutkan() memerlukan iterator dari iter().  exit 70
```

### ALUR-FUNGSI-12 — [rendah] global and nonlokal for the same name in one function are accepted silently

Reproduced: declaring both for one name compiles and runs with exit 0. Python rejects it at compile time. When the name is then assigned, the program fails with an unrelated message ('nonlokal' harus merujuk variabel pada fungsi pembungkus), so the conflict is never reported clearly. OS-independent.

Reproduksi:

```text
fungsi f() {
    variabel x = 1
    fungsi g() {
        global x
        nonlokal x
    }
    g()
}
f()
cetak("diterima")
```

Keluaran 1.4.0:

```text
diterima  exit 0
```

### ALUR-FUNGSI-13 — [rendah] Extra "Harapkan '}' setelah blok" error at EOF after a semantic error inside a block

Reproduced. The braces are balanced, but after the 'hentikan' semantic error the compiler also reports a missing '}' at end of file. Recovery after semanticError is cascading, which AGENTS.md says it should not do. Only the message is affected; the exit code (65) is correct. OS-independent.

Reproduksi:

```text
fungsi f() {
    hentikan
}
```

Keluaran 1.4.0:

```text
[baris 2, kolom 5] Kesalahan di 'hentikan': 'hentikan' hanya boleh di dalam perulangan.
[baris 4, kolom 1] Kesalahan di akhir: Harapkan '}' setelah blok.  exit 65
```

### ALUR-FUNGSI-14 — [rendah] buang error message still says kamus keys must be text

Reproduced. Since K8 any hashable value can be a kamus key, as the guide's Kamus section shows, so the phrase 'kamus dengan kunci teks' is outdated. The error is correct to raise; only the wording is wrong. OS-independent.

Reproduksi:

```text
st = "abc"
buang st[0]
```

Keluaran 1.4.0:

```text
'buang' elemen memerlukan daftar dengan indeks angka atau kamus dengan kunci teks.  exit 70
```

- *Ditolak (salah_uji):* untuk a, b dalam ... and unpacking of generator/kamus/himpunan are not supported — Reproduced as described, but this is a missing feature, not a contradiction of the guide. The guide documents unpacking for sequences only, and the error message lists exactly the supported types (daftar, tupel, teks, rentang). The guide's enumerasi example uses pasangan[0] rather than for-target unpacking. The reporter also calls it a gap. Worth recording as a feature request.

## Koleksi

Pemeriksaan: **319/357 lulus**; temuan terkonfirmasi: **12**; ditolak verifikator: 1.

| Sub-area | Lulus |
|---|---|
| daftar: metode, indeks, irisan, + dan *, ==/adalah, fungsi bawaan (t1_daftar.idk) | 84/96 |
| kamus: urutan, kunci campur, 1/1.0, unhashable, hapus/buang (t3_kamus.idk, t4_probe_kamus.idk) | 39/43 |
| tupel, himpunan, himpunan_beku (t5_tupel_himpunan.idk) | 79/80 |
| mutasi saat iterasi, cetak rekursif [...], iter/lanjutkan (t6_iterasi.idk) | 13/23 |
| kasus tepi: REPEAT_MAX, NaN, luapan, sarang dalam, hash dalam (t8_tepi.idk + r_*) | 45/50 |
| metode pada tupel/himpunan/kamus, stabilitas urut, repr (t9_lain.idk) | 35/36 |
| performa 100k elemen (t7_perf.idk + r_*) | 24/29 |

### KOLEKSI-01 — [tinggi] Stack overflow crash saat meng-hash tupel bersarang dalam (kunci kamus / anggota himpunan)

Reproduksi independen: 20000 tingkat (a,) lalu d[a]=1 dan himpunan([a]) (meski di dalam coba/kecuali) keduanya keluar dengan -1073741571 (0xC00000FD), tanpa keluaran. Interpreter crash, bukan galat yang bisa ditangkap. hashValue merekursi tanpa batas kedalaman, padahal valuesEqual punya batas. Bug ini tidak khusus Windows. Ambang crash di Windows lebih rendah karena stack C hanya 1 MB (di Linux 8 MB), tetapi Linux tetap akan crash pada kedalaman yang lebih besar.

Reproduksi:

```text
a = ()
untuk i dalam rentang(20000) {
    a = (a,)
}
d = kamus()
d[a] = 1
cetak(panjang(d))
```

Keluaran 1.4.0:

```text
(tanpa keluaran) exit=-1073741571; varian coba { himpunan([a]) } kecuali e {...} juga exit=-1073741571
```

### KOLEKSI-02 — [sedang] == diam-diam mengembalikan salah untuk daftar yang sama secara struktural bila bersarang >= 64 tingkat

Kedalaman 63 memberi benar, 64 memberi salah. 'dalam' mewarisi hasil yang sama. Daftar yang saling merujuk (e berisi f, f berisi e) juga memberi salah, padahal Python menaikkan RecursionError. Ini akibat EQUAL_MAX_DEPTH yang sengaja dipasang, tetapi melampaui batas seharusnya memunculkan galat, bukan jawaban salah tanpa pemberitahuan. Panduan menjanjikan == struktural (K7). Keparahan saya turunkan ke sedang karena struktur sedalam 64 tingkat jarang dipakai dan tidak ada crash. Logikanya ada di value.c, jadi tidak bergantung OS.

Reproduksi:

```text
fungsi bangun(n) {
    a = []
    untuk i dalam rentang(n) {
        a = [a]
    }
    kembalikan a
}
cetak(bangun(63) == bangun(63))
cetak(bangun(64) == bangun(64))
```

Keluaran 1.4.0:

```text
benar
salah
```

### KOLEKSI-03 — [sedang] Menghapus kunci kamus / anggota himpunan kuadratik: 100k penghapusan 13 s sampai >120 s

Waktu terukur ulang: buang d[i] dari belakang 20k butuh 3,5 s dan 50k butuh 33,1 s (kuadratik). s.hapus(i) atas 100k anggota himpunan butuh 13,5 s. Penyebabnya penggeseran ObjDict.keys saat hapus, sesuai AGENTS.md. Hasilnya benar, hanya lambat, jadi keparahan saya turunkan ke sedang. Algoritmanya tidak bergantung OS.

Reproduksi:

```text
s = himpunan(rentang(100000))
untuk i dalam rentang(100000) {
    s.hapus(i)
}
cetak(panjang(s))
```

Keluaran 1.4.0:

```text
0 (real 13,5 s); buang d[i] dari belakang 50k: 0 (real 33,1 s)
```

### KOLEKSI-04 — [sedang] daftar += membuat daftar baru (bukan di tempat): alias tidak ikut berubah dan loop += jadi kuadratik

Setelah b = a; a += [3], keluarannya [1, 2] [1, 2, 3] salah. Panduan memang menulis 'x += 5 # x = x + 5', tetapi hanya untuk angka. Identitas bahasa ini adalah semantik Python, dan di Python list += mengubah daftar di tempat. Akibatnya alias tidak ikut berubah dan perulangan += menjadi kuadratik. Tidak bergantung OS.

Reproduksi:

```text
a = [1, 2]
b = a
a += [3]
cetak(b, a, a adalah b)
```

Keluaran 1.4.0:

```text
[1, 2] [1, 2, 3] salah
```

### KOLEKSI-05 — [sedang] Membaca kunci kamus yang tidak ada mengembalikan kosong (bukan GalatKunci); hapus_kunci kunci hilang diam-diam salah

d["z"] mencetak kosong dan hapus_kunci mengembalikan salah, exit 0. Panduan tidak mendokumentasikan pengembalian kosong. Perilaku ini tidak konsisten dengan buang d[k] dan s.hapus(x) yang menaikkan GalatKunci, dan menyimpang dari semantik dict Python. Salah ketik kunci jadi tidak terdeteksi. Tidak bergantung OS.

Reproduksi:

```text
d = {"a": 1}
cetak(d["z"])
cetak(hapus_kunci(d, "z"))
```

Keluaran 1.4.0:

```text
kosong
salah
```

### KOLEKSI-06 — [sedang] Mengubah daftar/kamus/himpunan saat untuk: panjang dipotret di awal, elemen tambahan dilewati, penghapusan memicu galat indeks yang salah

Elemen yang ditambahkan saat perulangan tidak dikunjungi. Hasil saya '6 [1, 2, 3, 10, 20, 30]'; angka '3' dalam laporan penguji tampaknya keliru cetak, tetapi intinya sama. Menghapus elemen saat perulangan memicu 'Indeks list di luar jangkauan' dengan caret di baris untuk. Menambah kunci kamus saat perulangan diam-diam diabaikan (panjang 4, tanpa galat), padahal Python menaikkan RuntimeError. Pola worklist dengan menambah elemen saat iterasi lazim di Python. Penyebabnya OP_LIST_LEN dievaluasi sekali, tidak bergantung OS.

Reproduksi:

```text
a = [1, 2, 3]
untuk x dalam a {
    a.tambah(x * 10)
}
cetak(panjang(a), a)
b = [1, 2, 3, 4]
untuk x dalam b {
    hapus_pada(b, 0)
}
cetak(b)
```

Keluaran 1.4.0:

```text
6 [1, 2, 3, 10, 20, 30]
Indeks list di luar jangkauan (indeks 2, panjang 2).
[baris 7, kolom 17] dalam skrip
    untuk x dalam b {
exit=70
```

### KOLEKSI-07 — [rendah] hapus/hapus_pada/sisip di luar jangkauan menaikkan GalatNilai, bukan GalatIndeks

hapus([]) dan hapus_pada([1], 5) tertangkap sebagai GalatNilai, bukan GalatIndeks. Hal ini tidak konsisten dengan [1][5] yang menaikkan GalatIndeks, dan Python menaikkan IndexError. Keparahan saya turunkan ke rendah: galat tetap terdeteksi dan tetap bisa ditangkap lewat Galat atau kecuali e, hanya kelasnya yang keliru. Tidak bergantung OS.

Reproduksi:

```text
coba {
    hapus_pada([1], 5)
} kecuali GalatIndeks sebagai e {
    cetak("indeks")
} kecuali GalatNilai sebagai e {
    cetak("nilai", e.pesan)
}
```

Keluaran 1.4.0:

```text
nilai hapus_pada() indeks di luar jangkauan (indeks 5, panjang 1)
```

### KOLEKSI-08 — [rendah] hapus_pada dan sisip menolak indeks negatif; sisip melewati panjang galat alih-alih di-clamp

hapus_pada(a,-1), sisip(a,100,9), dan sisip(a,-1,8) semuanya ditolak. Panduan hanya menyebut indeks negatif berlaku untuk akses indeks dan penugasan, tidak eksplisit untuk hapus_pada/sisip. Namun semantik Python yang dijanjikan (pop(-1), clamp pada insert) tidak dipenuhi. Karena hasilnya galat yang jelas, bukan data rusak, keparahannya rendah. Tidak bergantung OS.

Reproduksi:

```text
a = [1, 2, 3]
cetak(hapus_pada(a, -1))
```

Keluaran 1.4.0:

```text
hapus_pada() indeks di luar jangkauan (indeks -1, panjang 3); sisip(a,100,9) dan sisip(a,-1,8) juga 'sisip() indeks di luar jangkauan'
```

### KOLEKSI-09 — [rendah] Indeks dan batas irisan desimal dipotong diam-diam ([10,20][1.5] -> 20)

[10, 20][1.5] menghasilkan 20, irisan [0.5:1.9] menghasilkan [10], dan [1] * 2.0 menghasilkan [1, 1]. Python menaikkan TypeError untuk ketiganya. Perilaku ini juga tidak konsisten dengan [1] * 2.5 yang ditolak. Tidak bergantung OS.

Reproduksi:

```text
cetak([10, 20][1.5])
cetak([10, 20, 30][0.5:1.9])
cetak([1] * 2.0)
```

Keluaran 1.4.0:

```text
20
[10]
[1, 1]
```

### KOLEKSI-10 — [rendah] Perbandingan urutan < > antar daftar/tupel tidak didukung, pesannya 'Operand harus berupa angka.'

[1,2] < [1,3] dan (1,2) < (1,3) menaikkan 'Operand harus berupa angka.' Pesan ini menyesatkan karena teks < teks didukung. Python membandingkan urutan secara leksikografis. terurut([[2],[1]]) juga ditolak. Fitur ini tidak didokumentasikan, tetapi pesannya keliru dan semantik Python tidak dipenuhi. Tidak bergantung OS.

Reproduksi:

```text
cetak([1, 2] < [1, 3])
```

Keluaran 1.4.0:

```text
Operand harus berupa angka. (exit 70)
```

### KOLEKSI-11 — [rendah] punya(kamus, daftar) mengembalikan salah, bukan GalatTipe kunci tak dapat di-hash

punya(d, [1]) dan [1] dalam d mengembalikan salah. Dokumentasi punya() menyatakan kunci boleh nilai apa pun yang hashable, dan d[[1]] menaikkan GalatTipe. Python menaikkan TypeError. Dampaknya kecil karena jawaban salah secara logis memang benar. Tidak bergantung OS.

Reproduksi:

```text
d = kamus()
cetak(punya(d, [1]))
cetak([1] dalam d)
```

Keluaran 1.4.0:

```text
salah
salah
```

### KOLEKSI-12 — [rendah] max/min bulat vs desimal dibandingkan lewat double: hilang presisi di atas 2**53

max(9007199254740993, 9007199254740992.0) mengembalikan 9007199254740992.0. Operator > pada pasangan yang sama memberi benar dengan tepat, jadi masalahnya hanya di max/min, yang tidak konsisten dengan operator perbandingan bahasa ini sendiri. Python mengembalikan 9007199254740993. Kasus tepi, tidak bergantung OS.

Reproduksi:

```text
cetak(max(9007199254740993, 9007199254740992.0))
cetak(9007199254740993 > 9007199254740992.0)
```

Keluaran 1.4.0:

```text
9007199254740992.0
benar
```

- *Ditolak (salah_uji):* Iterator dari iter() tidak bisa dipakai di untuk ... dalam maupun daftar() — Perilakunya memang terulang, tetapi tidak ada janji dokumentasi yang dilanggar. 07_PUSTAKA_STANDAR §1.3e menempatkan iter() sebagai alat lanjutkan() dan menyarankan untuk ... dalam langsung pada koleksinya. Pesan galat menyebut tipe yang diterima secara eksplisit. Panduan sintaks juga mencatat bahwa protokol iterator penuh dijadwalkan di fase berikutnya, dan AGENTS.md menyatakan untuk hanya meneri

## Kelas dan OOP

Pemeriksaan: **236/262 lulus**; temuan terkonfirmasi: **22**; ditolak verifikator: 1.

| Sub-area | Lulus |
|---|---|
| kelas/init/diri/atribut instance, argumen bawaan & bernama pada init | 21/22 |
| atribut kelas (bayangan instance, pewarisan, sekali dievaluasi, ruang nama bersama) | 15/16 |
| pewarisan tunggal, super (init, metode, ajaib, properti, metode_kelas, atribut kelas) | 14/18 |
| pewarisan berganda + MRO C3 (diamond satu sisi, atribut/properti/statis/ajaib lewat MRO, galat MRO) | 19/20 |
| metode ajaib: _teks_/_repr_/cetak/f-string/format, _sama_/_tidak_sama_, perbandingan + cerminan, aritmetika + cerminan k*, _negatif_, += | 45/45 |
| protokol: _panjang_, _logika_, _ambil_/_atur_, _hash_ (kamus/himpunan) | 24/28 |
| _iter_/_berikutnya_/generator/_panggil_ | 17/24 |
| dekorator @properti @metode_statis @metode_kelas @abstrak (+ galat kompilasi) | 26/30 |
| adalah_instansi, bisa_dipanggil, atur/ambil/punya_atribut, jenis | 17/20 |
| galat di dalam metode ajaib (31 jalur) + rekursi lewat metode ajaib (17 kasus, tanpa crash) | 42/48 |
| dengan ... sebagai (normal, galat, kembalikan/hentikan/lanjut, bersarang, _masuk_/_keluar_ gagal) | 15/16 |
| stres/GC: 200k objek, kamus 5000 kunci objek, terurut 2000 objek, mutasi koleksi dari dalam metode ajaib | 10/10 |

### OOP-01 — [tinggi] Galat dari _logika_ tertunda: cabang 'jika' tetap dijalankan, galat baru muncul belakangan

Reproduced independently. Under 03 'Metode ajaib', _logika_ is used by jika/selagi/tidak/dan/atau. A galat raised there must abort the condition, as Python __bool__ does. Instead the branch body runs and the pending native error surfaces only at the next native call. Uncaught, it is reported as 'Kesalahan tak tertangani dari fungsi bawaan' and the type and message are lost. 'tidak B()' returns salah, 'B() atau 1' returns the object, and the 'selagi' body runs once before the error. The cause is VM logic (isFalsey does not resolve the pending error), so nothing here is Windows-specific.

Reproduksi:

```text
kelas B { _logika_() { naikkan GalatNilai } }
cetak("a")
jika B() { cetak("CABANG") }
cetak("sesudah")
```

Keluaran 1.4.0:

```text
stdout: a / CABANG; stderr: Kesalahan tak tertangani dari fungsi bawaan. [baris 3, kolom 26] (exit 70). Variant: 'x = tidak B()' prints 'TIDAK lolos salah' before the error is caught.
```

### OOP-02 — [tinggi] Galat dari _sama_ saat tabrakan hash di kamus tertunda: kamus tetap diubah dan pernyataan berikutnya jalan

Reproduced. Reading d[K()] keeps running ('tidak boleh tercetak' is printed, langkah=1). d[K()] = 2 inserts a second key and the later statements run. Uncaught, the traceback points at a later line (panjang(d)) with the generic 'dari fungsi bawaan' message. This violates the K19 rule that a magic error must unwind immediately. The 'dalam', list '==' and himpunan paths already behave correctly, so this is an inconsistency in the kamus index paths.

Reproduksi:

```text
kelas K {
    _sama_(l) { naikkan GalatNilai }
    _hash_() { kembalikan 1 }
}
d = {}
d[K()] = 1
d[K()] = 2
cetak("LOLOS", panjang(d))
```

Keluaran 1.4.0:

```text
Kesalahan tak tertangani dari fungsi bawaan. [baris 8, kolom 25] cetak("LOLOS", panjang(d)) (exit 70); reported on line 8, not 7. With a coba: 'ditangkap: dari _sama_ | langkah = 1 | panjang d = 2'
```

### OOP-03 — [sedang] Metode diambil lewat kelas (K.m) atau @metode_kelas diambil sebagai nilai: diri/kelas_ini terikat ke fungsinya sendiri

Reproduced. f = K.siapa; f() returns '<fungsi siapa>' instead of <kelas K>. h = k.buat; h() fails with 'Harapkan paling banyak 0 argumen'. f = K.m; f() silently returns '<fungsi m>' as diri. 03 says @metode_kelas receives its class as kelas_ini; it does not say this is limited to the direct-call form. Silent wrong values are serious, but taking a method as a value is less common, so I rate it sedang. The unbound call K.m(K()) is not documented, so only the silent-binding part is a clear bug.

Reproduksi:

```text
kelas K {
    @metode_kelas
    siapa() { kembalikan kelas_ini }
    m() { kembalikan diri }
}
f = K.siapa
cetak(f())
g = K.m
cetak(g())
```

Keluaran 1.4.0:

```text
<fungsi siapa>
<fungsi m>
```

### OOP-04 — [sedang] super.p pada @properti mengembalikan fungsi, bukan nilai properti

Reproduced: it prints '<fungsi p>' and then 'fungsi' with exit 0. 03 says @properti is inherited and may be overridden with @properti again. Reaching the parent property through super is the natural use of that, and in Python super().p evaluates the property. The result is a silent wrong value.

Reproduksi:

```text
kelas A {
    @properti
    p() { kembalikan "pA" }
}
kelas B < A {
    @properti
    p() { kembalikan super.p }
}
cetak(B().p)
```

Keluaran 1.4.0:

```text
<fungsi p>
```

### OOP-05 — [sedang] Penugasan gabungan pada atribut/indeks (diri.x += 1, d[0] += 1) ditolak dengan pesan menyesatkan + galat kaskade

Reproduced for both the attribute and the index form. The error 'Harapkan ekspresi' at '+=' does not say that the form is unsupported. Inside a class, a valid later method then gets a bogus 'Tidak dapat mengembalikan nilai dari init' plus a 'Harapkan }' error. The docs only show compound assignment on plain variables, but the metode ajaib table lists '+=' and Python semantics are the reference. diri.x += 1 is a very common idiom.

Reproduksi:

```text
d = [1]
d[0] += 1
cetak(d)
```

Keluaran 1.4.0:

```text
[baris 2, kolom 6] Kesalahan di '+=': Harapkan ekspresi. (exit 65); in the class case there are 2 more cascade errors
```

### OOP-06 — [sedang] bisa_dipanggil() mengembalikan salah untuk objek ber-_panggil_

Reproduced: k(3) = 6 works, but bisa_dipanggil(k) returns salah. The docs contradict each other here. 07 says bisa_dipanggil reports whether x can be called like a function. 03 says _panggil_ makes an object callable like a function.

Reproduksi:

```text
kelas Kali {
    init(n) { diri.n = n }
    _panggil_(x) { kembalikan x * diri.n }
}
k = Kali(2)
cetak(k(3), bisa_dipanggil(k))
```

Keluaran 1.4.0:

```text
6 salah
```

### OOP-07 — [sedang] Comprehension dan iter() menolak objek ber-_iter_ yang diterima 'untuk', dengan pesan galat yang mengklaim objek _iter_ diterima

Reproduced. [x untuk x dalam Koleksi()] fails inside listcomp with a message that itself says objects with _iter_() are accepted, while the untuk statement works. iter(Gen()) also fails with a message that claims _iter_ is accepted. 07 §2.12 documents that daftar/tupel/terurut/jumlah/semua do not accept _iter_ objects, so that part is a known limitation. The comprehension (same 'untuk ... dalam' syntax, 03 _iter_ section) and iter() cases are not covered by it.

Reproduksi:

```text
kelas Koleksi {
    _iter_() { kembalikan [1, 2, 3] }
}
cetak([x untuk x dalam Koleksi()])
```

Keluaran 1.4.0:

```text
'untuk ... dalam' hanya dapat mengiterasi daftar, tupel, himpunan, teks, rentang, kamus, atau objek dengan metode _iter_(). [dalam listcomp()] (exit 70)
```

### OOP-08 — [sedang] Iterator bawaan (hasil iter()) tidak bisa di-'untuk' atau di-daftar()

Reproduced. 'untuk x dalam iter([1])' and daftar(iter([7,8])) both fail. User iterators (_iter_ returns diri + _berikutnya_) and generators do work in untuk. The built-in iterator type is therefore the only iterator that is not iterable, which is inconsistent and contradicts the Python iterator protocol. The docs (07 iter/lanjutkan) do not explicitly promise this, so I rate it sedang and not tinggi.

Reproduksi:

```text
untuk x dalam iter([1, 2]) { cetak(x) }
```

Keluaran 1.4.0:

```text
'untuk ... dalam' hanya dapat mengiterasi daftar, tupel, himpunan, teks, rentang, kamus, atau objek dengan metode _iter_(). [baris 1, kolom 26] (exit 70)
```

### OOP-09 — [sedang] super di dalam @metode_kelas gagal: 'Variabel tidak terdefinisi diri'

Reproduced. super inside a classmethod compiles to a 'diri' lookup, which does not exist there (its binding is named kelas_ini). The error leaks an internal detail. Docs: @metode_kelas is inherited and super points to the first parent, so super.k() in a subclass classmethod is a reasonable use.

Reproduksi:

```text
kelas A {
    @metode_kelas
    k() { kembalikan "kA" }
}
kelas B < A {
    @metode_kelas
    k() { kembalikan "kB+" + super.k() }
}
cetak(B.k())
```

Keluaran 1.4.0:

```text
Variabel tidak terdefinisi 'diri'. [baris 7, kolom 36] dalam k() (exit 70)
```

### OOP-10 — [sedang] Galat dari _hash_ ditutupi pesan generik pada hash(), himpunan(), himpunan.tambah()

Reproduced. The kamus literal and 'dalam' paths propagate G correctly, but hash(), himpunan([...]) and .tambah() replace it with a generic message, so 'kecuali G' does not catch it. This contradicts the AGENTS K19 rule 'a pending native error outranks the caller's generic message' and Python behaviour.

Reproduksi:

```text
kelas G < Galat { init(p) { diri.pesan = p } }
kelas B { _hash_() { naikkan G("dari _hash_") } }
coba { hash(B()) } kecuali G sebagai e { cetak("ok", e.pesan) } kecuali e { cetak("salah:", e) }
```

Keluaran 1.4.0:

```text
salah: Nilai itu tidak dapat di-hash.
```

### OOP-11 — [sedang] Argumen terlalu banyak melempar Galat tak bertipe, bukan GalatTipe

Reproduced. f(1,2) for f(a), K(1) for a class without init, and a method with too many arguments all raise a Galat that 'kecuali GalatTipe' does not catch. Too few arguments raise GalatTipe ('Parameter wajib ...'). The two are inconsistent, and Python raises TypeError for both.

Reproduksi:

```text
fungsi f(a) { }
coba { f(1, 2) } kecuali GalatTipe sebagai e { cetak("tipe") } kecuali e { cetak("lain", e) }
```

Keluaran 1.4.0:

```text
lain Harapkan paling banyak 1 argumen tetapi mendapat 2.
```

### OOP-12 — [rendah] panjang() pada nilai tak berpanjang melempar Galat tak bertipe, bukan GalatTipe

Reproduced for both panjang(K()) and panjang(5): 'kecuali GalatTipe' does not catch it. Python raises TypeError, and 03 uses GalatTipe for the analogous protocol cases. Rated rendah because the error is still catchable with plain 'kecuali e' and its message is clear.

Reproduksi:

```text
coba { panjang(5) } kecuali GalatTipe sebagai e { cetak("tipe") } kecuali e { cetak("lain:", e) }
```

Keluaran 1.4.0:

```text
lain: panjang() hanya menerima string, list, kamus, rentang, atau objek dengan _panjang_()
```

### OOP-13 — [rendah] Lokasi traceback galat yang keluar dari blok dengan/akhirnya menunjuk '}' penutup

Reproduced for both dengan and coba/akhirnya. The error is reported at the closing '}' of the block (the rethrow site) instead of the line that caused it. 03 'Membaca Pesan Kesalahan' says the snippet points at the offending expression. The message text is still correct, so this only affects diagnostics.

Reproduksi:

```text
fungsi f() {
    coba {
        x = [1][5]
    } akhirnya {
        cetak("fin")
    }
}
f()
```

Keluaran 1.4.0:

```text
fin / Indeks list di luar jangkauan (indeks 5, panjang 1). [baris 6, kolom 5] dalam f() snippet '}'
```

### OOP-14 — [rendah] Atribut kelas tidak bisa merujuk atribut kelas sebelumnya di badan kelas

Reproduced. 03 says class attributes follow Python's rules, and in Python a class body can refer to earlier names (B = A * 3). The bullet list in 03 does not mention this case explicitly, and the workaround K.A works, so I rate it rendah.

Reproduksi:

```text
kelas K {
    A = 2
    B = A * 3
}
cetak(K.B)
```

Keluaran 1.4.0:

```text
Variabel tidak terdefinisi 'A'. [baris 3, kolom 9] (exit 70)
```

### OOP-15 — [rendah] super.X untuk atribut kelas induk gagal

Reproduced. 03 says class attributes are looked up through the MRO and inherited, and super points to the first parent, but super only looks at methods. The workaround of writing the parent's class name works.

Reproduksi:

```text
kelas A { X = 1 }
kelas B < A { f() { kembalikan super.X } }
cetak(B().f())
```

Keluaran 1.4.0:

```text
Properti tidak terdefinisi 'X'. (exit 70)
```

### OOP-16 — [rendah] Pesan kelas abstrak menyebut 'keliling', dokumentasi menyebut 'luas'

Reproduced with the same class as the 03 'Kelas abstrak' example. The doc shows metode 'luas', but the runtime reports 'keliling' (apparently the last abstract method rather than the first). The behaviour is correct otherwise; only the doc and implementation disagree on the order.

Reproduksi:

```text
kelas Bentuk {
    @abstrak
    luas() { }
    @abstrak
    keliling() { }
}
Bentuk()
```

Keluaran 1.4.0:

```text
Kelas 'Bentuk' abstrak: metode 'keliling' belum diisi subkelas.
```

### OOP-17 — [rendah] Atribut kelas di subkelas tidak dianggap mengisi metode @abstrak

Reproduced. 03 says attributes and methods share one namespace and the later one wins, and Python's abc clears the abstract method when any attribute overrides it. Here the abstract check only looks at methods. It is an edge case.

Reproduksi:

```text
kelas K {
    @abstrak
    f() { }
}
kelas L < K { f = 5 }
cetak(L().f)
```

Keluaran 1.4.0:

```text
Kelas 'L' abstrak: metode 'f' belum diisi subkelas. (exit 70)
```

### OOP-18 — [rendah] Rantai pewarisan tunggal sedalam 32+ kelas ditolak, batas tidak didokumentasikan

Reproduced. The limit is deliberate: AGENTS.md says the C3 worklist is bounded at 32 to avoid a C-stack overflow. It is not mentioned in the user guide (03 Pewarisan berganda), and a linear chain should not need the N×N worklist. This is mainly a documentation gap; such deep chains are rare in practice.

Reproduksi:

```text
kelas A0 { f() { kembalikan 0 } }
fungsi rantai(n) {
    variabel k = A0
    untuk i dalam rentang(n) {
        kelas Baru < k { }
        k = Baru
    }
    kembalikan k
}
cetak(rantai(32)().f())
```

Keluaran 1.4.0:

```text
Rantai pewarisan terlalu panjang. (exit 70)
```

### OOP-19 — [rendah] ambil_atribut dengan terlalu banyak argumen memberi pesan salah

Reproduced. With 4 arguments the error claims the name is not text, even though "x" is text. The argument count should be reported. The impact is only a misleading message.

Reproduksi:

```text
kelas K { }
ambil_atribut(K(), "x", 1, 2)
```

Keluaran 1.4.0:

```text
ambil_atribut(objek, nama[, bawaan]) memerlukan nama berupa teks
```

### OOP-20 — [rendah] _logika_ yang mengembalikan bulat diterima tanpa galat

Reproduced. Python raises TypeError when __bool__ returns an int, and _panjang_/_hash_ do check their return types, so this is inconsistent. The docs do not specify a return-type check for _logika_, though. It is lenient but harmless.

Reproduksi:

```text
kelas L { _logika_() { kembalikan 1 } }
jika L() { cetak("diterima") }
```

Keluaran 1.4.0:

```text
diterima (exit 0)
```

### OOP-21 — [rendah] Galat kompilasi super/diri pada blok satu baris diikuti galat palsu 'Harapkan } setelah blok'

Reproduced with both 'super' in a class without a parent and 'diri' outside a class on a one-line block. A second bogus error at EOF follows. The multi-line form is clean. This contradicts the stated intent that semanticError avoids panic-mode cascades.

Reproduksi:

```text
fungsi f() { kembalikan diri }
```

Keluaran 1.4.0:

```text
[baris 1, kolom 25] Kesalahan di 'diri': Tidak dapat menggunakan 'diri' di luar kelas. / [baris 2, kolom 1] Kesalahan di akhir: Harapkan '}' setelah blok. (exit 65)
```

### OOP-22 — [rendah] Pesan 'buang' pada objek usang: 'kamus dengan kunci teks'

Reproduced. Since K8, kamus keys can be any hashable value, and buang d[1] works, so the message is stale. It also does not name the actual type (object).

Reproduksi:

```text
kelas K { }
k = K()
buang k[0]
```

Keluaran 1.4.0:

```text
'buang' elemen memerlukan daftar dengan indeks angka atau kamus dengan kunci teks.
```

- *Ditolak (salah_uji):* 'kecuali NamaKelasGalat {' tanpa 'sebagai' menjadi tangkap-semua — This follows the documented grammar. 03 documents 'kecuali Tipe sebagai e' for typed catches and 'kecuali e' (a bare identifier binding) for catch-all, and that form must be last. The compiler deliberately treats 'kecuali GalatTipe {' as untyped: if it is not the last clause it rejects it with ''kecuali' tanpa tipe harus menjadi yang terakhir'. The tester's expectation of a typed catch without seb

## Galat, pesan, kode keluar

Pemeriksaan: **243/270 lulus**; temuan terkonfirmasi: **18**.

| Sub-area | Lulus |
|---|---|
| Built-in error types triggered on purpose (t01, t05, t02b, r09, r14-r18) | 82/95 |
| coba/kecuali/akhirnya combinations, rethrow, sebagai, tuple of classes, custom subclasses (t03, t06, w01-w18, r01, r04, r05, r13) | 69/75 |
| Uncaught runtime error output format + exit 70 (u01-u08, v01-v13, r06-r08) | 21/24 |
| Compile errors exit 65 and beginner-mistake messages (c01-c27, d01-d18, p05-p06) | 42/47 |
| pastikan (p01-p04, t03 D10-D13) | 8/10 |
| keluar(n) and CLI exit codes 74 (k01-k17, missing file/dir/flag/empty) | 21/21 |
| Errors inside imported modules (m01-m10) | 8/10 |

### GALAT-01 — [tinggi] ✅ **Diperbaiki (terverifikasi ulang 1.4.0).** pastikan(kondisi, pesan) with parentheses always passes silently (docs say compile error)

03_PANDUAN_SINTAKS 'Penegasan (pastikan)' says this form is rejected at compile time with "Harapkan ')' setelah ekspresi" and does not silently pass. The binary parses it as a non-empty tupel, which is always truthy. So a documented guarantee is broken, and a false assertion passes without any sign. The parser behaves the same on every OS.

Reproduksi:

```text
pastikan(1 == 2, "harus gagal")
cetak("LOLOS padahal kondisi salah")
```

Keluaran 1.4.0:

```text
LOLOS padahal kondisi salah
exit=0
```

### GALAT-02 — [tinggi] ✅ **Diperbaiki (terverifikasi ulang 1.4.0).** User subclass of a built-in error loses ancestors above its direct parent, so kecuali Galat / GalatAritmetika does not catch it

Reproduced. A plain chain C<B<A is transitive, and so is a user chain G2<G1<Galat. A user class that derives from a built-in subclass is not: N1<GalatNilai is not an instance of Galat, and B2<GalatBagiNol is not caught by kecuali GalatAritmetika. The guide says to catch errors with a parent class ('Gunakan kelas induk'), and Python's issubclass is transitive. This is OS-independent VM logic.

Reproduksi:

```text
kelas B2 < GalatBagiNol { }
coba { naikkan B2() } kecuali GalatAritmetika sebagai e { cetak("tangkap Aritmetika") } kecuali e { cetak("lolos ke catch-all") }
kelas N1 < GalatNilai { }
coba { naikkan N1() } kecuali Galat sebagai e { cetak("tangkap Galat") } kecuali e { cetak("lolos ke catch-all") }
```

Keluaran 1.4.0:

```text
lolos ke catch-all
lolos ke catch-all
exit=0 (without the catch-all: 'Kesalahan tak tertangani.' exit 70)
```

### GALAT-03 — [tinggi] 'kecuali GalatX {' without 'sebagai' silently becomes a catch-all that binds the error to the name GalatX

Reproduced. 'kecuali GalatNilai {' is parsed as the untyped 'kecuali e' form, with the variable named GalatNilai. It catches a GalatTipe, and inside the handler GalatNilai is the message text, not the class. The guide documents only 'kecuali T sebagai e' and 'kecuali e'. The tupel form already requires 'sebagai'. Python's 'except ValueError:' is typed. Shipped examples (db_batas_tunggu.idk, db_blob.idk, http_pengalihan.idk) use this form as if it were typed, so they catch every error.

Reproduksi:

```text
coba { "a" + 1 } kecuali GalatNilai { cetak("tertangkap padahal GalatTipe; GalatNilai =", GalatNilai) }
```

Keluaran 1.4.0:

```text
tertangkap padahal GalatTipe; GalatNilai = Operand '+' harus dua angka, dua teks, dua daftar, atau dua tupel.
exit=0
```

### GALAT-04 — [sedang] Recursive function containing coba fails at depth 64 with plain Galat 'Terlalu banyak blok coba bersarang'

Reproduced. src/vm.h sets TRY_MAX to 64, and that handler stack is global across frames. So recursion that passes through a coba stops at depth 63 with a plain Galat, not GalatRekursi. Without coba, the same recursion reaches GalatRekursi correctly. Python allows about 1000 frames here. This is an internal limit and does not depend on the OS.

Reproduksi:

```text
fungsi s(n) { coba { kembalikan s(n+1) } kecuali GalatTipe sebagai e { kembalikan 0 } }
coba { s(0) } kecuali GalatRekursi sebagai e { cetak("rekursi") } kecuali e { cetak("lain:", e.pesan) }
```

Keluaran 1.4.0:

```text
lain: Terlalu banyak blok 'coba' bersarang.
exit=0
```

### GALAT-05 — [sedang] Uncaught error that passes through akhirnya or an unmatched kecuali reports the closing '}' instead of the original line

Reproduced. The division by zero is on line 3, but the error is reported at [baris 7, kolom 1] with the snippet '}', because the rethrow site replaces the original location. The guide says the snippet is printed for the place where the error actually happened ('tempat galat benar-benar terjadi'). This is OS-independent.

Reproduksi:

```text
coba {
    x = 1
    y = x / 0
    z = 3
} akhirnya {
    cetak("bersih")
}
```

Keluaran 1.4.0:

```text
stderr: Pembagian dengan nol.
[baris 7, kolom 1] dalam skrip
    }
    ^
stdout: bersih
exit=70
```

### GALAT-06 — [sedang] Undefined variable raises plain Galat, not the documented GalatNama

The guide's line 'buang x  # ... membacanya lagi = GalatNama' is explicitly contradicted: reading x after buang raises plain Galat. Reading an undefined name also gives plain Galat. Only 'buang tidak_ada' raises GalatNama. Python raises NameError in both cases.

Reproduksi:

```text
x = 1
buang x
coba { cetak(x) } kecuali GalatNama sebagai e { cetak("GalatNama") } kecuali e { cetak("lain:", e.pesan) }
```

Keluaran 1.4.0:

```text
lain: Variabel tidak terdefinisi 'x'.
exit=0
```

### GALAT-07 — [sedang] Out-of-range index on teks and rentang is plain Galat, not GalatIndeks (list and tupel are fine)

Reproduced for both teks and rentang. List and tupel raise GalatIndeks for the same mistake, and Python raises IndexError for str and range. The inconsistency is in shared VM code, so it is not OS-specific.

Reproduksi:

```text
coba { cetak("ab"[9]) } kecuali GalatIndeks sebagai e { cetak("GalatIndeks") } kecuali Galat sebagai e { cetak("bukan GalatIndeks:", e.pesan) }
coba { cetak(rentang(3)[9]) } kecuali GalatIndeks sebagai e { cetak("GalatIndeks") } kecuali Galat sebagai e { cetak("bukan GalatIndeks:", e.pesan) }
```

Keluaran 1.4.0:

```text
bukan GalatIndeks: Indeks string di luar jangkauan (indeks 9, panjang 2).
bukan GalatIndeks: Indeks rentang di luar jangkauan (indeks 9, panjang 3).
exit=0
```

### GALAT-08 — [sedang] Type errors raised with the wrong class: too many arguments, 'dalam' on a non-container, panjang(int), akar('a')

Reproduced. g(1,2) and '1 dalam 5' raise plain Galat, while panjang(123) and akar('a') raise GalatNilai. Python raises TypeError for all of these. The documented case of a parameter filled twice correctly gives GalatTipe, so the problem is limited to these cases. The guide does not name the class for each case, but GalatTipe is the documented class for wrong types, and Python is the reference for semantics.

Reproduksi:

```text
fungsi g(a) { kembalikan a }
coba { g(1, 2) } kecuali GalatTipe sebagai e { cetak("T") } kecuali e { cetak("lain:", e.pesan) }
coba { 1 dalam 5 } kecuali GalatTipe sebagai e { cetak("T") } kecuali e { cetak("lain:", e.pesan) }
coba { panjang(123) } kecuali GalatTipe sebagai e { cetak("T") } kecuali GalatNilai sebagai e { cetak("panjang -> GalatNilai") }
coba { akar("a") } kecuali GalatTipe sebagai e { cetak("T") } kecuali GalatNilai sebagai e { cetak("akar -> GalatNilai") }
```

Keluaran 1.4.0:

```text
lain: Harapkan paling banyak 1 argumen tetapi mendapat 2.
lain: Operand kanan 'dalam' harus daftar, tupel, himpunan, teks, kamus, atau rentang.
panjang -> GalatNilai
akar -> GalatNilai
exit=0
```

### GALAT-09 — [sedang] buka_berkas on a missing file raises GalatNilai instead of GalatOS

07_PUSTAKA_STANDAR says OS errors are GalatOS, 'seperti OSError Python, dengan alasan yang sama di semua sistem operasi'. buka_berkas on a missing path raises GalatNilai, while isi_direktori correctly raises GalatOS. The failing branch is a generic open failure, not Windows path handling.

Reproduksi:

```text
coba { buka_berkas("C:/tidak/ada/x.txt", "b") } kecuali GalatOS sebagai e { cetak("GalatOS") } kecuali Galat sebagai e { cetak("bukan GalatOS:", e.pesan) }
```

Keluaran 1.4.0:

```text
bukan GalatOS: buka_berkas() gagal membuka 'C:/tidak/ada/x.txt'
exit=0
```

### GALAT-10 — [sedang] Missing dict key returns kosong silently instead of raising GalatKunci

Reproduced. This is a known semantic gap, listed in internal docs/ANALISIS/01 as 'Kunci kamus hilang | kosong | KeyError', with a planned fix. The user guide neither documents the kosong behaviour nor warns about it, and Python's KeyError is the promised reference. It is a real deviation, but a deliberately deferred one. This is OS-independent.

Reproduksi:

```text
k = {"a": 1}
cetak(k["b"])
```

Keluaran 1.4.0:

```text
kosong
exit=0
```

### GALAT-11 — [sedang] Traceback frames never name the module file, so an error inside an imported module shows two 'dalam skrip' frames

Reproduced. Both frames read 'dalam skrip' with no file name, although the snippet comes from mod_b.idk. A user cannot tell which file line 2 belongs to. Python names the file in each frame. The traceback formatter behaves the same on every OS.

Reproduksi:

```text
# modul/mod_b.idk:
cetak("mod_b dimuat")
y = tidak_ada_di_mod_b
# main:
impor "modul/mod_b.idk"
```

Keluaran 1.4.0:

```text
Variabel tidak terdefinisi 'tidak_ada_di_mod_b'.
[baris 2, kolom 5] dalam skrip
    y = tidak_ada_di_mod_b
        ^
[baris 1, kolom 7] dalam skrip
exit=70
```

### GALAT-12 — [sedang] Assignment accepted as a condition: 'jika x = 0 {' assigns silently and branches

Reproduced: x is silently overwritten with 0 and the selain branch runs. The implementation keeps assignment as an expression on purpose (AGENTS.md notes that '(x = 1)' stays an assignment expression). But the user guide does not document assignment expressions, and Python rejects 'if x = 0:' as a SyntaxError. This is a silent = vs == trap for beginners, not a crash.

Reproduksi:

```text
x = 2
jika x = 0 { cetak("cabang jika") } selain { cetak("cabang selain") }
cetak("x sekarang", x)
```

Keluaran 1.4.0:

```text
cabang selain
x sekarang 0
exit=0
```

### GALAT-13 — [sedang] Two expressions juxtaposed on one line are accepted: 'cetak "x"' or 'cetak x' is a silent no-op

Reproduced. 'cetak "nilai x"', 'cetak x' and 'x 5' compile and do nothing, and two calls on one line both run. Statements are supposed to end at a newline (B16), and Python rejects all of these as a SyntaxError. The Python 2 style 'cetak "x"' is a common beginner mistake, and here it produces no output and no error.

Reproduksi:

```text
x = 10
cetak "nilai x"
cetak x
x 5
cetak("selesai tanpa galat")
```

Keluaran 1.4.0:

```text
selesai tanpa galat
exit=0
```

### GALAT-14 — [rendah] Unterminated string is reported at the EOF line with no column or snippet

Reproduced. The string opens on line 1, but the error is reported at '[baris 5]' (EOF) with no column or caret. Triple-quoted strings behave the same ('[baris 3]'). The guide says compile errors give a line and column. This is the scanner and is OS-independent.

Reproduksi:

```text
cetak("halo)
cetak(1)
cetak(2)
cetak(3)
```

Keluaran 1.4.0:

```text
[baris 5] Kesalahan: String tidak ditutup.
exit=65
```

### GALAT-15 — [rendah] Errors 'di akhir baris' report the next line number while the snippet shows the current line

Reproduced. A one-line file is reported at 'baris 2', while the snippet shows line 1. The newline token's line has already been incremented. The parser behaves the same on every OS.

Reproduksi:

```text
coba { cetak(1) }
```

Keluaran 1.4.0:

```text
[baris 2, kolom 18] Kesalahan di akhir baris: Harapkan 'kecuali' atau 'akhirnya' setelah blok 'coba'.
    coba { cetak(1) }
                     ^
exit=65
```

### GALAT-16 — [rendah] Uncaught custom error whose pesan is not a teks prints only 'Kesalahan tak tertangani.'; no class name is ever shown

Reproduced. A non-teks pesan is dropped, and a subclass without pesan shows no class name either, so the user cannot tell which error escaped. Python prints 'G: 12345'. This is a diagnostics-quality problem.

Reproduksi:

```text
kelas G < Galat { init(p) { diri.pesan = p } }
naikkan G(12345)
```

Keluaran 1.4.0:

```text
Kesalahan tak tertangani.
[baris 2, kolom 16] dalam skrip
    naikkan G(12345)
                   ^
exit=70
```

### GALAT-17 — [rendah] Cascading duplicate "Harapkan '}' setelah blok" errors after the first compile error

Reproduced. After the real error at line 2, the identical EOF error is printed twice. The parser does not resynchronise well after the error, so it reports noise. This is OS-independent.

Reproduksi:

```text
kelas A {
    fungsi f(diri) { }
}
```

Keluaran 1.4.0:

```text
[baris 2, kolom 5] Kesalahan di 'fungsi': Harapkan nama metode.
[baris 4, kolom 1] Kesalahan di akhir: Harapkan '}' setelah blok.
[baris 4, kolom 1] Kesalahan di akhir: Harapkan '}' setelah blok.
exit=65
```

### GALAT-18 — [rendah] Float overflow in ** returns takhingga instead of an overflow error

Reproduced. 2.0 ** 5000 prints takhingga, while integer 2 ** 5000 correctly raises an overflow error. Python raises OverflowError for float pow overflow. The guide does not document this, so it is a minor deviation from Python semantics. pow() returns inf per IEEE, which is plausibly the same on every OS.

Reproduksi:

```text
cetak(2.0 ** 5000)
```

Keluaran 1.4.0:

```text
takhingga
exit=0
```

### GALAT-19 — [sedang] ✅ **Diperbaiki.** Kelas galat BAWAAN menolak pesan konstruktor, sehingga `GalatNilai("x")` gagal sedangkan subkelas buatan pengguna berhasil

Ditemukan saat menyusun `examples/fitur_lengkap.idk`. Semua kelas galat bawaan dibuat di C lewat `defineErrorClass()` (`src/vm.c`), yang hanya menyetel `klass->superclass` tanpa menyalin metode induk. Pewarisan di VM ini memakai **copy-down** (`OP_INHERIT` menyalin tabel metode induk ke subkelas), jadi kelas yang dibuat dari C tidak pernah memperoleh `init` — tabel metodenya kosong. `callValue()` kemudian jatuh ke cabang `argCount != 0` dan menolak argumen.

Akibatnya pustaka standar harus mendefinisikan ulang subkelasnya sendiri hanya untuk bisa membawa pesan (`kelas GalatBita < Galat { init(pesan) { diri.pesan = pesan } }` di `pustaka/bita.idk`, dan pola yang sama di `GalatDb`, `GalatJalur`, `GalatJaringan`, `GalatKoleksi`, dan lainnya) — 62 pemanggilan `naikkan XxX("pesan")` di pustaka bergantung pada pengulangan itu.

Yang menyamarkan bug ini: `naikkan GalatNilai("x")` **terlihat** berhasil ketika ditangkap `kecuali` tanpa `sebagai`, karena yang tertangkap sebenarnya adalah galat runtime "Harapkan 0 argumen tetapi mendapat 1", bukan `GalatNilai`. Pesan aslinya hilang diam-diam.

Reproduksi:

```text
variabel e = GalatNilai("pesan saya")
```

Keluaran 1.4.0:

```text
Harapkan 0 argumen tetapi mendapat 1.
exit=70
```

Bandingkan — subkelas buatan pengguna bekerja seperti yang diharapkan:

```text
kelas GalatUji < Galat { init(pesan) { diri.pesan = pesan } }
coba { naikkan GalatUji("rusak") } kecuali GalatUji sebagai e { cetak(e.pesan) }
```

```text
rusak
exit=0
```

Perbaikan yang disarankan: beri kelas galat bawaan sebuah `init` yang menerima pesan opsional (atau salin metode induk di `defineErrorClass`, menyamai perilaku `OP_INHERIT`), lalu hapus subkelas duplikat di pustaka.


## Modul pustaka standar

Pemeriksaan: **1770/1855 lulus**; temuan terkonfirmasi: **23**; ditolak verifikator: 1.

| Sub-area | Lulus |
|---|---|
| hash md5/sha1/sha256/sha512 vs hashlib (kosong, unicode, emoji, batas padding 55..129, 0..255 byte, galat input) | 88/88 |
| base64 vs Python base64 (enkode/enkode_url/dekode/bolak-balik/galat) | 92/96 |
| bita (ke_bita/ke_teks/ke_heksa/dari_heksa, UTF-8 tidak sah) | 72/78 |
| builtin matematika + trigonometri vs math (sin..atan2, radian/derajat, bulat/bulatkan/lantai/atap, bagi_sisa, biner/oktal/heksa, pangkat, akar, min/max, acak, nan/inf) | 218/226 |
| pustaka lama teks/daftar/matematika/kamus (impor "pustaka/x.idk") | 58/60 |
| json (serialisasi, urai, escape unicode, bersarang, JSON tidak sah, bolak-balik, 3000 rekaman) | 51/68 |
| statistik vs Python statistics | 95/118 |
| iterasi vs itertools + fungsional vs functools | 60/64 |
| koleksi (Penghitung/KamusBawaan/Antrian) + salin (dangkal/dalam/siklus) | 46/50 |
| jalur vs posixpath (38 jalur x 7 fungsi, gabung) + fungsi berkas (ada/ukuran/waktu_ubah/absolut) | 298/298 |
| pola vs re (re.ASCII): 57 pola x 8 operasi, bendera, penolakan fitur backtracking, templat ganti, Cocokan, ReDoS | 519/520 |
| opsi vs argparse (urai daftar + CLI -h/exit 2) | 39/41 |
| csv vs Python csv (urai/bentuk/urai_kamus/bentuk_kamus, kutip, CRLF, pemisah) | 45/52 |
| log (ambang, format, berkas, hierarki pencatat) | 15/15 |
| proses (jalankan/keluaran, stdin, kode keluar, argumen ber-spasi/kutip/&, tanpa shell, 1.2 MB keluaran) | 14/14 |
| kura (SVG, posisi/arah vs turtle, lingkaran/busur, galat) | 25/25 |
| pembaruan (urai_versi/banding_versi/saran/pesan, tanpa jaringan) | 15/15 |
| gaya impor (impor x, sebagai, dari x impor a, b, impor "pustaka/x.idk", dari cwd lain, modul tak ada) | 14/15 |
| kinerja pustaka pada data 20k-60k karakter | 6/12 |

### PUSTAKA-01 — [tinggi] impor "berkas.idk" menulis global ke tabel builtin bersama sehingga mencemari semua modul bernamespace (termasuk pustaka standar)

Direproduksi. Kontrol: fungsi panjang() yang didefinisikan langsung di skrip TIDAK bocor ke modul mm (mm.f() tetap 3), tetapi definisi yang sama lewat impor "bajak.idk" membuat mm.f() mencetak DIBAJAK; 'panjang = 5' lewat impor berkas membuat mm.f() gagal 'Hanya dapat memanggil fungsi dan kelas.'. impor "pustaka/base64.idk" lalu impor json merusak json.json_teks(5) karena 'dari bita impor ke_teks' di base64.idk menimpa builtin ke_teks. 03 §Modul menyebut modul bernamespace punya namespace sendiri dan 'Modul yang diimpor tidak mencemari variabel Anda'. Logika VM (bukan OS).

Reproduksi:

```text
# m/bajak.idk
fungsi panjang(x) {
    kembalikan "DIBAJAK"
}
# m/mm.idk
fungsi f() {
    kembalikan panjang("abc")
}
# m/v01.idk (jalankan dari folder m)
impor "bajak.idk"
impor mm
cetak(mm.f())
```

Keluaran 1.4.0:

```text
DIBAJAK (exit 0); varian v01b: impor "pustaka/base64.idk" / impor json / cetak(json.json_teks(5)) -> 'panjang() hanya menerima string, ...' [baris 48] dalam ke_teks() [baris 50] dalam json_teks(), exit 70
```

### PUSTAKA-02 — [tinggi] ✅ **Diperbaiki (terverifikasi ulang 1.4.0).** Argumen kata kunci di dalam fungsi membuat argumen posisi berupa global/upvalue menjadi kosong

Direproduksi. g(D, b = 2) di level skrip -> [7, 2], di dalam fungsi -> [kosong, 2]; dengan upvalue -> [kosong, 1]. Cocok dengan hipotesis prescanFunctionBody membaca 'D, b =' sebagai pembongkaran 'a, b = ...' sehingga D jadi lokal. Mengenai statistik.kuantil(DATA, n = 2) dan csv.urai(t, pemisah = ";") di dalam closure. Bug kompilator, tidak bergantung OS.

Reproduksi:

```text
fungsi g(a, b = 0) {
    kembalikan [a, b]
}
D = 7
cetak(g(D, b = 2))
fungsi f() {
    kembalikan g(D, b = 2)
}
cetak(f())
```

Keluaran 1.4.0:

```text
[7, 2]
[kosong, 2]
```

### PUSTAKA-03 — [tinggi] ✅ **Diperbaiki (terverifikasi ulang 1.4.0).** json_urai tidak mendekode \uXXXX, \b, \f, dan menerima escape tidak sah

Direproduksi: "é" -> "u00e9", "\b\f" -> "bf", "a\qb" -> "aqb" tanpa galat. 07 §2.5 menjanjikan parsing JSON; RFC 8259 mewajibkan \uXXXX dan menolak escape lain. Data rusak diam-diam.

Reproduksi:

```text
impor json
cetak(repr(json.json_urai("\"\\u00e9\"")))
cetak(repr(json.json_urai("\"\\b\\f\"")))
cetak(repr(json.json_urai("\"a\\qb\"")))
```

Keluaran 1.4.0:

```text
"u00e9"
"bf"
"aqb" (exit 0)
```

### PUSTAKA-04 — [tinggi] bulat(), lantai(), atap() pada nan mengembalikan -9223372036854775808 diam-diam

Direproduksi. Inkonsisten dengan bulat(inf) yang sudah menaikkan 'melebihi jangkauan bilangan bulat'; 07 §1.1 menyamakan dengan math.floor/ceil yang menaikkan ValueError untuk NaN. Hasil konversi double NaN->int64 tidak terdefinisi di C, jadi nilai persisnya bisa berbeda per platform, tapi bugnya (tidak ada pemeriksaan NaN) tidak bergantung OS.

Reproduksi:

```text
n = ke_angka("nan")
cetak(bulat(n), lantai(n), atap(n))
```

Keluaran 1.4.0:

```text
-9223372036854775808 -9223372036854775808 -9223372036854775808 (exit 0)
```

### PUSTAKA-05 — [tinggi] ✅ **Diperbaiki (terverifikasi ulang 1.4.0).** bita.ke_teks menerima UTF-8 overlong (C0 80 dan E0 80 80 menjadi NUL)

Direproduksi: [0xC0,0x80] dan [0xE0,0x80,0x80] menghasilkan teks 1 karakter dengan ord 0, tanpa GalatBita. 07 §2.18: ke_teks menaikkan GalatBita bila urutannya tidak sah; overlong tidak sah menurut UTF-8 (RFC 3629).

Reproduksi:

```text
dari bita impor ke_teks
t = ke_teks([0xC0, 0x80])
cetak(panjang(t), ord(t))
```

Keluaran 1.4.0:

```text
1 0 (tanpa galat, exit 0)
```

### PUSTAKA-06 — [sedang] statistik.kuantil memotong titik di luar data alih-alih mengekstrapolasi (metode exclusive)

Direproduksi: kuantil([1,2,3,4], n = 10) -> [1, 1.0, 1.5, ..., 4, 4], sedangkan statistics.quantiles (exclusive) memberi [0.5, 1.0, ..., 4.5]. 07 §2.8 eksplisit menyebut 'metode exclusive seperti statistics.quantiles'. Kasus default (n=4, 4 data) benar. Sub-klaim kuantil([5]) BUKAN bug: 07 §2.8 mendokumentasikan GalatStatistik untuk data kurang dari dua. Severity diturunkan ke sedang karena hanya muncul bila n > panjang data.

Reproduksi:

```text
impor statistik
cetak(statistik.kuantil([1, 2, 3, 4], n = 10))
```

Keluaran 1.4.0:

```text
[1, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4, 4]
```

### PUSTAKA-07 — [sedang] bita.ke_teks: surrogate, > U+10FFFF, byte negatif memberi galat chr() yang tidak tertangkap GalatBita; 0xFF dilaporkan 'terpotong'

Direproduksi: [ED A0 80] -> 'chr() kode di luar jangkauan Unicode (55296)' yang lolos dari 'kecuali GalatBita' (exit 70); [F4 90 80 80] -> (1114112); [-1] -> (-1); [255] dan [256] -> GalatBita 'urutan UTF-8 terpotong' (pesan menyesatkan). Melanggar kontrak GalatBita di 07 §2.18.

Reproduksi:

```text
dari bita impor ke_teks, GalatBita
coba {
    cetak(ke_teks([0xED, 0xA0, 0x80]))
} kecuali GalatBita sebagai e {
    cetak("GalatBita", e.pesan)
}
```

Keluaran 1.4.0:

```text
chr() kode di luar jangkauan Unicode (55296)
[baris 6, kolom 1] dalam skrip ; exit 70
```

### PUSTAKA-08 — [sedang] json_teks menghasilkan JSON tidak sah: karakter kontrol mentah, inf/nan jadi takhingga/nan, kunci bukan teks crash

Direproduksi: json_teks(chr(1)) menulis byte 0x01 mentah; [inf, nan] -> [takhingga,nan] (bukan JSON); json_teks({1: "satu"}) crash dengan pesan internal 'panjang() hanya menerima string...' dari json_escape. Sub-klaim tupel BUKAN bug: 07 §2.5 hanya mencantumkan kamus/list/angka/string/boolean/kosong, dan pesan 'tipe tidak didukung: tupel' jelas.

Reproduksi:

```text
impor json
cetak(repr(json.json_teks(chr(1))))
cetak(json.json_teks([ke_angka("inf"), ke_angka("nan")]))
cetak(json.json_teks({1: "satu"}))
```

Keluaran 1.4.0:

```text
"\"<0x01 mentah>\""
[takhingga,nan]
panjang() hanya menerima string, list, ... [baris 18] dalam json_escape() ; exit 70
```

### PUSTAKA-09 — [sedang] json_urai menerima JSON tidak sah: data sisa di akhir dan angka cacat

Direproduksi: "1 2"->1, "[1]x"->[1], "truex"->benar, "-"->0, "01"->1, "1."->1.0, semuanya tanpa galat. 07 §2.5 menjanjikan kesalahan yang dapat ditangkap; json.loads menolak semuanya.

Reproduksi:

```text
impor json
untuk s dalam ["1 2", "[1]x", "truex", "-", "01", "1."] {
    coba { cetak(s, "->", repr(json.json_urai(s))) } kecuali e { cetak(s, "-> galat", e) }
}
```

Keluaran 1.4.0:

```text
1 2 -> 1
[1]x -> [1]
truex -> benar
- -> 0
01 -> 1
1. -> 1.0
```

### PUSTAKA-10 — [sedang] Pustaka yang membangun teks per karakter berjalan kuadratik

Direproduksi (resolusi waktu() 1 detik): base64.enkode 20k byte ~1 s, 40k byte ~3 s (2x input -> ~3x waktu); x = x + "a" 40.000 kali ~1 s. Pola akumulasi string sudah diperlakukan sebagai bug pada commit e6f9db5. Angka absolut di mesin ini lebih kecil dari laporan, tapi pertumbuhan superlinear tetap terlihat. Bukan hal khusus Windows.

Reproduksi:

```text
impor base64
d = [65] * 20000
mulai = waktu()
s = base64.enkode(d)
cetak("20k", waktu() - mulai)
d = [65] * 40000
mulai = waktu()
s = base64.enkode(d)
cetak("40k", waktu() - mulai)
```

Keluaran 1.4.0:

```text
20k 1.0
40k 3.0
```

### PUSTAKA-11 — [sedang] csv.urai membuang CR tanpa kutip: baris berakhiran \r tergabung dan 'a\rb' jadi 'ab'

Direproduksi: "a,b\rc,d\r" -> [["a", "bc", "d"]]; "a\rb,c" -> [["ab", "c"]]. \r\n ditangani dengan benar. 07 §2.20 menjanjikan RFC 4180 dan perilaku csv Python, yang memperlakukan \r tunggal sebagai akhir baris. Data rusak diam-diam (mis. berkas berakhiran CR gaya Mac lama).

Reproduksi:

```text
impor csv
cetak(csv.urai("a,b\rc,d\r"))
cetak(csv.urai("a\rb,c"))
```

Keluaran 1.4.0:

```text
[["a", "bc", "d"]]
[["ab", "c"]]
```

### PUSTAKA-12 — [sedang] csv.urai_kamus membuat rekaman palsu dari baris kosong

Direproduksi: "a,b\n\n1,2\n" -> [{"a": "", "b": ""}, {"a": "1", "b": "2"}]; DictReader melewati baris kosong. Baris pendek diisi "" dan kolom lebih dibuang diam-diam (Python: None / kunci None). Menyimpang dari csv.DictReader yang dirujuk 07 §2.20.

Reproduksi:

```text
impor csv
cetak(csv.urai_kamus("a,b\n\n1,2\n"))
```

Keluaran 1.4.0:

```text
[{"a": "", "b": ""}, {"a": "1", "b": "2"}]
```

### PUSTAKA-13 — [sedang] opsi: nilai bulat yang meluap crash (exit 70) alih-alih galat pemakaian exit 2 / GalatOpsi

Direproduksi: "abc" menghasilkan GalatOpsi dengan benar, tetapi "99999999999999999999" menghasilkan galat ke_angka mentah (bukan GalatOpsi) saat keluar_saat_galat = salah, dan traceback exit 70 dalam mode bawaan. 07 §2.15 menjanjikan pemakaian + exit 2 / GalatOpsi.

Reproduksi:

```text
dari opsi impor PenguraiArgumen
p = PenguraiArgumen("prog")
p.tambah_argumen("--jumlah", pendek = "-j", bawaan = 1, jenis_nilai = "bulat")
cetak(p.urai(["-j", "99999999999999999999"]))
```

Keluaran 1.4.0:

```text
ke_angka() melebihi jangkauan bilangan bulat
[baris 312, kolom 36] dalam _ubah()
[baris 443, kolom 48] dalam urai() ; exit 70
```

### PUSTAKA-14 — [sedang] bulatkan(inf/nan) mengembalikan takhingga/nan desimal, tidak konsisten dengan bulat(inf)

Direproduksi: bulatkan(inf), bulatkan(nan) -> takhingga nan, padahal bulat(inf) menaikkan galat. 07 §1.1 menyebut bulatkan 'persis seperti round() di Python' dan 'tanpa digit hasilnya bulat'; Python round(inf) menaikkan OverflowError, round(nan) menaikkan ValueError.

Reproduksi:

```text
inf = ke_angka("inf")
nan = ke_angka("nan")
cetak(bulatkan(inf), bulatkan(nan))
cetak(bulat(inf))
```

Keluaran 1.4.0:

```text
takhingga nan
bulat() melebihi jangkauan bilangan bulat (exit 70)
```

### PUSTAKA-15 — [sedang] pangkat(0, -1) mengembalikan takhingga padahal 0 ** -1 menaikkan galat

Direproduksi: pangkat(0, -1) -> takhingga; 0 ** -1 dan 0.0 ** -1 -> '0 tidak dapat dipangkatkan dengan bilangan negatif.'. 07 §1.1 menyebut pangkat 'seperti operator **'. Sub-klaim pangkat(-8, 1/3) -> nan lemah: operator (-8) ** (1/3) juga memberi nan, jadi keduanya konsisten di dalam bahasa ini.

Reproduksi:

```text
cetak(pangkat(0, -1))
cetak(0 ** -1)
```

Keluaran 1.4.0:

```text
takhingga
0 tidak dapat dipangkatkan dengan bilangan negatif. (exit 70)
```

### PUSTAKA-16 — [rendah] base64.dekode menerima input tanpa padding yang benar

Direproduksi: "SG"->[72], "SGF"->[72, 97], "SGFsbw" dan "SGFsbw="->[72, 97, 108, 111]. Panjang 1 ("S") memang ditolak, jadi pemeriksaan panjang ada tetapi lebih longgar dari b64decode yang dirujuk 07 §2.19 (Incorrect padding).

Reproduksi:

```text
impor base64
cetak(base64.dekode("SG"))
cetak(base64.dekode("SGFsbw="))
```

Keluaran 1.4.0:

```text
[72]
[72, 97, 108, 111]
```

### PUSTAKA-17 — [rendah] iterasi.iris(urutan, x) dengan satu batas memperlakukannya sebagai awal, bukan akhir seperti islice

Direproduksi: iris("ABCDEFG", 2) -> ["C","D","E","F","G"]. Signature di sumbernya iris(urutan, mulai = 0, akhir = kosong, ...), jadi ini disengaja, tetapi 07 §2.9 menyebut modul ini 'padanan itertools' dan hanya mendokumentasikan penyesuaian untuk rangkai/produk/peta_bintang, tidak untuk iris. Penyimpangan ini tidak dicatat. Sub-klaim putar_ulang([], 5) yang menaikkan galat dan kombinasi(.., -1) -> [] hanya deviasi minor.

Reproduksi:

```text
impor iterasi
cetak(iterasi.iris("ABCDEFG", 2))
```

Keluaran 1.4.0:

```text
["C", "D", "E", "F", "G"]
```

### PUSTAKA-18 — [rendah] statistik kurang presisi dibanding statistics (aritmetika eksak)

Direproduksi: rata_rata([0.1]*10) = 0.09999999999999999, varians = 2.139922160430262e-34 (Python: 0.1 dan 0.0). Disebabkan penjumlahan float naif. 07 §2.8 menyebut 'seperti statistics.variance'. Dampaknya kecil.

Reproduksi:

```text
impor statistik
cetak(statistik.rata_rata([0.1] * 10), statistik.varians([0.1] * 10))
```

Keluaran 1.4.0:

```text
0.09999999999999999 2.139922160430262e-34
```

### PUSTAKA-19 — [rendah] MAT_PI dan MAT_E dipotong ke 15 digit

Direproduksi. Sumbernya pustaka/matematika.idk baris 5-6 berisi literal MAT_PI = 3.14159265358979 dan MAT_E = 2.71828182845905, padahal 07 §2.3 menyebutnya 'Konstanta π dan e' (kehilangan sekitar 3e-15).

Reproduksi:

```text
impor "pustaka/matematika.idk"
cetak(MAT_PI, MAT_E)
```

Keluaran 1.4.0:

```text
3.14159265358979 2.71828182845905
```

### PUSTAKA-20 — [rendah] daftar_iris dengan awal negatif membungkus indeks dan menggandakan elemen

Direproduksi: daftar_iris([1,2,3,4], -2, 4) -> [3, 4, 1, 2, 3, 4]. Hasil mana pun yang masuk akal (galat, [3, 4], atau []) tidak akan menggandakan elemen; ini jelas salah untuk 'sublist [mulai, akhir)' di 07 §2.2.

Reproduksi:

```text
impor "pustaka/daftar.idk"
cetak(daftar_iris([1, 2, 3, 4], -2, 4))
```

Keluaran 1.4.0:

```text
[3, 4, 1, 2, 3, 4]
```

### PUSTAKA-21 — [rendah] -0.0 dicetak sebagai 0.0 (cetak, repr)

Direproduksi: cetak(-0.0), repr(-0.0) -> 0.0 0.0, padahal atan2(-0.0, -1) = -3.141592653589793 membuktikan tanda tersimpan. Dokumentasi tidak membahasnya, tetapi bahasa ini menjanjikan semantik Python (repr(-0.0) == '-0.0'). Masalah tampilan saja.

Reproduksi:

```text
cetak(-0.0, repr(-0.0), atan2(-0.0, -1))
```

Keluaran 1.4.0:

```text
0.0 0.0 -3.141592653589793
```

### PUSTAKA-22 — [rendah] max([1, 1.0]) mengembalikan elemen setara terakhir, bukan yang pertama

Direproduksi: max([1, 1.0]) dan max(1, 1.0) -> 1.0; min([1.0, 1]) -> 1.0 (benar, elemen pertama). Python max mengembalikan maksimum pertama (1). Perbedaannya hanya pada tipe bulat/desimal dari nilai yang setara.

Reproduksi:

```text
cetak(max([1, 1.0]), max(1, 1.0))
```

Keluaran 1.4.0:

```text
1.0 1.0
```

### PUSTAKA-23 — [rendah] csv.urai menerima pemisah kosong atau lebih dari satu karakter tanpa galat; '""' menjadi []

Direproduksi: pemisah="" dan pemisah=",," -> [["a,b"]] diam-diam (Python menaikkan TypeError). csv.urai("\"\"") -> [] padahal csv.reader memberi [['']]. Baris '""' bukan baris kosong, jadi aturan 'baris kosong menjadi []' di 07 tidak berlaku.

Reproduksi:

```text
impor csv
cetak(csv.urai("a,b", pemisah=""))
cetak(csv.urai("a,b", pemisah=",,"))
cetak(csv.urai("\"\""))
```

Keluaran 1.4.0:

```text
[["a,b"]]
[["a,b"]]
[]
```

- *Ditolak (salah_uji):* Penghitung(kamus) menghitung kunci; 'dalam' tidak bisa dipakai pada Penghitung/KamusBawaan — Perilakunya memang terulang (Penghitung({"a": 3, "b": 1}).paling_umum() -> [["a", 1], ["b", 1]]; 'dalam' -> 'Operand kanan dalam harus daftar, ...'), tetapi bukan kontrak yang didokumentasikan. 07 §2.7 mendokumentasikan Penghitung(urutan=kosong) dengan argumen urutan, bukan pemetaan. Daftar API Penghitung/KamusBawaan tidak menyebut dukungan 'dalam', dan bahasa ini tidak punya metode ajaib untuk 'd

## Integrasi Windows: berkas, jalur, proses, soket

Pemeriksaan: **284/312 lulus**; temuan terkonfirmasi: **13**.

| Sub-area | Lulus |
|---|---|
| I/O berkas dasar (buka/baca/tulis/sambung/baca_semua/tulis_semua/tambah_berkas, CRLF, ^Z, NUL, UTF-8, kosong) | 27/31 |
| Galat berkas & izin (C:\Windows, read-only, terkunci, mode salah, handle tertutup) | 26/36 |
| Jalur Windows (\ dan /, drive, ..\, spasi, huruf besar-kecil, CON/NUL/AUX, >260 karakter) | 20/24 |
| Direktori (buat/hapus/isi_direktori terurut, direktori_kini, modul jalur) | 30/30 |
| Berkas besar 50 MB + 100k baris + append paralel | 9/9 |
| Lingkungan (lingkungan/ambil_lingkungan, Windows env) | 13/13 |
| proses.jalankan (jalur_indk, cmd.exe /c, .bat, kode keluar, stdin, 1 MB stdout+stderr, CRLF) | 46/48 |
| --sandbox | 17/17 |
| Soket TCP (200 permintaan 2 proses, ditolak, port, 200 KB, tutup, peer reset) | 29/32 |
| HTTP: todo_server (201 permintaan http_ambil) + pustaka web | 11/14 |
| waktu/tanggal/format_tanggal/henti/jam | 15/20 |
| Shell: kode keluar di cmd.exe, PowerShell 5.1, Git Bash; argumen spasi; pipa | 38/38 |

### WINDOWS-OS-01 — [tinggi · khas Windows] Temuan keamanan (rincian dilaporkan secara privat)

Rincian dan cara reproduksi sengaja tidak dicantumkan sampai diperbaiki (lihat `docs/ATURAN_BRANCH_DAN_PR.md`, bagian keamanan).

### WINDOWS-OS-02 — [tinggi · khas Windows] Temuan keamanan (rincian dilaporkan secara privat)

Rincian dan cara reproduksi sengaja tidak dicantumkan sampai diperbaiki (lihat `docs/ATURAN_BRANCH_DAN_PR.md`, bagian keamanan).

### WINDOWS-OS-03 — [sedang · khas Windows] ada_berkas() mengembalikan salah untuk berkas yang ada tetapi sedang dikunci proses lain

Direproduksi: tanpa kunci hasilnya benar. Saat berkas dibuka PowerShell dengan FileShare None hasilnya salah, sementara jalur.ada() dan jalur.adalah_berkas() tetap memberi benar benar. Penyebabnya, src/native.c nativeFnAdaBerkas memakai fopen(path,"rb") alih-alih stat/GetFileAttributes. Dokumen 1.6 berbunyi 'Apakah berkas ada'. Pelanggaran berbagi (sharing violation) khas Windows; di POSIX kesalahan yang sama hanya muncul untuk berkas tanpa izin baca (chmod 000). Tingkat diturunkan ke sedang karena ini negatif palsu pada kondisi khusus, bukan kerusakan data.

Reproduksi:

```text
cetak(ada_berkas("kunci.txt"))   # jalankan saat kunci.txt dibuka [IO.File]::Open(...,'Open','ReadWrite','None') di PowerShell
```

Keluaran 1.4.0:

```text
benar (tanpa kunci) / salah (saat dikunci); jalur.ada + jalur.adalah_berkas: benar benar
```

### WINDOWS-OS-04 — [sedang · khas Windows] Jalur panjang (>260 karakter) tidak didukung: ada_berkas salah, baca_semua gagal, isi_direktori 'tidak ada' untuk berkas yang ada

Direproduksi dengan LongPathsEnabled=1 dan jalur absolut 424 karakter yang dibuat PowerShell. ada_berkas memberi salah, baca_semua gagal membuka, dan isi_direktori melaporkan 'tidak ada'. indk.exe tampaknya tidak punya manifest longPathAware dan tidak memakai awalan \\?\. Batas MAX_PATH khas Windows. Tingkat diturunkan ke sedang karena jalur sepanjang ini jarang dipakai dan tidak ada janji eksplisit di dokumen.

Reproduksi:

```text
# lp/ddd...(50)/ x5 /f.txt ada (jalur absolut 424 karakter)
cetak(ada_berkas("lp/dd...d/dd...d/dd...d/dd...d/dd...d/f.txt"))
coba { cetak(isi_direktori("lp/dd...d/dd...d/dd...d/dd...d/dd...d")) } kecuali e { cetak("G", e) }
```

Keluaran 1.4.0:

```text
salah
G baca_semua() gagal membuka 'lp/ddd.../f.txt'
G isi_direktori(): 'lp/ddd...' tidak ada
```

### WINDOWS-OS-05 — [sedang] Galat I/O berkas (baca_semua/buka_berkas/hapus_berkas) adalah GalatNilai, bukan GalatOS, dan tanpa alasan

Direproduksi: baca_semua, hapus_berkas, dan buka_berkas pada berkas yang tidak ada tidak tertangkap oleh 'kecuali GalatOS'. Galatnya GalatNilai dengan pesan generik 'gagal membuka/menghapus' tanpa alasan. Sebaliknya isi_direktori memberi GalatOS "... tidak ada". Dokumen menyatakan kesalahan sistem operasi adalah GalatOS seperti OSError Python, dan Python memberi FileNotFoundError, turunan OSError. Ini kode C lintas OS (fopen tanpa errno), jadi tidak khas Windows.

Reproduksi:

```text
coba { baca_semua("tidak_ada_xyz.txt") } kecuali GalatOS sebagai e { cetak("OS", e.pesan) } kecuali e { cetak("lain", adalah_instansi(e, GalatNilai), e) }
```

Keluaran 1.4.0:

```text
lain benar baca_semua() gagal membuka 'tidak_ada_xyz.txt'
```

### WINDOWS-OS-06 — [sedang] web: respons 413 untuk badan > 1 MB tidak sampai ke klien http_ambil; klien mendapat reset koneksi

Direproduksi dengan server buatan sendiri (web.Aplikasi + baca_permintaan + tangani + soket_tutup). http_ambil yang mengirim badan 1,26 MB dua kali mendapat 'soket_baca() gagal membaca dari soket', bukan status 413. Klien Python mentah kadang masih menerima 413 karena waktunya berbeda (race). Server menutup soket dengan data masuk yang belum dibaca, sehingga TCP mengirim RST yang membuang respons. Perilaku ini juga terjadi di Linux, jadi tidak khas Windows. Dokumen 2.6 menjanjikan jawaban 413.

Reproduksi:

```text
# server: impor web; app=web.Aplikasi(); app.rute("POST","/g",fungsi(req){kembalikan web.teks(200,"ok")}); srv=soket_dengar(18981); k=soket_terima(srv); soket_tulis(k, app.tangani(web.baca_permintaan(k))); soket_tutup(k)
impor "pustaka/http.idk"
b = "q" * 60000
b2 = b
untuk i dalam rentang(20) { b2 = b2 + b }
coba { cetak(http_ambil("http://127.0.0.1:18981/g", "POST", {"Content-Type": "text/plain"}, b2).status) } kecuali e { cetak("GALAT", e) }
```

Keluaran 1.4.0:

```text
GALAT soket_baca() gagal membaca dari soket
```

### WINDOWS-OS-07 — [sedang] web.Aplikasi.tangani: penangan yang 'naikkan "teks"' atau 'naikkan KelasGalat' lolos dari tangani, bukan 500

Direproduksi dengan binari terpasang: /i (naikkan GalatNilai()) memberi 500. /t (naikkan "rusak") dan /k (naikkan GalatNilai) LOLOS dari tangani(), sehingga di server nyata menjadi galat tak tertangani. 'naikkan "teks"' adalah sintaks yang didokumentasikan (03_PANDUAN_SINTAKS baris 2505). Dokumen 2.6 menjanjikan 'Galat dari penangan -> 500'. Penyebabnya pustaka/web.idk memakai 'kecuali Galat sebagai e', padahal komentar di berkas yang sama mengakui ada galat berupa teks mentah. Penangan yang mengembalikan kosong juga membuat tangani() mengembalikan kosong. Logika ini ada di pustaka, jadi lintas OS.

Reproduksi:

```text
impor web
app = web.Aplikasi()
app.rute("GET", "/t", fungsi(req) { naikkan "rusak" })
coba {
    cetak(repr(app.tangani("GET /t HTTP/1.0\r\nHost: x\r\n\r\n"))[0:30])
} kecuali e {
    cetak("LOLOS", e)
}
```

Keluaran 1.4.0:

```text
LOLOS rusak   (/i -> "HTTP/1.1 500 Internal Server ; /k -> LOLOS <kelas GalatNilai>; /n -> kosong)
```

### WINDOWS-OS-08 — [sedang] waktu() hanya beresolusi 1 detik; selisih setelah henti(0.3) adalah 0.0

Direproduksi: 1793702373.0 dua kali, selisih 0.0. src/native.c memakai NUMBER_VAL((double)time(NULL)), jadi resolusinya 1 detik di semua OS. Python time.time() memberi pecahan detik, dan pengukuran durasi dengan waktu() menjadi tidak berguna.

Reproduksi:

```text
a = waktu()
henti(0.3)
b = waktu()
cetak(repr(a), repr(b), b - a)
```

Keluaran 1.4.0:

```text
1793702373.0 1793702373.0 0.0
```

### WINDOWS-OS-09 — [rendah] input() di akhir stdin mengembalikan kosong, bukan galat EOF

Direproduksi: printf 'a' | indk memberi "a" lalu kosong, exit 0, tanpa galat. Python menaikkan EOFError, dan semantik Python adalah acuan bahasa ini. Namun dokumen 1.8 tidak menyebut perilaku EOF sama sekali, dan baca_baris memakai sentinel kosong yang serupa. Jadi ini lebih ke penyimpangan yang tidak terdokumentasi daripada kerusakan. Tingkat diturunkan ke rendah.

Reproduksi:

```text
cetak(repr(input()))
coba { cetak(repr(input())) } kecuali e { cetak("galat", e) }
```

Keluaran 1.4.0:

```text
"a"
kosong
```

### WINDOWS-OS-10 — [rendah · khas Windows] jam() di Windows menghitung waktu dinding, bukan waktu CPU

Direproduksi: selisih jam() selama henti(0.5) adalah 0.503. src/vm.c:77 memakai clock()/CLOCKS_PER_SEC. clock() di MSVCRT/UCRT mengukur waktu dinding, sedangkan di POSIX mengukur waktu CPU. Dokumen 1.7 berbunyi 'Waktu CPU berjalan'.

Reproduksi:

```text
j = jam()
henti(0.5)
cetak("jam", jam() - j)
```

Keluaran 1.4.0:

```text
jam 0.5030000000000001
```

### WINDOWS-OS-11 — [rendah] tanggal() memberi angka desimal (2026.0); format_tanggal dengan pola tak sah diam-diam memberi ""

Direproduksi: tanggal()["tahun"] adalah 2026.0 dan ke_bulat(t) adalah t memberi salah, padahal K2 membedakan bulat dan desimal (lintas OS). format_tanggal("%Y %Q %m") memberi "" sementara "%Y %m" memberi "2026 11". Bagian format_tanggal kemungkinan khas CRT Windows: strftime UCRT gagal pada spesifier tak dikenal, sedangkan glibc meneruskannya apa adanya. Pola tak sah jarang dipakai, jadi tetap rendah.

Reproduksi:

```text
t = tanggal()["tahun"]
cetak(repr(t), ke_bulat(t) adalah t)
cetak(repr(format_tanggal("%Y %Q %m")), repr(format_tanggal("%Y %m")))
```

Keluaran 1.4.0:

```text
2026.0 salah
"" "2026 11"
```

### WINDOWS-OS-12 — [rendah] soket_tulis mengembalikan desimal (3.0); soket_dengar menerima port desimal 18979.7

Direproduksi: soket_tulis(k, "abc") memberi 3.0, sedangkan tulis() memberi bulat 3. soket_dengar(18979.7) diterima dan mengembalikan <soket> karena port dipotong diam-diam. Ini ketidakkonsistenan K2 di native.c yang lintas OS.

Reproduksi:

```text
srv = soket_dengar(18978)
k = soket_sambung("127.0.0.1", 18978)
t = soket_terima(srv)
cetak(repr(soket_tulis(k, "abc")))
soket_tutup(t)
soket_tutup(k)
soket_tutup(srv)
cetak("desimal:", soket_dengar(18979.7))
```

Keluaran 1.4.0:

```text
3.0
desimal: <soket>
```

### WINDOWS-OS-13 — [rendah] baca()/baca_semua menyimpan \r\n sementara baca_baris membuangnya; baca() pada handle tulis diam-diam memberi ""

Direproduksi: baca_semua dan baca memberi "a\r\nb\n", baca_baris memberi "a" "b" kosong, dan baca() pada handle mode "t" memberi "" tanpa galat. Pembuangan baris baru oleh baca_baris dan pembacaan biner oleh baca/baca_semua bisa dianggap pilihan desain, karena dokumen 1.6 tidak menjanjikan translasi baris baru. Namun baca() pada handle tulis yang diam-diam berhasil adalah bug nyata kecil (Python: UnsupportedOperation). Perilaku ini lintas OS, karena berkas berisi \r\n juga ada di Linux.

Reproduksi:

```text
tulis_semua("vb.txt", "a\r\nb\n")
cetak(repr(baca_semua("vb.txt")))
f = buka_berkas("vb.txt", "b")
cetak(repr(baca_baris(f)), repr(baca_baris(f)), repr(baca_baris(f)))
tutup(f)
w = buka_berkas("vb.txt", "t")
cetak(repr(baca(w)))
tutup(w)
```

Keluaran 1.4.0:

```text
"a\r\nb\n"
"a" "b" kosong
""
```


## CLI, REPL, perkakas, SQLite

Pemeriksaan: **241/269 lulus**; temuan terkonfirmasi: **15**.

| Sub-area | Lulus |
|---|---|
| CLI flags & usage (-v/-h/--help/unknown flags/no-arg subcommands/exit codes) | 23/24 |
| Input file robustness (empty, directory, garbage, NUL, long lines, deep nesting) | 19/30 |
| REPL (stdin, multi-line blocks, errors do not exit, CRLF, Ctrl-Z, keluar, long lines) | 20/24 |
| input() and argumen (EOF, quotes, empty, unicode, via proses) | 15/18 |
| --dis | 9/9 |
| indk periksa | 13/14 |
| indk uji | 14/14 |
| indk rapikan (--periksa/--tulis, idempotence, comments, bytecode equality on all 140 examples) | 17/20 |
| --periksa-pembaruan | 3/4 |
| --sandbox | 12/12 |
| SQLite db_* (CRUD, params, transactions, BLOB, batas_tunggu, errors, file DB, 2-process concurrency, injection) | 96/100 |

### CLI-ALAT-01 — [tinggi] Compiler stack overflow crash on deep nesting (32 nested fungsi, ~2500 nested parens, ~4000 nested blocks)

Reproduced independently: 31 nested fungsi runs (exit 0), 32 nested fungsi, 2500 nested parentheses and 4000 nested jika-blocks all die silently with exit -1073741571 (0xC00000FD STATUS_STACK_OVERFLOW); --dis on the 32-level file crashes the same way. No compile error is reported. The very low threshold for fungsi (32) points to a large per-Compiler struct on the C stack; Windows' 1 MB main-thread stack makes it hit early, while Linux's 8 MB stack would only raise the threshold. The recursion is unbounded on every OS, so the bug is not Windows-specific.

Reproduksi:

```text
# generated: 32 levels of
fungsi f0() {
fungsi f1() {
... (32 nested)
kembalikan 2
}
... (32 closing braces)
# or: x = ((((...1...)))) with 2500 parentheses
```

Keluaran 1.4.0:

```text
(no output) exit -1073741571; same for k2500.idk, b4000.idk, --dis f32.idk
```

### CLI-ALAT-02 — [tinggi] input() truncates lines at 1023 bytes and returns the rest on the next call

With input 'x'*5000 + 'yyy', the first input() returns 1023 characters and the second returns more 'x' instead of 'yyy'. The docs (1.8) say input() reads one line from stdin, and Python reads the whole line. The cause looks like a fixed fgets buffer, which behaves the same on every OS. Data is corrupted silently.

Reproduksi:

```text
a = input()
b = input()
cetak(panjang(a))
cetak(panjang(b), repr(b)[:10])
# python -c "print('x'*5000); print('yyy')" | indk inp.idk
```

Keluaran 1.4.0:

```text
1023
1023 "xxxxxxxxx
```

### CLI-ALAT-03 — [tinggi] rapikan --tulis truncates the file at a NUL byte (data loss); running ignores code after NUL

The file bytes 'cetak(1)\0cetak(2)\ncetak(3)\n' after rapikan --tulis become only 'cetak(1)\n' with exit 0, so user code is lost without any warning. Running the file prints only 1 with exit 0. The source is treated as a C string (strlen), which happens on every OS. Python rejects null bytes in source. The NUL input is unusual, but the silent destructive rewrite earns tinggi.

Reproduksi:

```text
printf 'cetak(1)\0cetak(2)\ncetak(3)\n' > nul2.idk
indk rapikan --tulis nul2.idk
```

Keluaran 1.4.0:

```text
exit=0; file content afterwards: 'cetak(1)\n'; 'indk nul.idk' prints '1' exit 0
```

### CLI-ALAT-04 — [sedang · khas Windows] argumen: characters outside the ANSI code page become '?'

From PowerShell (CreateProcessW, so the wide string is intact), 'é' arrives correctly ([233]), but '日本' becomes [63, 63] and the emoji becomes [63, 63]. argv is read through the ANSI main() instead of wmain/CommandLineToArgvW. The root cause is in the same family as T-W1 (ANSI API), but the symptom is different: argument values are corrupted, not just file paths. On POSIX argv is raw UTF-8 bytes, so it is not affected there.

Reproduksi:

```text
untuk a dalam argumen {
 cetak(panjang(a), [ord(c) untuk c dalam a])
}
# indk argo.idk "é" "日本" "😀"
```

Keluaran 1.4.0:

```text
1 [233]
2 [63, 63]
2 [63, 63]
```

### CLI-ALAT-05 — [sedang] REPL does not buffer multi-line () / [] or triple-quoted strings; '{' inside an unclosed triple string breaks the session

'xs = [1,\n2]' in the REPL gives 'Harapkan ekspresi' then 'Variabel tidak terdefinisi xs'. The same code is valid in a file (B16). A triple-quoted string over two lines reports 'String tiga kutip tidak ditutup' twice. With '{ baris2' inside a triple string, the REPL waits on '...' because braceDelta does not know about triple strings. The docs only say multi-line blocks are supported, so the ()/[] case is a weak doc promise, but it is an inconsistency between file and REPL. My simpler repro with 'a {' on the same line as the opening quotes did not swallow; the tester's case with '{' on a later line did (reproduced as r5.txt).

Reproduksi:

```text
xs = [1,
2]
cetak(xs)
--- and ---
t = """baris1
{ baris2
baris3"""
cetak(t)
cetak(9)
```

Keluaran 1.4.0:

```text
[baris 2, kolom 1] Kesalahan di akhir: Harapkan ekspresi.
[baris 1, kolom 2] Kesalahan di ']': Harapkan ekspresi.
Variabel tidak terdefinisi 'xs'.
--- triple: [baris 2] ... tidak ditutup. / [baris 5] ... tidak ditutup. / Harapkan '}' setelah blok. then '... ... ... ...'
```

### CLI-ALAT-06 — [sedang] REPL silently splits input lines longer than ~1023 bytes

A line cetak(panjang("a"*1015 literal)) gives 'String tidak ditutup' twice (on line 1 and line 2), so the line is cut into two. This is the same fixed-buffer (fgets 1024) pattern as input(). It works in a file.

Reproduksi:

```text
cetak(panjang("aaaa...(1015 x a)..."))   # fed to indk REPL via stdin
```

Keluaran 1.4.0:

```text
[baris 1] Kesalahan: String tidak ditutup.
[baris 2] Kesalahan: String tidak ditutup.
```

### CLI-ALAT-07 — [sedang] SQLite: whole-number desimal parameter bound as INTEGER; large desimal (1e300) rejected

db_ambil_satu(db, 'SELECT ? AS a, typeof(?) AS t', [2.0, 2.0]) gives {"a": 2, "t": "integer"}, while 2.5 gives real. 1e300 raises 'db: angka parameter di luar rentang tepat'. K2 keeps bulat and desimal as different types, and Python sqlite3 binds float as REAL. The docs only map INTEGER/REAL to angka and say the ±2^53 limit applies to INTEGER, not to a desimal parameter. The binder clearly checks 'is whole' instead of checking the VAL type.

Reproduksi:

```text
db = db_buka(":memory:")
cetak(db_ambil_satu(db, "SELECT ? AS a, typeof(?) AS t", [2.0, 2.0]))
cetak(db_ambil_satu(db, "SELECT ? AS a", [1e300]))
```

Keluaran 1.4.0:

```text
{"a": 2, "t": "integer"}
E db: angka parameter di luar rentang tepat
```

### CLI-ALAT-08 — [sedang] SQLite: bulat parameters above ~2^53 rejected though reads return exact int64 (no round-trip)

SELECT 9223372036854775807 returns the exact bulat. Binding 4611686018427387904 (2^62) raises 'angka parameter di luar rentang tepat', yet 9007199254740993 (2^53+1) is accepted. The check is inconsistent (likely done on a double conversion), and it contradicts both K2 (bulat = int64) and the doc's claim that INTEGER beyond ±2^53 is rejected on read.

Reproduksi:

```text
db = db_buka(":memory:")
cetak(db_ambil_satu(db, "SELECT 9223372036854775807 AS a"))
cetak(db_ambil_satu(db, "SELECT ? AS a", [9007199254740993]))
cetak(db_ambil_satu(db, "SELECT ? AS a", [4611686018427387904]))
```

Keluaran 1.4.0:

```text
{"a": 9223372036854775807}
{"a": 9007199254740993}
E db: angka parameter di luar rentang tepat (uncaught -> exit 70)
```

### CLI-ALAT-09 — [sedang] rapikan formats and --tulis rewrites files with syntax errors, exit 0

On 'fungsi f( {\n  cetak(1\n}}}\n', rapikan --tulis exits 0 and rewrites the file to 'fungsi f({\n        cetak(1\n        } } }', while periksa on the same file reports two syntax errors with exit 65. The docs do not say rapikan validates, but rewriting broken code (here also changing '}}}' to '} } }') without any signal is risky. The behaviour is OS-independent.

Reproduksi:

```text
fungsi f( {
  cetak(1
}}}
```

Keluaran 1.4.0:

```text
rapikan --tulis: exit 0, file becomes:
fungsi f({
        cetak(1
        } } }
periksa: [baris 1, kolom 11] Kesalahan di '{': Harapkan nama parameter. ... exit 65
```

### CLI-ALAT-10 — [rendah] Docs contradict themselves on BLOB support and the INTEGER ±2^53 rule

07_PUSTAKA_STANDAR 1.10 lines 619-635 document BLOB as a list of bytes (0..255), but the 'Batasan yang disengaja' list at line 660 still says 'BLOB belum didukung', and it says INTEGER beyond ±2^53 is rejected while SELECT 9223372036854775807 returns the exact value. This is a pure documentation inconsistency.

Reproduksi:

```text
db = db_buka(":memory:")
cetak(db_ambil_satu(db, "SELECT 9223372036854775807 AS a"))
```

Keluaran 1.4.0:

```text
{"a": 9223372036854775807}   (docs line 660: 'BLOB belum didukung ... INTEGER di luar ±2^53 ditolak')
```

### CLI-ALAT-11 — [rendah] input() at EOF returns kosong instead of raising

Reproduced: with empty stdin, input() returns kosong and the program exits 0. Python raises EOFError. The docs only say 'Baca satu baris dari stdin' and say nothing about EOF. This can be a deliberate design choice, so it is a semantic deviation and a doc gap, not a crash. A program loop 'selagi benar { input() }' will never end at EOF.

Reproduksi:

```text
a = input()
cetak("dapat:", a)
# printf '' | indk eof.idk
```

Keluaran 1.4.0:

```text
dapat: kosong
exit=0
```

### CLI-ALAT-12 — [rendah] Unknown options are treated as file names

'indk -x' prints 'Tidak dapat membuka file "-x".' with exit 74, while the rapikan subcommand already reports 'Opsi tidak dikenal' with exit 64. This is a UX inconsistency, independent of the OS.

Reproduksi:

```text
indk -x
```

Keluaran 1.4.0:

```text
Tidak dapat membuka file "-x".
exit 74
```

### CLI-ALAT-13 — [rendah] rapikan inserts a space in negative slice steps: x[::-1] -> x[:: -1]

Reproduced. The formatter treats '-' after ':' as a binary operator. Semantics are unchanged; this is cosmetic.

Reproduksi:

```text
x = [1,2,3]
cetak(x[1:], x[::-1])
```

Keluaran 1.4.0:

```text
x = [1, 2, 3]
cetak(x[1:], x[:: -1])
```

### CLI-ALAT-14 — [rendah] periksa findings are not reported in line order

Rerunning periksa on the tester's lint1.idk gives line order 6, 9, 11, 2, 17, 30, 36 (inner scopes first). This is cosmetic, and nothing in the docs promises an order.

Reproduksi:

```text
(tester's lint1.idk: unused locals in a function body and in nested scopes)
```

Keluaran 1.4.0:

```text
lint1.idk:6 ...
lint1.idk:9 ...
lint1.idk:11 ...
lint1.idk:2  variabel lokal 'sementara' tidak pernah dipakai
lint1.idk:17 ...
7 temuan
```

### CLI-ALAT-15 — [rendah] --periksa-pembaruan shows versions in inconsistent formats

Reproduced: 'Versi terpasang : 1.4.0' next to 'Versi terbaru   : v1.4.0' (the tag name is shown raw). The comparison still works ('sudah yang terbaru'); this is cosmetic only.

Reproduksi:

```text
indk --periksa-pembaruan
```

Keluaran 1.4.0:

```text
Versi terpasang : 1.4.0
Versi terbaru   : v1.4.0

Versi Anda sudah yang terbaru.
```

