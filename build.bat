@echo off
rem Builds QuickContinue.SA.asi (32-bit, static CRT) into .\out with the Visual Studio C++ compiler.
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (echo vswhere.exe not found - install Visual Studio with the C++ x86 tools & exit /b 1)
rem vswhere runs as a plain command (not inside for /f): its path contains parentheses, which cmd mangles there.
set "VSTMP=%TEMP%\QuickContinue.SA.vsdir.txt"
"%VSWHERE%" -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%VSTMP%"
set "VSDIR="
set /p VSDIR=<"%VSTMP%"
del "%VSTMP%" 2>nul
if not defined VSDIR (echo Visual Studio with the C++ x86 toolset not found & exit /b 1)
call "%VSDIR%\VC\Auxiliary\Build\vcvars32.bat" >nul || exit /b 1
if not exist out mkdir out
cl /nologo /LD /MT /O2 /W4 /EHsc /std:c++17 /utf-8 src\main.cpp /Fo:out\ /Fe:out\QuickContinue.SA.asi /link /MACHINE:X86 user32.lib shell32.lib || exit /b 1
copy /y data\QuickContinue.SA.ini out\QuickContinue.SA.ini >nul
echo.
echo Built out\QuickContinue.SA.asi
