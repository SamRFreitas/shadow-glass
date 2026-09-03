@echo off
REM dev-shell.bat
REM
REM Opens a new Developer Command Prompt for VS (cl.exe already on PATH,
REM same as opening it from the Start Menu) already positioned at this
REM project's root folder — no more typing "cd ..." every time.
REM
REM vswhere.exe ships with every modern Visual Studio installer and is
REM the supported way to ask "where is Visual Studio actually installed"
REM without hardcoding a path/version/edition. VsDevCmd.bat (inside that
REM install) is the exact script the Start Menu shortcut runs to set up
REM the compiler environment — this just automates that same call.
REM
REM %~dp0 = the folder this .bat file itself lives in. Using it instead
REM of a hardcoded path means this keeps working even if the project
REM folder gets moved or renamed.

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
    echo Could not find vswhere.exe - is Visual Studio installed?
    pause
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property installationPath`) do set "VSINSTALL=%%i"

if not defined VSINSTALL (
    echo vswhere.exe did not report a Visual Studio installation.
    pause
    exit /b 1
)

start "Shadow Glass dev shell" cmd /k "call "%VSINSTALL%\Common7\Tools\VsDevCmd.bat" -arch=amd64 && cd /d "%~dp0""
