# SuperLauncher Windows release build.
# Requirements: Qt 6.4 (with windeployqt in PATH), CMake, Visual Studio.
# Run from the repo root: powershell -ExecutionPolicy Bypass -File scripts\build_windows.ps1
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE) { exit 1 }
cmake --build build --config Release
if ($LASTEXITCODE) { exit 1 }
windeployqt --release --no-translations --compiler-runtime build\Release\SuperLauncher.exe
if ($LASTEXITCODE) { exit 1 }
Write-Host "Done: build\Release\SuperLauncher.exe"
