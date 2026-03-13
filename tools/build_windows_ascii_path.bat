@echo off
setlocal
for %%I in ("%~dp0..") do set "PROJECT_ROOT=%%~fI"
set "CMAKE_EXE=%PROJECT_ROOT%\.external-cache\cmake\cmake-3.31.6-windows-x86_64\bin\cmake.exe"

if not exist "%CMAKE_EXE%" (
    set "CMAKE_EXE=cmake"
)

subst X: /d >nul 2>&1
subst X: "%PROJECT_ROOT%"
if errorlevel 1 exit /b 1
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64
if errorlevel 1 exit /b 1
"%CMAKE_EXE%" -S "X:\" -B "X:\build\x-nmake-release" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1
"%CMAKE_EXE%" --build "X:\build\x-nmake-release" --target CookieOnTheRoofX --config Release
if errorlevel 1 exit /b 1
copy /Y "X:\build\x-nmake-release\Cookie On The Roof X.exe" "X:\dist\Cookie On The Roof X.exe" >nul
subst X: /d >nul 2>&1
endlocal
