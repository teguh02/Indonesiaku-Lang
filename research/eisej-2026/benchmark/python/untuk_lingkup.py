# Padanan Python dari examples/untuk_lingkup.idk (K21), baris demi baris.
k = 99
for k in range(3): pass
print(k)
def f():
    j = 7
    for j in range(3): pass
    return j
print(f())
for q in range(3): pass
print(q)
x = "global"
def g():
    print([x for x in [1, 2]], x)
    for y in [1]: pass
    return y
print(g(), x)
def h():
    global x
    for x in ["dari h"]: pass
h()
print(x)
def luar():
    z = 0
    def dalam_f():
        nonlocal z
        for z in [5, 6]: pass
    dalam_f()
    return z
print(luar())
def kosongan():
    for w in []: pass
    return w
try:
    print(kosongan())
except Exception as e:
    print("galat:", e)
