#ifndef MyAppName
  #define MyAppName "JamulusPlus"
#endif
#ifndef MyAppVersion
  #define MyAppVersion "0.1.0"
#endif
#ifndef MyAppDisplayVersion
  #define MyAppDisplayVersion MyAppVersion
#endif
#ifndef MyVersionInfoVersion
  #define MyVersionInfoVersion MyAppVersion
#endif
#ifndef MyAppPublisher
  #define MyAppPublisher "AndyMcProducer"
#endif
#ifndef MyAppURL
  #define MyAppURL "https://github.com/AndyMcProducer/JamulusPlus"
#endif
#ifndef MyAppCopyright
  #define MyAppCopyright "Copyright (C) 2026 AndyMcProducer"
#endif
#ifndef MyVst3Dir
  #define MyVst3Dir "{commoncf64}\VST3"
#endif
#ifndef MyStandaloneDir
  #define MyStandaloneDir "{autopf64}\JamulusPlus"
#endif
#ifndef MyVst3SourceDir
  #define MyVst3SourceDir "..\dist\JamulusPlus.vst3"
#endif
#ifndef MyStandaloneSourceDir
  #define MyStandaloneSourceDir "..\dist\JamulusPlus-Standalone"
#endif
#ifndef MyOutputDir
  #define MyOutputDir "installer_output"
#endif
#ifndef MyOutputBaseFilename
  #define MyOutputBaseFilename "JamulusPlus_Setup_{#MyAppVersion}"
#endif

[Setup]
AppId={{0E0E7283-1D30-4D38-8EEA-6C69490B5B81}
AppName={#MyAppName}
AppVersion={#MyAppDisplayVersion}
AppVerName={#MyAppName} {#MyAppDisplayVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
AppCopyright={#MyAppCopyright}
VersionInfoVersion={#MyVersionInfoVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription={#MyAppName} Installer
VersionInfoCopyright={#MyAppCopyright}
VersionInfoProductName={#MyAppName}
VersionInfoProductVersion={#MyVersionInfoVersion}
DefaultDirName={#MyStandaloneDir}
DefaultGroupName={#MyAppName}
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#MyOutputDir}
OutputBaseFilename={#MyOutputBaseFilename}
SetupIconFile=compiler:SetupClassicIcon.ico
UninstallDisplayIcon={app}\JamulusPlus.exe
Compression=lzma2/max
SolidCompression=no
LZMANumBlockThreads=1
WizardStyle=modern
WizardSizePercent=120
WizardResizable=yes
UninstallDisplayName={#MyAppName}
CreateUninstallRegKey=yes
Uninstallable=yes
MinVersion=10.0
LicenseFile=LICENSE.txt

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
WelcomeLabel1=Welcome to the {#MyAppName} Setup Wizard
WelcomeLabel2=This will install the JamulusPlus standalone app and/or the VST3 plugin.%n%nStandalone location:%n{#MyStandaloneDir}%n%nVST3 location:%n{#MyVst3Dir}%n%nClick Next to continue.
FinishedHeadingLabel=Installation Complete
FinishedLabelNoIcons={#MyAppName} has been successfully installed.

[Types]
Name: "full"; Description: "Standalone app and VST3 plugin"
Name: "plugin"; Description: "VST3 plugin only"
Name: "standalone"; Description: "Standalone app only"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "plugin"; Description: "JamulusPlus VST3 plugin"; Types: full plugin custom
Name: "standalone"; Description: "JamulusPlus standalone app"; Types: full standalone custom

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut for the standalone app"; Components: standalone

[Files]
Source: "{#MyVst3SourceDir}\*"; DestDir: "{#MyVst3Dir}\JamulusPlus.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: plugin
Source: "{#MyStandaloneSourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: standalone

[Icons]
Name: "{group}\JamulusPlus"; Filename: "{app}\JamulusPlus.exe"; Components: standalone
Name: "{commondesktop}\JamulusPlus"; Filename: "{app}\JamulusPlus.exe"; Tasks: desktopicon; Components: standalone
Name: "{group}\Uninstall JamulusPlus"; Filename: "{uninstallexe}"

[Registry]
Root: HKLM; Subkey: "Software\AndyMcProducer\JamulusPlus"; ValueType: string; ValueName: "StandaloneInstallPath"; ValueData: "{app}"; Flags: uninsdeletekey; Components: standalone
Root: HKLM; Subkey: "Software\AndyMcProducer\JamulusPlus"; ValueType: string; ValueName: "VST3InstallPath"; ValueData: "{#MyVst3Dir}\JamulusPlus.vst3"; Flags: uninsdeletevalue; Components: plugin

[Run]
Filename: "{app}\JamulusPlus.exe"; Description: "Launch JamulusPlus"; Flags: postinstall nowait skipifsilent unchecked; Components: standalone

[UninstallDelete]
Type: filesandordirs; Name: "{#MyVst3Dir}\JamulusPlus.vst3"
Type: filesandordirs; Name: "{app}"

[Code]
const
  VC_REDIST_KEY = 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64';
  VC_REDIST_KEY_WOW = 'SOFTWARE\WOW6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\x64';
  VC_REDIST_MAJOR_MIN = 14;
  VC_REDIST_MINOR_MIN = 29;

function IsVCRedistInstalled: Boolean;
var
  Major, Minor: Cardinal;
begin
  Result := False;
  if RegQueryDWordValue(HKLM, VC_REDIST_KEY, 'Major', Major) then
    if RegQueryDWordValue(HKLM, VC_REDIST_KEY, 'Minor', Minor) then
      Result := (Major > VC_REDIST_MAJOR_MIN) or ((Major = VC_REDIST_MAJOR_MIN) and (Minor >= VC_REDIST_MINOR_MIN));

  if not Result then
    if RegQueryDWordValue(HKLM, VC_REDIST_KEY_WOW, 'Major', Major) then
      if RegQueryDWordValue(HKLM, VC_REDIST_KEY_WOW, 'Minor', Minor) then
        Result := (Major > VC_REDIST_MAJOR_MIN) or ((Major = VC_REDIST_MAJOR_MIN) and (Minor >= VC_REDIST_MINOR_MIN));
end;

function InitializeSetup(): Boolean;
var
  VCWarning: String;
begin
  Result := True;
  if not IsVCRedistInstalled() then
  begin
    VCWarning := 'The Microsoft Visual C++ 2015-2022 Redistributable (x64) may not be installed.' + #13#10 + #13#10 +
                 'JamulusPlus requires this runtime to function correctly.' + #13#10 + #13#10 +
                 'Download: https://aka.ms/vs/17/release/vc_redist.x64.exe' + #13#10 + #13#10 +
                 'Do you want to continue with the installation?';
    Result := MsgBox(VCWarning, mbConfirmation, MB_YESNO or MB_DEFBUTTON1) = IDYES;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
    Log('JamulusPlus installation completed successfully.');
end;

