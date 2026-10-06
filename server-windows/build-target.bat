@echo off
REM build-target.bat <executable name>
REM
REM Builds ONE program and runs it — unlike build.bat, which builds every
REM target in CMakeLists.txt. Made for the Phase 1 staircase: when only
REM the program under test is compiled, any error that shows up belongs
REM to that program and not to an unrelated one.
REM
REM Example:
REM   build-target adapters_test
REM
REM Run this from the Developer Command Prompt for VS, inside
REM server-windows\ — it needs cl.exe on PATH, same as build.bat.

if "%~1"=="" (
    echo Usage:   build-target ^<executable name^>
    echo Example: build-target adapters_test
    exit /b 1
)

REM Step 1 - configure: CMake reads CMakeLists.txt and prepares the build
REM folder. Nothing is compiled yet. The OpenSSL path is needed because
REM the same CMakeLists.txt also describes the network programs.
echo === 1/3 Configure ===
cmake -B build -DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL-Win64"
if errorlevel 1 exit /b 1

REM Step 2 - build: the compiler turns the .cpp into an .exe, for this
REM one target only.
echo.
echo === 2/3 Build %~1 ===
cmake --build build --target %~1
if errorlevel 1 exit /b 1

REM Step 3 - run it, then show the number its main() returned
REM (0 = success, anything else = the program reported an error).
echo.
echo === 3/3 Run %~1.exe ===
build\Debug\%~1.exe
echo.
echo Exit code: %errorlevel%
