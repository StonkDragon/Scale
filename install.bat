@echo off

echo "Installing..."

del /S "%USERPROFILE%\\Scale"

echo "Copying files"
xcopy .\\Scale %USERPROFILE%\\Scale /E/H/C/I
copy .\\sclc.exe %SystemRoot%\\sclc.exe

echo "Installed!"
