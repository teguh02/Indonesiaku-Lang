# Padanan Python dari examples/cetak_fungsi.idk (K12), baris demi baris.

# --- Pemakaian dasar ---
print("satu")
print(1, 2, 3)
print("campur", 42, True, None, 3.5)

# Tanpa argumen: baris kosong.
print()

# --- Pemisah dan akhiran ---
print("a", "b", "c", sep="-")
print("tanpa", "spasi", sep="")
print("tanpa newline", end="")
print(" <- lanjut di baris yang sama")
print("x", "y", sep="+", end=" (selesai)\n")

# --- print sebagai nilai ---
tulis = print
tulis("dipanggil lewat variabel")

print(callable(print))
print(type(print))

def jalankan(fn, pesan):
    fn(pesan)
jalankan(print, "dioper sebagai argumen")

# Tetap berfungsi di dalam fungsi dan perulangan.
def sapa(nama):
    print("halo", nama)
sapa("budi")

for i in range(3):
    print("baris", i)

# --- Galat ---
try:
    print("a", sep=5)
except Exception as e:
    print("galat:", e)

try:
    print("a", tak_dikenal="x")
except Exception as e:
    print("galat:", e)

# Fungsi bawaan lain menolak argumen bernama.
try:
    print(len("ab", sep="-"))
except Exception as e:
    print("galat:", e)
