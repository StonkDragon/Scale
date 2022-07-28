@echo off

echo "Installing..."

del /S "%USERPROFILE%\\Scale"

echo "Copying files"
xcopy .\\Scale %USERPROFILE%
echo "Adding to PATH"
setx path "%PATH%;%USERPROFILE%\\Scale\\bin"

echo "Installed!"
