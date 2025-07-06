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
if exist ".\Release\input_simulator.exe" (
    if exist "..\input_simulator.exe" (
        del "..\input_simulator.exe"
    )
    copy ".\Release\input_simulator.exe" "..\input_simulator.exe"
) else (
    echo Build failed. input_simulator.exe not found.
)

cd ..