@echo off
REM build.bat [executable name]
REM
REM Configures and builds server-windows in one step — the same as
REM running these two commands by hand every time:
REM   cmake -B build -DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL-Win64"
REM   cmake --build build
REM
REM If you pass an executable's name (without .exe), it also runs it
REM right after a successful build — e.g.:
REM   build signaling_test
REM builds everything, then launches build\Debug\signaling_test.exe.
REM
REM Run this from the Developer Command Prompt for VS — it needs cl.exe
REM on PATH, same requirement as running the cmake commands by hand.

cmake -B build -DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL-Win64"
if errorlevel 1 exit /b 1

cmake --build build
if errorlevel 1 exit /b 1

echo.
echo Build complete. Executables are in build\Debug\

if not "%~1"=="" (
    echo.
    echo Running %~1.exe ...
    echo.
    build\Debug\%~1.exe
)
