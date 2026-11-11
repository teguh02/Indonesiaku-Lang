; Installer Windows untuk Indonesiaku (Inno Setup 6).
;
; Menggantikan compiler/indonesiaku_installer.iss yang basi: skrip lama
; memasang "indonesiaku.exe" (sekarang bernama indk.exe), menyebut versi
; 0.1.4, dan menunjuk handle GitHub yang salah.
;
; Dibangun oleh .github/workflows/release.yml:
;   iscc /DVersiApp=1.4.0 packaging\windows\indonesiaku.iss
;
; AppId HARUS tetap sama antar versi. Inno memakainya untuk mengenali
; pasangan lama: dengan AppId yang sama, memasang versi baru akan menimpa
; yang lama di tempatnya -- pengguna tidak perlu meng-uninstall dulu.
; Mengganti AppId akan membuat dua pasangan berdampingan.

#ifndef VersiApp
  #define VersiApp "0.0.0"
#endif

; Hanya tests\pemasang_path.ps1 yang menimpa ini (/DAppIdApp=...), agar
; pemasang uji tidak pernah menimpa atau meng-uninstall Indonesiaku milik
; pengguna. Rilis selalu memakai GUID bawaan di bawah.
#ifndef AppIdApp
  #define AppIdApp "{{B7F1C4E2-9A3D-4F58-8C21-6E0D5A9B3C47}"
#endif

[Setup]
AppId={#AppIdApp}
AppName=Indonesiaku
AppVersion={#VersiApp}
AppVerName=Indonesiaku {#VersiApp}
AppPublisher=Teguh Rijanandi
AppPublisherURL=https://github.com/teguh02/Indonesiaku-Lang
AppSupportURL=https://github.com/teguh02/Indonesiaku-Lang/issues
AppUpdatesURL=https://github.com/teguh02/Indonesiaku-Lang/releases
DefaultDirName={autopf}\Indonesiaku
DefaultGroupName=Indonesiaku
AllowNoIcons=yes
LicenseFile=..\..\LICENSE
OutputDir=..\..\dist
OutputBaseFilename=Indonesiaku-Setup-{#VersiApp}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
; Memasang ke Program Files perlu hak admin; PATH sistem juga.
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
; Menimpa pasangan lama tanpa menanyakan apa pun.
CloseApplications=yes
RestartApplications=no
; Siarkan WM_SETTINGCHANGE setelah PATH berubah (pasang maupun uninstall),
; supaya terminal baru dari Explorer langsung melihat PATH yang benar.
ChangesEnvironment=yes
UninstallDisplayName=Indonesiaku {#VersiApp}
; indk.exe belum punya resource ikon, jadi entri Add/Remove Programs
; diarahkan ke berkas .ico yang ikut terpasang.
UninstallDisplayIcon={app}\indonesiaku.ico
; Ikon dan gambar wizard memakai logo "idk" persegi (website/assets/
; logo-idk-square.png). Tanpa SetupIconFile, Inno memakai ikon kardus
; bawaannya -- tidak ada kaitannya dengan proyek ini.
SetupIconFile=indonesiaku.ico
WizardSmallImageFile=wizard-kecil-55.bmp,wizard-kecil-83.bmp,wizard-kecil-110.bmp,wizard-kecil-138.bmp

[Languages]
Name: "indonesian"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "tambahpath"; Description: "Tambahkan indk ke PATH sistem"; GroupDescription: "Integrasi:"

[Files]
; indk.exe dan pustaka/ HARUS berdampingan: biner tanpa pustaka standar
; gagal pada 'impor web' dengan "Modul 'web' tidak ditemukan".
Source: "..\..\indk.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\pustaka\*"; DestDir: "{app}\pustaka"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\examples\*"; DestDir: "{app}\examples"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\CHANGELOG.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
; Ikon ikut dipasang supaya pintasan Start Menu tetap punya ikon setelah
; berkas pemasang dihapus.
Source: "indonesiaku.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Indonesiaku (REPL)"; Filename: "{app}\indk.exe"; IconFilename: "{app}\indonesiaku.ico"
Name: "{group}\Uninstall Indonesiaku"; Filename: "{uninstallexe}"

[Registry]
; PATH sistem. {olddata} dipertahankan agar entri lain tidak hilang.
; Uninstall TIDAK bisa memakai flag uninsdeletevalue di sini: flag itu
; menghapus seluruh nilai Path. Entri {app} dibuang oleh
; CurUninstallStepChanged di [Code] (T-W13).
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; \
    ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; \
    Tasks: tambahpath; Check: BelumAdaDiPath(ExpandConstant('{app}'))

[Code]
const
  KunciEnv = 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';

// Mencegah {app} ditambahkan dua kali ketika pengguna memasang ulang atau
// memperbarui: tanpa ini, PATH memanjang setiap upgrade.
function BelumAdaDiPath(Dir: string): Boolean;
var
  PathSekarang: string;
begin
  if not RegQueryStringValue(HKEY_LOCAL_MACHINE, KunciEnv, 'Path', PathSekarang) then
  begin
    Result := True;
    exit;
  end;
  Result := Pos(';' + Uppercase(Dir) + ';', ';' + Uppercase(PathSekarang) + ';') = 0;
end;

// Path tanpa entri Dir. Pembandingan tanpa beda huruf besar/kecil, dengan
// spasi tepi dan '\' penutup diabaikan. Entri lain (termasuk urutan dan
// entri kosong yang sudah ada) dibiarkan apa adanya, dan pembuangan tidak
// meninggalkan ';;' baru.
function HapusDariPath(PathLama, Dir: string): string;
var
  Sisa, Entri, Target: string;
  P: Integer;
  Pertama: Boolean;
begin
  Result := '';
  Pertama := True;
  Target := Uppercase(RemoveBackslashUnlessRoot(Trim(Dir)));
  Sisa := PathLama;
  repeat
    P := Pos(';', Sisa);
    if P > 0 then
    begin
      Entri := Copy(Sisa, 1, P - 1);
      Delete(Sisa, 1, P);
    end else
      Entri := Sisa;
    if Uppercase(RemoveBackslashUnlessRoot(Trim(Entri))) <> Target then
    begin
      if not Pertama then
        Result := Result + ';';
      Result := Result + Entri;
      Pertama := False;
    end;
  until P = 0;
end;

// T-W13: uninstaller dulu meninggalkan {app} di PATH sistem. Dibuang di
// usUninstall, sebelum berkas dihapus; nilai registri hanya ditulis ulang
// bila memang berubah. Entri installer lawas (Program Files (x86)\
// Indonesiaku\bin) bukan milik pasangan ini dan tidak disentuh.
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  PathLama, PathBaru: string;
begin
  if CurUninstallStep <> usUninstall then
    exit;
  if not RegQueryStringValue(HKEY_LOCAL_MACHINE, KunciEnv, 'Path', PathLama) then
    exit;
  PathBaru := HapusDariPath(PathLama, ExpandConstant('{app}'));
  if PathBaru <> PathLama then
    RegWriteExpandStringValue(HKEY_LOCAL_MACHINE, KunciEnv, 'Path', PathBaru);
end;
