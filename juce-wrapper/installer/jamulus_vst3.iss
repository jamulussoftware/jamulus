; ============================================================================
; Jamulus VST3 Installer Script for Inno Setup 6
; https://jrsoftware.org/isdl.php
; ============================================================================

#define MyAppName "Jamulus VST3"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Jamulus Project"
#define MyAppURL "https://jamulus.io"
#define MyAppCopyright "Copyright (C) 2025 Jamulus Project"

; VST3 installs to the standard Common Files\VST3 location
#define MyVst3Dir "{commoncf64}\VST3"

[Setup]
; Unique Application Identifier (generate new GUID for major releases)
AppId={{B7A23C1E-7D4F-4C8B-A3B3-5F1E2D3E4F5G}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
AppCopyright={#MyAppCopyright}

; Version info for the installer executable
VersionInfoVersion={#MyAppVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription={#MyAppName} Installer
VersionInfoCopyright={#MyAppCopyright}
VersionInfoProductName={#MyAppName}
VersionInfoProductVersion={#MyAppVersion}

; Install to VST3 directory (no user prompt needed)
DefaultDirName={#MyVst3Dir}
DisableDirPage=yes

; Start Menu group (disabled - VST plugins don't need shortcuts)
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes

; Admin required to write to Program Files
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog

; 64-bit only (VST3 is 64-bit)
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

; Output settings
OutputDir=installer_output
OutputBaseFilename=JamulusVST3_Setup_{#MyAppVersion}
SetupIconFile=compiler:SetupClassicIcon.ico
UninstallDisplayIcon={app}\JamulusVST3.vst3\Contents\x86_64-win\JamulusVST3.vst3

; Compression (LZMA2 is better for Inno 6)
Compression=lzma2/ultra64
SolidCompression=yes
LZMANumBlockThreads=4

; Visual style
WizardStyle=modern
WizardSizePercent=120
WizardResizable=yes

; Uninstaller settings
UninstallDisplayName={#MyAppName}
CreateUninstallRegKey=yes
Uninstallable=yes

; Minimum Windows version (Windows 10+)
MinVersion=10.0

; Show "What's new" at finish if you have a changelog
; InfoAfterFile=changelog.txt

; License file (GPL v2 from Jamulus)
LicenseFile=LICENSE.txt

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Messages]
SetupAppTitle={#MyAppName} Setup
SetupWindowTitle={#MyAppName} Setup - Version {#MyAppVersion}
WelcomeLabel1=Welcome to the {#MyAppName} Setup Wizard
WelcomeLabel2=This will install {#MyAppName} version {#MyAppVersion} on your computer.%n%nThe plugin will be installed to your VST3 folder:%n%n{#MyVst3Dir}%n%nClick Next to continue.
FinishedHeadingLabel=Installation Complete
FinishedLabelNoIcons={#MyAppName} has been successfully installed.%n%nYou can now load the plugin in your DAW by scanning for new VST3 plugins.

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "plugin"; Description: "Jamulus VST3 Plugin"; Types: full custom; Flags: fixed
Name: "docs"; Description: "Documentation and Readme"; Types: full

[Files]
; Main VST3 bundle (entire directory structure)
Source: "..\build\plugin\jamulus_vst3_artefacts\Release\VST3\JamulusVST3.vst3\*"; \
    DestDir: "{app}\JamulusVST3.vst3"; \
    Flags: ignoreversion recursesubdirs createallsubdirs; \
    Components: plugin

; Documentation (if exists)
; Source: "..\docs\README.txt"; DestDir: "{app}"; Flags: ignoreversion; Components: docs
; Source: "..\docs\LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion; Components: docs

[Icons]
; Uninstaller in Start Menu (optional)
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"; Comment: "Uninstall {#MyAppName}"

[Registry]
; Register plugin location for easy discovery (optional)
Root: HKLM; Subkey: "Software\{#MyAppPublisher}\{#MyAppName}"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Flags: uninsdeletekey

[Run]
; Post-install actions (optional)
; Example: Open documentation
; Filename: "{app}\README.txt"; Description: "View Readme"; Flags: postinstall skipifsilent shellexec unchecked

[UninstallDelete]
; Clean up any generated files during uninstall
Type: filesandordirs; Name: "{app}\JamulusVST3.vst3"

[Code]
// ============================================================================
// Custom Pascal Script for Installation Logic
// ============================================================================

const
  // Visual C++ 2015-2022 Redistributable registry keys
  VC_REDIST_KEY = 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64';
  VC_REDIST_KEY_WOW = 'SOFTWARE\WOW6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\x64';
  VC_REDIST_MAJOR_MIN = 14;
  VC_REDIST_MINOR_MIN = 29; // VS 2019 16.11+

function IsVCRedistInstalled: Boolean;
var
  Major, Minor: Cardinal;
begin
  Result := False;
  
  // Check native 64-bit registry
  if RegQueryDWordValue(HKLM, VC_REDIST_KEY, 'Major', Major) then
  begin
    if RegQueryDWordValue(HKLM, VC_REDIST_KEY, 'Minor', Minor) then
    begin
      Result := (Major > VC_REDIST_MAJOR_MIN) or 
                ((Major = VC_REDIST_MAJOR_MIN) and (Minor >= VC_REDIST_MINOR_MIN));
    end;
  end;
  
  // Also check WOW6432Node for edge cases
  if not Result then
  begin
    if RegQueryDWordValue(HKLM, VC_REDIST_KEY_WOW, 'Major', Major) then
    begin
      if RegQueryDWordValue(HKLM, VC_REDIST_KEY_WOW, 'Minor', Minor) then
      begin
        Result := (Major > VC_REDIST_MAJOR_MIN) or 
                  ((Major = VC_REDIST_MAJOR_MIN) and (Minor >= VC_REDIST_MINOR_MIN));
      end;
    end;
  end;
end;

function InitializeSetup(): Boolean;
var
  VCWarning: String;
begin
  Result := True;
  
  // Check for VC++ Runtime
  if not IsVCRedistInstalled() then
  begin
    VCWarning := 'The Microsoft Visual C++ 2015-2022 Redistributable (x64) may not be installed.' + #13#10 + #13#10 +
                 'The Jamulus VST3 plugin requires this runtime to function correctly.' + #13#10 + #13#10 +
                 'If the plugin fails to load in your DAW, please download and install ' +
                 'the VC++ Redistributable from:' + #13#10 +
                 'https://aka.ms/vs/17/release/vc_redist.x64.exe' + #13#10 + #13#10 +
                 'Do you want to continue with the installation?';
    
    Result := MsgBox(VCWarning, mbConfirmation, MB_YESNO or MB_DEFBUTTON1) = IDYES;
  end;
end;

function InitializeUninstall(): Boolean;
begin
  Result := True;
  // Confirm uninstall
  if MsgBox('Are you sure you want to uninstall {#MyAppName}?' + #13#10 + #13#10 +
            'Your DAW will no longer be able to load this plugin.', 
            mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDNO then
  begin
    Result := False;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    // Post-install actions can be added here
    // For example, logging installation success
    Log('Jamulus VST3 installation completed successfully.');
  end;
end;
