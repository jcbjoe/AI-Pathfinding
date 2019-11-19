@echo off

rem Gather arguments
set my_dir=%~dp0
set target_name=%1
set debug_mode=%2
set targets=%my_dir%unity_common.cpp %my_dir%..\..\%target_name%\build\unity_%target_name%.cpp

rem All trailing arguments are the cpp files to compile
:loop
if "%3"=="" goto after_loop
set targets=%targets% %3
shift
goto loop
:after_loop

@echo Building %target_name% - Windows %debug_mode%

setlocal enabledelayedexpansion

rem Call vswhere.exe and get the path to the latest version installed
for /f "usebackq tokens=*" %%i in (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do set vc_dir=%%i

rem Load version file if it exists and trim to make subdirectory
set vc_version_file="%vc_dir%\VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt"
if exist %vc_version_file% (
  set /p vc_version=<%vc_version_file%
  set vc_version=!vc_version: =!
)

rem Check if path was found
if not "%vc_version%"=="" (
  set vcpath="%vc_dir%\VC\Tools\MSVC\%vc_version%"
  goto compiler_found
)

@echo Supported Visual C++ compiler not installed
exit /b

:compiler_found

rem Find the latest Windows 10 SDK
set sdk_include_root=C:\Program Files (x86)\Windows Kits\10\Include\
set sdk_lib_root=C:\Program Files (x86)\Windows Kits\10\Lib\
set sdk_test_file=\um\d3d12.h

set sdk_version=10.0.17763.0
if exist "%sdk_include_root%%sdk_version%%sdk_test_file%" goto winsdk_found

set sdk_version=10.0.17134.0
if exist "%sdk_include_root%%sdk_version%%sdk_test_file%" goto winsdk_found

@echo Supported Windows 10 SDK not installed
exit /b

:winsdk_found

echo Visual C++ compiler version: %vc_version%
echo Windows 10 SDK version: %sdk_version%
set sdk_include=%sdk_include_root%%sdk_version%
set sdk_lib="%sdk_lib_root%%sdk_version%\um\x64"
set ucrt_lib="%sdk_lib_root%%sdk_version%\ucrt\x64"
set vcpath=%vcpath:"=%
set vc_compiler="%vcpath%\bin\HostX64\x64\cl.exe"
set vc_include="%vcpath%\include"
set vc_lib="%vcpath%\lib\x64"

rem Change directory to the batch file directory
cd /D "%my_dir%"

rem Create build output and object directories/Zi 
mkdir "%my_dir%..\..\%target_name%\build\win_%debug_mode%\obj" 2>nul

set optimization=/O2 /GL /D NDEBUG
if %debug_mode%==debug set optimization=/Od

rem Invoke compiler
%vc_compiler% ^
%optimization% ^
/Zi ^
/W3 /wd4200 ^
/std:c++latest /permissive- ^
/Oi /GR- /GS- /DYNAMICBASE:NO /fp:fast ^
/MP /nologo ^
/D _HAS_EXCEPTIONS=0 /D _ITERATOR_DEBUG_LEVEL=0 /D _CRT_SECURE_NO_WARNINGS ^
/Fd"%my_dir%..\..\%target_name%\build\win_%debug_mode%\%target_name%.pdb" ^
/Fo"%my_dir%..\..\%target_name%\build\win_%debug_mode%\obj\\" ^
/Fe"%my_dir%..\..\%target_name%\build\win_%debug_mode%\%target_name%.exe" ^
/Fm"%my_dir%..\..\%target_name%\build\win_%debug_mode%\%target_name%.map" ^
/I %vc_include% /I "%sdk_include%\um" /I "%sdk_include%\ucrt" /I "%sdk_include%\shared" ^
%targets% ^
/link /OPT:ref /subsystem:windows /entry:"main" /incremental:no /NODEFAULTLIB ^
kernel32.lib user32.lib ucrt.lib vcruntime.lib msvcrt.lib dxgi.lib d3d11.lib dxguid.lib d3dcompiler.lib ole32.lib ^
/LIBPATH:%vc_lib% /LIBPATH:%sdk_lib% /LIBPATH:%ucrt_lib%