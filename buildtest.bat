@echo off
setlocal
REM if "%WAV_VCVARS_LOCATION%"=="" set WAV_VCVARS_LOCATION="C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat"
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64
set path=d:\Projects\wav\misc;%path%

set CommonCompilerFlags= -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -FC -MTd -Od -Zo -Z7
set CommonLinkerFlags= -incremental:no -opt:ref
set Sources=..\code\test.cpp
pushd build

del *.pdb > NUL 2> NUL
del *.dll > NUL 2> NUL
cl %OutputFile% %CommonCompilerFlags% -Fmwav.map %Sources% /link %CommonLinkerFlags%
endlocal
popd