; ohmflow.iss — instalador do OhmFlow para o usuário final (Inno Setup 6)
;
; Gera dist\ohm_flow_setup.exe a partir do dist\instalador\ohmflow.exe já compilado.
; O programa só depende de DLLs do Windows 10/11 (KERNEL32, USER32 e UCRT),
; então o setup apenas copia os arquivos, cria atalhos e registra o
; desinstalador: nada de MSYS2, gcc ou internet na máquina de quem instala.
;
; Uso: make setup [VERSAO=x.y.z]
;      (chama: iscc /DVersao=x.y.z /DExeOrigem=dist/instalador/ohmflow.exe installer\ohmflow.iss)

#ifndef Versao
  #define Versao "0.0.0-dev"
#endif
; make setup passa o exe compilado com -DOHM_LOGS_APPDATA (logs em
; %APPDATA%\OhmFlow\logs, já que a pasta de instalação pode não aceitar escrita)
#ifndef ExeOrigem
  #define ExeOrigem "dist\ohmflow.exe"
#endif

[Setup]
; AppId fixo: uma versão nova instala por cima da antiga, no mesmo lugar
AppId={{8F3C2A71-5D4B-4E9A-B6C1-2A7E9D0F4B53}
AppName=OhmFlow
AppVersion={#Versao}
AppPublisher=Grolla05
AppPublisherURL=https://github.com/Grolla05/OhmFlow
AppSupportURL=https://github.com/Grolla05/OhmFlow/issues
AppUpdatesURL=https://github.com/Grolla05/OhmFlow/releases
; Caminhos abaixo são relativos à raiz do repositório
SourceDir=..
OutputDir=dist
OutputBaseFilename=ohm_flow_setup
SetupIconFile=src\assets\ohmflow.ico
LicenseFile=LICENSE
; Sem admin por padrão (%LOCALAPPDATA%\Programs\OhmFlow); o diálogo inicial
; permite instalar para todos os usuários (Arquivos de Programas)
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
DefaultDirName={autopf}\OhmFlow
DefaultGroupName=OhmFlow
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\ohmflow.exe
; UCRT nativa só a partir do Windows 10; o exe é x64
MinVersion=10.0
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Avisa o Windows quando a tarefa "path" mexe na variável de ambiente
ChangesEnvironment=yes
WizardStyle=modern
Compression=lzma2/max
SolidCompression=yes

[Languages]
Name: "ptbr"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[CustomMessages]
ptbr.TarefaPath=Adicionar o OhmFlow ao PATH (rodar "ohmflow" em qualquer terminal)
ptbr.GrupoOutras=Outras opções:

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "path"; Description: "{cm:TarefaPath}"; GroupDescription: "{cm:GrupoOutras}"; Flags: unchecked

[Files]
Source: "{#ExeOrigem}"; DestDir: "{app}"; DestName: "ohmflow.exe"; Flags: ignoreversion
Source: "README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\OhmFlow"; Filename: "{app}\ohmflow.exe"; WorkingDir: "{app}"
Name: "{group}\{cm:UninstallProgram,OhmFlow}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\OhmFlow"; Filename: "{app}\ohmflow.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\ohmflow.exe"; Description: "{cm:LaunchProgram,OhmFlow}"; WorkingDir: "{app}"; Flags: postinstall nowait skipifsilent

[Code]
// PATH: usuário (HKCU\Environment) ou máquina, conforme o modo de instalação
procedure RaizPath(var Raiz: Integer; var Chave: String);
begin
  if IsAdminInstallMode then begin
    Raiz := HKEY_LOCAL_MACHINE;
    Chave := 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';
  end else begin
    Raiz := HKEY_CURRENT_USER;
    Chave := 'Environment';
  end;
end;

// Acrescenta {app} ao fim do PATH, se ainda não estiver lá
procedure AdicionaPath;
var
  Raiz: Integer;
  Chave, Path, Dir: String;
begin
  RaizPath(Raiz, Chave);
  Dir := ExpandConstant('{app}');
  if not RegQueryStringValue(Raiz, Chave, 'Path', Path) then
    Path := '';
  if Pos(';' + Uppercase(Dir) + ';', ';' + Uppercase(Path) + ';') > 0 then
    exit;
  if (Path <> '') and (Path[Length(Path)] <> ';') then
    Path := Path + ';';
  RegWriteExpandStringValue(Raiz, Chave, 'Path', Path + Dir);
end;

// Tira {app} do PATH, mantendo as outras entradas como estavam
procedure RemovePath;
var
  Raiz: Integer;
  Chave, Path, Dir: String;
begin
  RaizPath(Raiz, Chave);
  Dir := ExpandConstant('{app}');
  if not RegQueryStringValue(Raiz, Chave, 'Path', Path) then
    exit;
  Path := ';' + Path + ';';
  if StringChangeEx(Path, ';' + Dir + ';', ';', True) > 0 then
    RegWriteExpandStringValue(Raiz, Chave, 'Path', Copy(Path, 2, Length(Path) - 2));
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and WizardIsTaskSelected('path') then
    AdicionaPath;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
    RemovePath;
end;
