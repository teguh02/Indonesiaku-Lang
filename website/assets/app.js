/* ============================================================
   Indonesiaku — Alpine.js: bahasa (ID/EN) + tema (terang/gelap)
   Kamus terjemahan: assets/i18n.json (diedit tanpa menyentuh kode).
   Alpine baru dimuat SETELAH kamus siap, supaya tidak ada kedipan kunci.
   ============================================================ */
(function () {
  "use strict";

  var ALPINE_URL = "https://cdn.jsdelivr.net/npm/alpinejs@3.14.1/dist/cdn.min.js";
  var SCRIPT_SRC = (document.currentScript && document.currentScript.src) || "";
  var I18N_URL = SCRIPT_SRC.replace(/[^/]*$/, "") + "i18n.json";

  var I18N = { id: {}, en: {} };

  function currentLang() {
    try {
      return localStorage.getItem("indk-lang") === "en" ? "en" : "id";
    } catch (e) {
      return "id";
    }
  }

  function terjemah(lang, key) {
    return (I18N[lang] && I18N[lang][key]) || I18N.id[key] || key;
  }

  // Tersedia lebih awal untuk skrip non-Alpine (reader dokumentasi, playground).
  window.t18n = function (key) {
    return terjemah(currentLang(), key);
  };

  /* Versi dipakai bersama oleh semua halaman.
     Diambil dari cache-buster `app.js?v=X.Y.Z` karena itulah satu-satunya
     penanda versi yang SELALU ikut dinaikkan proses rilis di setiap halaman.
     Badge footer dulu ditulis tangan per halaman, jadi index.html tertinggal
     di v1.5.0 sementara about/author sudah v1.7.0. */
  var VERSI = (SCRIPT_SRC.match(/[?&]v=(\d+\.\d+\.\d+)/) || [])[1] || "";
  window.indkVersi = VERSI;

  function terapkanVersi() {
    if (!VERSI) return; // tanpa ?v= biarkan teks bawaan HTML apa adanya
    var el = document.querySelectorAll("[data-indk-versi]");
    for (var i = 0; i < el.length; i++) el[i].textContent = "v" + VERSI;
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", terapkanVersi);
  } else {
    terapkanVersi();
  }
  window.indkLang = currentLang;

  /* Salin teks ke papan klip. Dipakai Alpine (salin()) dan skrip non-Alpine
     seperti tombol "Salin sebagai Markdown" di docs.html, supaya jalur
     fallback untuk browser tanpa navigator.clipboard tidak ditulis dua kali. */
  window.indkSalinTeks = function (teks) {
    if (navigator.clipboard && navigator.clipboard.writeText) {
      return navigator.clipboard.writeText(teks).catch(function () {
        fallbackCopy(teks);
      });
    }
    fallbackCopy(teks);
    return Promise.resolve();
  };

  function simpanTema(dark) {
    try {
      localStorage.setItem("indk-theme", dark ? "dark" : "light");
    } catch (e) {
      /* mode privat: abaikan */
    }
  }

  // Fallback untuk origin non-secure (mis. http://IP-LAN).
  function fallbackCopy(text) {
    var area = document.createElement("textarea");
    area.value = text;
    area.setAttribute("readonly", "");
    area.style.position = "fixed";
    area.style.opacity = "0";
    document.body.appendChild(area);
    area.select();
    try {
      document.execCommand("copy");
    } catch (e) {
      /* abaikan */
    }
    document.body.removeChild(area);
  }

  // Didaftarkan SEBELUM Alpine dieksekusi.
  document.addEventListener("alpine:init", function () {
    Alpine.data("shell", function () {
      return {
        lang: currentLang(),
        dark: document.documentElement.classList.contains("dark"),
        menu: false,

        init: function () {
          document.documentElement.lang = this.lang;
          this.$watch("lang", function (value) {
            document.documentElement.lang = value;
            try {
              localStorage.setItem("indk-lang", value);
            } catch (e) {
              /* abaikan */
            }
            // Beri tahu skrip non-Alpine (mis. daftar sidebar dokumentasi).
            document.documentElement.dispatchEvent(new CustomEvent("indk:lang"));
          });
        },

        t: function (key) {
          return terjemah(this.lang, key);
        },

        setLang: function (value) {
          this.lang = value === "en" ? "en" : "id";
          // Simpan langsung supaya t18n() ikut berubah tanpa menunggu watcher.
          try {
            localStorage.setItem("indk-lang", this.lang);
          } catch (e) {
            /* abaikan */
          }
        },

        toggleTheme: function () {
          this.dark = !this.dark;
          document.documentElement.classList.toggle("dark", this.dark);
          simpanTema(this.dark);
        },

        salin: function (target, button) {
          var text = "";
          if (typeof target === "string") {
            text = target;
          } else if (target && target.innerText) {
            text = target.innerText;
          }

          var done = function () {
            if (!button) return;
            var label = button.querySelector(".copy-label");
            if (!label) return;
            if (button._salinTimer) clearTimeout(button._salinTimer);
            label.textContent = window.t18n("ui_copied");
            label.classList.add("text-emerald-400");
            button._salinTimer = setTimeout(function () {
              label.textContent = window.t18n("ui_copy");
              label.classList.remove("text-emerald-400");
            }, 1500);
          };

          if (window.indkSalinTeks) {
            window.indkSalinTeks(text).then(done, done);
          } else {
            fallbackCopy(text);
            done();
          }
        },
      };
    });
  });

  function muatAlpine() {
    var s = document.createElement("script");
    s.src = ALPINE_URL;
    s.defer = true;
    document.head.appendChild(s);
  }

  fetch(I18N_URL)
    .then(function (res) {
      if (!res.ok) throw new Error(res.status + " " + res.statusText);
      return res.json();
    })
    .then(function (kamus) {
      if (kamus && kamus.id && kamus.en) I18N = kamus;
    })
    .catch(function (err) {
      // Situs tetap bisa dipakai dengan teks statis berbahasa Indonesia.
      console.error("[i18n] gagal memuat " + I18N_URL + ":", err);
    })
    .then(muatAlpine);
})();
