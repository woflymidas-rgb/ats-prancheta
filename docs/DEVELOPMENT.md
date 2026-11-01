# Development

The player-facing README describes installation and use. For contributors:

- Install Visual Studio Build Tools 2022 with Desktop development with C++ and a Windows SDK.
- Run `./build.ps1 -RunTests` in PowerShell. If needed, pass `-ToolchainRoot` pointing to VC/Tools/MSVC/<version>.
- Outputs: build/ATSPrancheta.dll and build/ATSPrancheta.exe. Tests use isolated synthetic fixtures and do not start the actual game.
- Build the installer with Inno Setup: `./build-installer.ps1 -Compiler <path-to-ISCC.exe>`. See [installer behavior](INSTALLER.md).
- Project code is MIT. Preserve dependency licenses in licenses/, sdk-1.16/ and sii-reader-source/. The project license does not relicense third-party media/game assets. Credits are in ../CREDITOS.txt.
- The companion reads telemetry records and renders with GDI+. The plugin reads the loaded save in memory to distinguish a new job from a restored trip. It does not modify game saves or controls. Stored records are in %LOCALAPPDATA%/ATSPrancheta/prototype-single-profile.
- Public display name: Cargo Clipboard. ATSPrancheta identifiers, filenames and storage paths remain unchanged for compatibility. ETS2 is planned, not currently supported.
