# Installer behavior

Inno Setup7/6 builds installer/ATS-Prancheta.iss. Compile application first with
build.ps1, then build-installer.ps1. Output goes to dist/. No downloads occur inside
the installer. It requests administrator access to copy the DLL into protected game
folders. The application itself is launched as the original user, never as admin.

Steam registry and libraryfolders.vdf are inspected to suggest an ATS path. The user
can browse and must confirm the directory directly containing amtrucks.exe, usually ATS/bin/win_x64. The first destination page explicitly describes the independent application folder.
Previous DLL receives a timestamped .bak. User histories/settings are not included
or deleted. An uninstaller and Start-menu shortcuts are provided. Other plugins are
not replaced. User must close ATS and visor first; no process is terminated by setup.
The visor still needs to be running before playing; no autostart service/task is added.

Status: compiled successfully with Inno Setup 7.1.1. Interactive installation validation remains pending. No installer release has been published.