// Daftar builtin ekstensi VS Code harus sama persis dengan defineNative()
// di src/vm.c. Dijalankan oleh tests/builtin_konsisten.sh.
//
// Ditulis sebagai berkas tersendiri, bukan `node -e` di dalam heredoc bash:
// pola regex di sini penuh backslash, dan melewatkannya lewat dua lapis
// kutip shell sempat mengubah \b menjadi literal dan membuat SELURUH builtin
// terlihat hilang.
"use strict";

const fs = require("fs");
const path = require("path");

const AKAR = path.join(__dirname, "..");
const EKS = path.join(AKAR, "editors", "vscode-indonesiaku");

let gagal = 0;
const lapor = (ok, pesan, rinci) => {
    console.log(`  ${ok ? "OK   " : "GAGAL"} ${pesan}`);
    if (!ok) {
        gagal = 1;
        if (rinci && rinci.length) {
            console.log(rinci.map((x) => "          " + x).join("\n"));
        }
    }
};

// --- sumber kebenaran: defineNative() di VM --------------------------------
// Builtin berawalan '_' dikecualikan: itu detail implementasi yang dibungkus
// pustaka/*.idk, bukan nama yang diketik pengguna.
const vmSrc = fs.readFileSync(path.join(AKAR, "src", "vm.c"), "utf8");
const vm = [...vmSrc.matchAll(/defineNative\("([a-z_0-9]+)"/g)]
    .map((m) => m[1])
    .filter((n) => !n.startsWith("_"));
const vmSet = [...new Set(vm)].sort();

if (vmSet.length < 50) {
    console.log(`GAGAL: hanya ${vmSet.length} builtin terbaca dari src/vm.c; pola kemungkinan rusak`);
    process.exit(1);
}

// --- daftar di language-data.js --------------------------------------------
const data = require(path.join(EKS, "language-data.js"));
const eks = [...new Set((data.BUILTINS || []).filter((n) => !n.startsWith("_")))].sort();

const hilang = vmSet.filter((n) => !eks.includes(n));
const asing = eks.filter((n) => !vmSet.includes(n));

lapor(hilang.length === 0, `semua builtin VM ada di language-data.js (${vmSet.length})`, hilang);
lapor(asing.length === 0, "tidak ada nama asing di language-data.js", asing);

// --- tmLanguage -------------------------------------------------------------
const tmPath = path.join(EKS, "syntaxes", "indonesiaku.tmLanguage.json");
const tmMentah = fs.readFileSync(tmPath, "utf8");

// '\b' yang tertulis sebagai escape JSON menjadi U+0008: JSON tetap sah,
// tetapi regex kata kuncinya mati total tanpa gejala.
lapor(!tmMentah.includes("\u0008"), "tmLanguage bebas backspace literal (U+0008)");

const tm = JSON.parse(tmMentah);
const pola = [];
(function jelajah(o) {
    if (Array.isArray(o)) return o.forEach(jelajah);
    if (o && typeof o === "object") {
        for (const [k, v] of Object.entries(o)) {
            if ((k === "match" || k === "begin" || k === "end") && typeof v === "string") pola.push(v);
            jelajah(v);
        }
    }
})(tm);

// Pola yang menyorot builtin: yang memuat nama builtin terpanjang.
const terpanjang = vmSet.slice().sort((a, b) => b.length - a.length)[0];
const polaBuiltin = pola.find((p) => p.includes(terpanjang)) || pola.find((p) => p.includes("cetak"));

if (!polaBuiltin) {
    lapor(false, "pola builtin tidak ditemukan di tmLanguage");
} else {
    const takAda = vmSet.filter((n) => !polaBuiltin.includes(n));
    lapor(takAda.length === 0, "tiap builtin VM tercantum di pola tmLanguage", takAda);

    // Cocok UTUH, bukan sebagian: tanpa urutan terpanjang-dulu, 'himpunan'
    // menang atas 'himpunan_beku' dan hanya awalannya yang tersorot.
    const re = new RegExp(polaBuiltin);
    const ujiPositif = ["cetak", "himpunan_beku", "teks_gabung", "panjang_byte"].filter((n) => vmSet.includes(n));
    const positifGagal = ujiPositif.filter((n) => {
        const m = n.match(re);
        return !m || m[0] !== n;
    });
    lapor(positifGagal.length === 0, "builtin cocok utuh (termasuk nama berawalan sama)", positifGagal);

    const negatifGagal = ["daftarku", "hashing", "cetakan"].filter((n) => {
        const m = n.match(re);
        return m && m[0] === n;
    });
    lapor(negatifGagal.length === 0, "kata serupa tidak ikut tersorot", negatifGagal);
}

console.log(gagal === 0 ? "LULUS" : "GAGAL");
process.exit(gagal);
