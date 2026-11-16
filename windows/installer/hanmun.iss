; 빠른 한문 입력기 설치 파일 (Inno Setup 6)
#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\..\build"
#endif

[Setup]
AppId={{2F3C03EE-41F3-47DB-A00C-ACDACFFB961A}
AppName=빠른 한문 입력기
AppVersion={#AppVersion}
AppPublisher=세 (@segrc_)
AppPublisherURL=https://x.com/segrc_
AppSupportURL=https://github.com/segrc/hanmun
DefaultDirName={autopf}\Hanmun
DisableProgramGroupPage=yes
DisableDirPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.17763
OutputDir=..\..\dist
OutputBaseFilename=hanmun-setup-{#AppVersion}
SetupIconFile=..\src\hanmun.ico
UninstallDisplayIcon={app}\x64\hanmun.dll
LicenseFile=..\..\LICENSE
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=no
RestartIfNeededByRun=no

[Languages]
Name: "ko"; MessagesFile: "compiler:Languages\Korean.isl"

[Files]
Source: "{#BuildDir}\x64\Release\hanmun.dll"; DestDir: "{app}\x64"; Flags: ignoreversion regserver restartreplace uninsrestartdelete 64bit
Source: "{#BuildDir}\x86\Release\hanmun.dll"; DestDir: "{app}\x86"; Flags: ignoreversion regserver restartreplace uninsrestartdelete 32bit
Source: "..\..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\THIRD_PARTY_NOTICES.txt"; DestDir: "{app}"; Flags: ignoreversion

[Run]
Filename: "ms-settings:regionlanguage"; Description: "언어 설정 열기 (한국어 → 키보드에 ‘빠른 한문 입력기’ 추가)"; Flags: postinstall shellexec skipifsilent unchecked
