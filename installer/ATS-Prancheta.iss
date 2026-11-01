#define AppVersion "1.0.0"
[Setup]
AppId={{85BAC7D0-1B91-4596-8641-B35011C35145}
AppName=ATS Prancheta
AppVersion={#AppVersion}
AppPublisher=Click Taberna
AppPublisherURL=https://steamcommunity.com/id/clicktaberna/
AppSupportURL=https://github.com/woflymidas-rgb/ats-prancheta/issues
DefaultDirName={autopf}\ATS Prancheta
DefaultGroupName=ATS Prancheta
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\dist
OutputBaseFilename=ATS-Prancheta-1.0.0-Setup-r3
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=no
RestartApplications=no
UninstallDisplayIcon={app}\ATSPrancheta.exe
LicenseFile=..\LICENSE
[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"
[CustomMessages]
brazilianportuguese.AppDirectoryHelp=Esta pasta guarda o aplicativo da prancheta. Pode escolher qualquer pasta; ela não precisa ficar dentro da pasta do jogo.
english.AppDirectoryHelp=This folder stores the clipboard application. Choose any folder; it does not need to be inside the game folder.
brazilianportuguese.GameDirectoryTitle=Local do executável do ATS
english.GameDirectoryTitle=ATS executable location
brazilianportuguese.GameDirectorySubtitle=Selecione a pasta que contém amtrucks.exe
english.GameDirectorySubtitle=Select the folder containing amtrucks.exe
brazilianportuguese.GameDirectoryHelp=Feche o ATS e a prancheta antes de instalar. Selecione a pasta do executável do jogo, geralmente: ...\American Truck Simulator\bin\win_x64
english.GameDirectoryHelp=Close ATS and the clipboard before installing. Select the game executable folder, usually: ...\American Truck Simulator\bin\win_x64
brazilianportuguese.GameDirectoryLabel=Pasta que contém amtrucks.exe:
english.GameDirectoryLabel=Folder containing amtrucks.exe:
brazilianportuguese.InvalidGameDirectory=Não encontramos amtrucks.exe nesta pasta. Selecione a pasta ...\American Truck Simulator\bin\win_x64.
english.InvalidGameDirectory=amtrucks.exe was not found in this folder. Select ...\American Truck Simulator\bin\win_x64.
brazilianportuguese.BackupError=Não foi possível preservar o backup da DLL. A instalação foi interrompida.
english.BackupError=The DLL could not be backed up. Installation was stopped.
brazilianportuguese.StartClipboard=Iniciar a prancheta
english.StartClipboard=Start the clipboard
[Files]
Source: "..\build\ATSPrancheta.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\ATSPrancheta.dll"; DestDir: "{code:GetPluginDir}"; Flags: ignoreversion
Source: "..\assets\*"; DestDir: "{app}\assets"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\assets\company-catalog.json"; DestDir: "{code:GetPluginDir}\ATSPrancheta-assets"; Flags: ignoreversion
Source: "..\assets\logos-base\*"; DestDir: "{code:GetPluginDir}\ATSPrancheta-assets\logos-base"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\licenses\*"; DestDir: "{app}\licenses"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\CREDITOS.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
[Icons]
Name: "{group}\ATS Prancheta"; Filename: "{app}\ATSPrancheta.exe"; WorkingDir: "{app}"
Name: "{group}\Desinstalar ATS Prancheta"; Filename: "{uninstallexe}"
[Run]
Filename: "{app}\ATSPrancheta.exe"; Description: "{cm:StartClipboard}"; Flags: nowait postinstall skipifsilent runasoriginaluser
[Code]
var GamePage: TInputDirWizardPage;
function GetPluginDir(Param: String): String;
begin Result := AddBackslash(GamePage.Values[0]) + 'plugins'; end;
function GameValid(Path: String): Boolean;
begin Result := FileExists(AddBackslash(Path) + 'amtrucks.exe'); end;
procedure InitializeWizard;
var Steam, Candidate, Line, Value: String; Lines: TArrayOfString; I, P: Integer;
begin
  WizardForm.SelectDirLabel.Caption := CustomMessage('AppDirectoryHelp');
  WizardForm.SelectDirLabel.AutoSize := False;
  WizardForm.SelectDirLabel.WordWrap := True;
  WizardForm.SelectDirLabel.Height := ScaleY(44);
  WizardForm.SelectDirBrowseLabel.Top := WizardForm.SelectDirLabel.Top + WizardForm.SelectDirLabel.Height + ScaleY(10);
  WizardForm.SelectDirBrowseLabel.AutoSize := False;
  WizardForm.SelectDirBrowseLabel.WordWrap := True;
  WizardForm.SelectDirBrowseLabel.Height := ScaleY(32);
  WizardForm.DirEdit.Top := WizardForm.SelectDirBrowseLabel.Top + WizardForm.SelectDirBrowseLabel.Height + ScaleY(8);
  WizardForm.DirBrowseButton.Top := WizardForm.DirEdit.Top - ScaleY(1);
  GamePage := CreateInputDirPage(wpSelectDir, CustomMessage('GameDirectoryTitle'),
    CustomMessage('GameDirectorySubtitle'), CustomMessage('GameDirectoryHelp'), False, '');
  GamePage.Add(CustomMessage('GameDirectoryLabel'));
  if RegQueryStringValue(HKCU, 'Software\Valve\Steam', 'SteamPath', Steam) then begin
    Candidate := AddBackslash(Steam) + 'steamapps\common\American Truck Simulator\bin\win_x64';
    if GameValid(Candidate) then GamePage.Values[0] := Candidate;
    if LoadStringsFromFile(AddBackslash(Steam) + 'steamapps\libraryfolders.vdf', Lines) then
      for I := 0 to GetArrayLength(Lines)-1 do begin
        Line := Trim(Lines[I]); P := Pos('"path"', Line);
        if P > 0 then begin
          Value := Trim(Copy(Line, P+6, Length(Line)));
          if (Length(Value)>1) and (Value[1]='"') then begin
            Delete(Value,1,1); P := Pos('"',Value);
            if P>0 then begin
              Value := Copy(Value,1,P-1); StringChangeEx(Value,'\\','\',True);
              Candidate := AddBackslash(Value) + 'steamapps\common\American Truck Simulator\bin\win_x64';
              if GameValid(Candidate) then GamePage.Values[0] := Candidate;
            end;
          end;
        end;
      end;
  end;
end;
function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if (CurPageID=GamePage.ID) and not GameValid(GamePage.Values[0]) then begin
    MsgBox(CustomMessage('InvalidGameDirectory'), mbError, MB_OK); Result := False;
  end;
end;
function PrepareToInstall(var NeedsRestart: Boolean): String;
var Target, Backup: String;
begin
  Result := '';
  if not GameValid(GamePage.Values[0]) then begin Result := CustomMessage('InvalidGameDirectory'); Exit; end;
  Target := AddBackslash(GetPluginDir('')) + 'ATSPrancheta.dll';
  if FileExists(Target) then begin
    Backup := Target + '.' + GetDateTimeString('yyyymmddhhnnsszzz','-',':') + '.bak';
    if not CopyFile(Target, Backup, True) then Result := CustomMessage('BackupError');
  end;
end;
