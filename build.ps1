param([string]$ToolchainRoot=$env:VCToolsInstallDir,[string]$WindowsSdkRoot=${env:WindowsSdkDir},[switch]$RunTests)
$ErrorActionPreference='Stop'
if(!$ToolchainRoot){
    $vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if(!(Test-Path $vswhere)){throw 'Install Visual Studio Build Tools with Desktop development with C++, or pass -ToolchainRoot.'}
    $vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $ToolchainRoot=(Get-ChildItem "$vs\VC\Tools\MSVC" -Directory | Sort-Object Name -Descending | Select-Object -First 1).FullName
}
if(!$WindowsSdkRoot){$WindowsSdkRoot="${env:ProgramFiles(x86)}\Windows Kits\10"}
$kit=(Get-ChildItem "$WindowsSdkRoot\Include" -Directory | Where-Object {Test-Path "$($_.FullName)\um\Windows.h"} | Sort-Object Name -Descending | Select-Object -First 1).Name
$compiler=Join-Path $ToolchainRoot 'bin\Hostx64\x64\cl.exe'
$librarian=Join-Path $ToolchainRoot 'bin\Hostx64\x64\lib.exe'
if(!(Test-Path $compiler) -or !$kit){throw 'MSVC x64 / Windows SDK not found.'}
$env:INCLUDE="$ToolchainRoot\include;$WindowsSdkRoot\Include\$kit\ucrt;$WindowsSdkRoot\Include\$kit\shared;$WindowsSdkRoot\Include\$kit\um"
$env:LIB="$ToolchainRoot\lib\x64;$WindowsSdkRoot\Lib\$kit\ucrt\x64;$WindowsSdkRoot\Lib\$kit\um\x64"
Push-Location $PSScriptRoot
try {
    New-Item -ItemType Directory build\reader -Force | Out-Null
    $readerRoot='sii-reader-source\SII-Decrypt-cpp-main\src'
    $readerFiles=@(Get-ChildItem $readerRoot -Recurse -Filter '*.cpp' | Where-Object {$_.Name -notin @('sii_console.cpp','sii_dll.cpp')} | Select-Object -ExpandProperty FullName)
    & $compiler /nologo /c /MT /O2 /EHsc /std:c++17 /D_CRT_SECURE_NO_WARNINGS /Fo"build\reader\\" /I "$readerRoot\crypto" /I "$readerRoot\compress" /I "$readerRoot\core" $readerFiles
    if($LASTEXITCODE){throw 'Reader compilation failed'}
    $objects=@(Get-ChildItem build\reader -Filter '*.obj' | Select-Object -ExpandProperty FullName)
    & $librarian /nologo /OUT:build\reader-core.lib $objects
    if($LASTEXITCODE){throw 'Reader library failed'}
    $common=@('/nologo','/MT','/O2','/W4','/WX','/EHsc','/utf-8','/std:c++17','/I','sdk-1.16\include')
    & $compiler @common /LD /Fobuild\production.obj production.cpp /link build\reader-core.lib /DEF:production.def /OUT:build\ATSPrancheta.dll
    if($LASTEXITCODE){throw 'Plugin build failed'}
    & $compiler @common /Fobuild\viewer.obj viewer.cpp /link /SUBSYSTEM:WINDOWS /OUT:build\ATSPrancheta.exe
    if($LASTEXITCODE){throw 'Viewer build failed'}
    if($RunTests){foreach($name in @('document-check','plugin-check','settings-check','localization-check','audio-check','companion-check')) {
        & $compiler @common /UNDEBUG "/Fobuild\$name.obj" "$name.cpp" /link build\reader-core.lib "/OUT:build\$name.exe"
        if($LASTEXITCODE){throw "Test build failed: $name"}
        if($name -eq 'companion-check'){
            New-Item -ItemType Directory build\lifecycle-fixture -Force | Out-Null
            Copy-Item build\companion-check.exe build\lifecycle-fixture\amtrucks.exe -Force
        }
        & ".\build\$name.exe"
        if($LASTEXITCODE){throw "Test failed: $name"}
    }}
} finally {Pop-Location}
