ContourStudio
=============

Open ContourStudio.sln in Visual Studio 2022.
Supported build: Release | x64, toolset v143.
Build command (Developer PowerShell):
  msbuild ContourStudio.sln /t:Build /p:Configuration=Release /p:Platform=x64 /m

Output: build\Release\FREE_FOR_YOU.exe
Intermediate files: build\obj\x64\Release\

src\application_entry.cpp   Application startup and DirectX lifecycle
src\main_window.cpp         User interface and shortcut state
src\window_lifecycle.cpp    Restore game state and request window close
src\head_visualization.h    Head, AREA ring and Size fan drawing
src\runtime\               Feature controllers and configuration
src\widgets\               Shared UI controls
src\helpers\               Drawing, fonts and texture support
src\settings\              Theme, layout and application state
src\assets\                Current head image and embedded resource
src\data\                  Embedded fonts and source fonts
thirdparty\                External libraries; preserve license notices
docs\                      Rename history and cleanup record
build\                     Generated outputs, ignored by version control

build\rename-backup contains the pre-rename source backup.
This backup is not compiled or distributed with the application.

Current limitations:
- Hard Head configuration is incomplete.
- AREA's in-game interpretation has not been confirmed.
- Successful compilation does not establish in-game behavior or malware safety.
Convenience build:
  powershell -File scripts\build.ps1
  powershell -File scripts\build.ps1 -Rebuild

Code style:
  .editorconfig defines encoding, indentation and line endings.
  .clang-format defines C++ formatting.
  Format application code only; skip thirdparty, src/runtime/vendor,
  embedded resource arrays in src/assets and font data in src/data.