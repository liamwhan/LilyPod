@echo off
REM Downloads ffmpeg binaries for win32

echo Checking 7zip (7z.exe) is available in the system path
for %%X in (7z.exe) do (set FOUND7Z=%%~$PATH:X)
IF NOT DEFINED FOUND7Z (
    ECHO Required dependency not found: 7zip
    ECHO Please download and install all dependencies before running this script
    exit /B
)
echo 7zip is installed

IF NOT EXIST ffmpeg mkdir ffmpeg
IF NOT EXIST ffmpeg\win32 mkdir ffmpeg\win32

echo =============================
echo Downloading ffmpeg 4.4 win32
echo =============================
curl -L -o ffmpeg\win32\ff.7z https://www.gyan.dev/ffmpeg/builds/packages/ffmpeg-4.4-essentials_build.7z

echo =============================
echo Extracting ff.7z
echo =============================
7z e -r ffmpeg\win32\ff.7z ffmpeg.exe -offmpeg\win32 -aoa

echo ===============================
echo Cleaning up ffmpeg\win32 folder
echo ===============================
del ffmpeg\win32\ff.7z > NUL 2> NUL

echo Done.