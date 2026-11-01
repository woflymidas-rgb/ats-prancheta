# Installer and companion lifecycle

Build application with build.ps1 -RunTests, then use build-installer.ps1 with Inno Setup.
Setup r4 installs the DLL/assets, the overlay and a plugin-local ATSPrancheta-launch.ini
containing the exact overlay executable path. Only that configured application is launched,
using CreateProcess with an explicit executable, no shell and no inherited handles.

After successful telemetry initialization, the DLL starts the overlay with the game PID
and creation time. If the overlay is already running, the DLL asks it to attach to that
same game instance. The overlay validates process name and creation time, opens a process
handle, and exits when the process ends. PID reuse cannot attach it to another program.
A dedicated process fixture tests lifetime detection without starting the real ATS.
SDK reinitialization does not require another overlay window. No Windows service,
registry autorun entry or scheduled task is added. The manual shortcut is a fallback.

Steam/libraryfolders detection suggests the directory containing amtrucks.exe. The user
confirms it; it is independent of the application installation folder. A prior DLL is
backed up with a timestamp. Documents and user settings are preserved on uninstall.
Close ATS and the old overlay before upgrading. Setup does not terminate either process.
The post-install "start overlay" checkbox was removed: the next ATS launch starts it.

Status: setup compiles; game opening/closing lifecycle requires manual confirmation.

Setup r5: Start menu and desktop shortcuts are independent optional tasks, unchecked on a fresh install. The program-group selection page is hidden. Automatic launch with ATS does not depend on either shortcut. Uninstall remains available in Windows installed apps without shortcuts.
