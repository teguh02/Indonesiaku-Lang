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

[Setup]
AppId={{B7F1C4E2-9A3D-4F58-8C21-6E0D5A9B3C47}
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
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; \
    ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; \
    Tasks: tambahpath; Check: BelumAdaDiPath(ExpandConstant('{app}'))

[Code]
// Mencegah {app} ditambahkan dua kali ketika pengguna memasang ulang atau
// memperbarui: tanpa ini, PATH memanjang setiap upgrade.
function BelumAdaDiPath(Dir: string): Boolean;
var
  PathSekarang: string;
begin
  if not RegQueryStringValue(HKEY_LOCAL_MACHINE,
       'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
       'Path', PathSekarang) then
  begin
    Result := True;
    exit;
  end;
  Result := Pos(';' + Uppercase(Dir) + ';', ';' + Uppercase(PathSekarang) + ';') = 0;
end;
