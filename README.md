# ATS Prancheta

[English](#english) · [Português](#português)

## English

An open-source 2D clipboard with persistent transport documents for American Truck Simulator.
By [Click Taberna](https://steamcommunity.com/id/clicktaberna/).
Project code: MIT. Dependencies and media retain their own terms.

### Status and installation

Source code is available here. The installer has compiled successfully but still needs
installation validation before a public release. No installer release is published yet.
This is a native telemetry DLL and an external Windows overlay, not a .scs Workshop mod.
No Python, application account or network connection is needed while playing.

F8 toggles the clipboard; Ctrl+F8 changes the shortcut. Existing game bindings are allowed.
New documents open automatically. Data stays fixed during the trip and is stored per profile.
Windows x64 / ATS 1.61.3.18 tested. VR, other platforms and World of Trucks not validated.
Language follows the ATS startup log; 43 locale variants included.

### Build and audit

Install Visual Studio Build Tools 2022 with Desktop development with C++ and Windows SDK.
Run `./build.ps1 -RunTests` in PowerShell. Optionally pass `-ToolchainRoot` to the
VC/Tools/MSVC/<version> directory. Output: build/ATSPrancheta.dll and ATSPrancheta.exe.
Dependencies and their licenses are included. Tests use synthetic data and never start ATS.
To build setup, install Inno Setup and run `./build-installer.ps1 -Compiler <path-to-ISCC.exe>`.
See [installer behavior](docs/INSTALLER.md). Identical binaries across toolchains are not promised.

The plugin receives telemetry and reads the loaded save in memory to distinguish new jobs
from restores. It does not modify saves, controls, cargo weights or physics. The overlay
renders via GDI+ and reads shortcuts using Win32. No network requests or credentials are used.
Data stays in `%LOCALAPPDATA%/ATSPrancheta/prototype-single-profile`.
Open source enables inspection; it is not a digital signature or a security certification.

MIT covers our code, not SCS game resources or third-party media. See CREDITOS.txt and licenses/.
Audio files are excluded from this source upload while redistribution conditions are reviewed.
Missing audio does not prevent displaying documents; playback fails silently.
Real-company logos from the third-party mod used during development are not redistributed.
[Truck Manager notes](docs/MOD-MANAGERS.md): integration is not confirmed; both default to F8.

## Português

Documento de transporte em prancheta 2D para American Truck Simulator.
Autor: [Click Taberna](https://steamcommunity.com/id/clicktaberna/).
Código do projeto sob MIT; dependências e recursos possuem condições próprias.

## Instalar

Use uma versão publicada em Releases. Esta aplicação é uma DLL de telemetria e um
visor Windows externo; não é um arquivo .scs e não se instala por inscrição no Workshop.
Não há exigência de Python para jogar, conexão de rede ou conta neste aplicativo.
O instalador foi compilado, mas ainda aguarda validação de instalação. Não há release pública neste momento.

F8 abre/fecha por padrão; Ctrl+F8 configura a tecla, inclusive teclas usadas pelo jogo.
Novo documento abre automaticamente. Registros são fixos e separados por perfil.
Windows x64 / ATS 1.61.3.18 testado. Outros sistemas, VR e World of Trucks não validados.
O idioma segue o registro de inicialização do ATS;43 variantes incluídas.

## Compilar e conferir

Instale Visual Studio Build Tools2022 com C++Desktop e Windows10/11 SDK.
No PowerShell, dentro desta pasta: `./build.ps1 -RunTests`.
Alternativamente informe `-ToolchainRoot` para a pasta VC/Tools/MSVC/<versão>.
Saída: build/ATSPrancheta.dll e build/ATSPrancheta.exe.
SDK1.16 e leitor SII estão incluídos com licenças; não é necessário fornecer saves.
Os testes usam dados sintéticos em diretórios próprios e não iniciam o ATS.
Não se promete igualdade byte a byte entre compiladores diferentes.

## Transparência

O plugin recebe telemetria e lê cópias/em memória do save carregado para distinguir
contratação nova de restauração. Não altera saves/controles/pesos/física do ATS.
O visor lê os registros locais, desenha via GDI+ e consulta as teclas por Win32.
Dados ficam em %LOCALAPPDATA%/ATSPrancheta/prototype-single-profile.
O código da distribuição não realiza requisições de rede nem inclui credenciais.
Abrir o código facilita auditoria; não equivale a assinatura digital ou prova de segurança.
Não desative antivírus para instalar. Reporte falsos positivos com versão/hash do arquivo.

## Recursos e dependências

MIT aplica-se ao código próprio. SDK, SII-Decrypt-cpp e nlohmann/json mantêm suas
licenças em licenses e nas respectivas pastas. Traduções/nomes/logos oficiais extraídos
do ATS não são relicenciados por este LICENSE. A arte da prancheta foi gerada para
este projeto; áudios e suas fontes constam em CREDITOS.txt. A permissão de redistribuição
do Page-flip ainda deve ser esclarecida antes da release pública.
Logos do mod de empresas reais usado nos testes não estão incluídos.
Os MP3s ficam fora deste envio de código enquanto as condições de redistribuição são conferidas; a ficha funciona sem áudio.

## Gerenciadores

Veja docs/MOD-MANAGERS.md. Não há integração com Truck Manager confirmada.
