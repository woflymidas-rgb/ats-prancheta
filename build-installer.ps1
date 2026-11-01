param([string]$Compiler)
$ErrorActionPreference='Stop'
if(!$Compiler){$Compiler="${env:ProgramFiles(x86)}\Inno Setup 7\ISCC.exe"}
if(!(Test-Path $Compiler)){$Compiler="${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"}
if(!(Test-Path $Compiler)){throw 'Install Inno Setup or pass -Compiler with the path to ISCC.exe.'}
& $Compiler "$PSScriptRoot\installer\ATS-Prancheta.iss"
if($LASTEXITCODE){throw 'Installer compilation failed'}
