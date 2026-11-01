#ifndef indonesiaku_sistem_h
#define indonesiaku_sistem_h

// Operating-system natives for Fase 6 gelombang C2: exit, stderr,
// environment, directories, and running other programs. Everything that
// lets a program look at or act on the machine beyond the files it was
// already allowed to open is refused under `indk --sandbox` (the web
// playground runs every program that way).

#include <stdbool.h>
#include <stddef.h>

#include "object.h"

// Called from main(): argv[0] for jalur_indk(), and the --sandbox flag.
void sistemAturArgv0(const char* argv0);
void sistemAturSandbox(bool aktif);
bool sistemSandbox(void);

// Absolute path of the running interpreter (UTF-8) into `out`. False when it
// cannot be determined or does not fit. Shared by jalur_indk() and the
// module search, which looks for pustaka/ next to the interpreter.
bool sistemJalurInterpreter(char* out, size_t ukuran);

Value nativeFnKeluar(int argCount, Value* args);           // keluar(kode=0)
Value nativeFnTulisGalat(int argCount, Value* args);       // tulis_galat(teks)
Value nativeFnLingkungan(int argCount, Value* args);       // lingkungan()
Value nativeFnAmbilLingkungan(int argCount, Value* args);  // ambil_lingkungan(nama, bawaan)
Value nativeFnDirektoriKini(int argCount, Value* args);    // direktori_kini()
Value nativeFnIsiDirektori(int argCount, Value* args);     // isi_direktori(jalur=".")
Value nativeFnBuatDirektori(int argCount, Value* args);    // buat_direktori(jalur)
Value nativeFnHapusDirektori(int argCount, Value* args);   // hapus_direktori(jalur)
Value nativeFnJalurIndk(int argCount, Value* args);        // jalur_indk()
Value nativeFnInfoJalur(int argCount, Value* args);        // _info_jalur(jalur)
Value nativeFnJalurNyata(int argCount, Value* args);       // _jalur_nyata(jalur)
Value nativeFnJalankanProses(int argCount, Value* args);   // _jalankan_proses(perintah, masukan)

#endif
