; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=Speex
AppVerName=Speex V1.0.4
AppPublisherURL=http://www.speex.org
AppSupportURL=http://www.speex.org
AppUpdatesURL=http://www.speex.org
DefaultDirName={pf}\Speex
DefaultGroupName=Speex
AllowNoIcons=yes
LicenseFile=..\COPYING
InfoAfterFile=..\README
OutputDir=.
OutputBaseFilename=speex_win32_1.0.4_setup
;WizardImageFile=speexlogo.bmp
; uncomment the following line if you want your installation to run on NT 3.51 too.
; MinVersion=4,3.51

[Tasks]
;Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; MinVersion: 4,4

[Dirs]
Name: "{app}"
Name: "{app}\doc"
Name: "{app}\libspeex"
Name: "{app}\libspeex\include"
Name: "{app}\libspeex\include\speex"

[Files]
Source: "speexdec\Release\speexdec.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "speexenc\Release\speexenc.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\doc\manual.pdf"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "libspeex\Release\libspeex.lib"; DestDir: "{app}\libspeex"; Flags: ignoreversion
Source: "libspeex\Release\libspeex.dll"; DestDir: "{app}\libspeex"; Flags: ignoreversion
Source: "libspeex\Release\libspeex.exp"; DestDir: "{app}\libspeex"; Flags: ignoreversion
Source: "..\include\speex.h"; DestDir: "{app}\libspeex\include"; Flags: ignoreversion
Source: "..\include\speex_bits.h"; DestDir: "{app}\libspeex\include"; Flags: ignoreversion
Source: "..\include\speex_callbacks.h"; DestDir: "{app}\libspeex\include"; Flags: ignoreversion
Source: "..\include\speex_header.h"; DestDir: "{app}\libspeex\include"; Flags: ignoreversion
Source: "..\include\speex_stereo.h"; DestDir: "{app}\libspeex\include"; Flags: ignoreversion
Source: "..\include\speex\speex.h"; DestDir: "{app}\libspeex\include\speex\"; Flags: ignoreversion
Source: "..\include\speex\speex_bits.h"; DestDir: "{app}\libspeex\include\speex\"; Flags: ignoreversion
Source: "..\include\speex\speex_callbacks.h"; DestDir: "{app}\libspeex\include\speex\"; Flags: ignoreversion
Source: "..\include\speex\speex_header.h"; DestDir: "{app}\libspeex\include\speex\"; Flags: ignoreversion
Source: "..\include\speex\speex_stereo.h"; DestDir: "{app}\libspeex\include\speex\"; Flags: ignoreversion

[Run]

