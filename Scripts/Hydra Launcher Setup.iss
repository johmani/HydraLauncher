#define MyAppName "Hydra Launcher"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Muhammad ibn al-walid"
#define MyAppURL "https://github.com/johmani"
#define MyAppExeName "HydraLauncher.exe"

[Setup]

AppId={{19EC4E00-49F1-4226-9917-9A7BDEDD9646}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes


OutputDir= "..\Build\Out\Dist"
OutputBaseFilename=Hydra Launcher-Setup
SetupIconFile=..\Resources\Icons\Hydra.ico
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\Build\Out\Dist\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Build\Out\Dist\HydraEngine.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Build\Out\Dist\imgui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Build\Out\Dist\msvcp140.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Build\Out\Dist\msvcp140_atomic_wait.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Build\Out\Dist\nvrhi.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Build\Out\Dist\vcruntime140.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Build\Out\Dist\vcruntime140_1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Build\Out\Dist\glfw.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Build\Out\Dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\Build\Out\Dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

