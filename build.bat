@echo off
setlocal
REM if "%WAV_VCVARS_LOCATION%"=="" set WAV_VCVARS_LOCATION="C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat"
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
set path=d:\Projects\wav\misc;%path%

set CommonCompilerFlags= -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -EHsc -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -FC

REM -MTd: Defines _DEBUG and _MT
REM -MT: Define _MT (use multithreaded static version of the runtime library)
REM -0d: Disable optimizations
REM -Ox: Enable max optimizations
REM -Zo: Enahnce optimized debugging
REM -Z7: Produce object files with full symbolic information
if NOT "%1"=="Release" (set CommonCompilerFlags=%CommonCompilerFlags% -MTd -Od -Zo -Z7) else (set CommonCompilerFlags=%CommonCompilerFlags% -MT -Ox)

set Defines=-DWAV_WIN32=1
if NOT "%1"=="Release" set Defines=-DWAV_SLOW=1 -DWAV_INTERNAL=1
if "%2"=="Test" set Defines=%Defines% -DWAV_INTERNAL=1

set CommonLinkerFlags= -incremental:no -opt:ref -LIBPATH:"%DXSDK_DIR%/Lib/x86" OLE32.LIB d3d11.lib d3dcompiler.lib Shell32.lib Kernel32.lib Pathcch.lib
set Includes=-I..\code -I..\code\imgui -I..\code\imgui\backends -I "%WindowsSdkDir%Include\um" -I "%WindowsSdkDir%Include\shared" -I "%DXSDK_DIR%Include"
set Sources=..\code\win32_wav.cpp ..\code\imgui\backends\imgui_impl_dx11.cpp ..\code\imgui\backends\imgui_impl_win32.cpp ..\code\imgui\imgui*.cpp

set OutputFile=
if "%1"=="Release" set OutputFile=-Fe..\publish\LilyPod.exe

IF NOT EXIST build mkdir build
IF NOT EXIST build\fonts mkdir build\fonts
IF NOT EXIST publish mkdir publish
IF NOT EXIST publish\fonts mkdir publish\fonts

IF NOT EXIST ffmpeg\win32\ffmpeg.exe CALL dlff.bat
if "%1"=="Release" (
    xcopy ffmpeg\win32\ffmpeg.exe publish\ /Y /Q
    xcopy misc\favicon.ico publish\ /Y /Q
) else (
    xcopy ffmpeg\win32\ffmpeg.exe build\ /Y /Q
    xcopy misc\favicon.ico build\ /Y /Q
)

pushd build

del *.pdb > NUL 2> NUL
del *.dll > NUL 2> NUL
cl %OutputFile% %CommonCompilerFlags% %Defines% %Includes% -Fmwav.map %Sources% /link %CommonLinkerFlags%
endlocal
popd
