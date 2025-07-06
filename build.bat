@echo off
cls
if exist build (
    rmdir /s /q build
)
mkdir build
cd build
cmake ..
cmake --build . --config Release

@REM copy the executable to the parent directory
if exist ".\Release\MouseClickSimulator.exe" (
    if exist "..\MouseClickSimulator.exe" (
        del "..\MouseClickSimulator.exe"
    )
    copy ".\Release\MouseClickSimulator.exe" "..\MouseClickSimulator.exe"
) else (
    echo Build failed. MouseClickSimulator.exe not found.
)

cd ..