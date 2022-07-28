#!/bin/sh

echo "Installing..."

rm -rf ~/Scale

echo "Copying files"
cp -r Scale ~
echo "Adding to PATH"
export PATH="$PATH:~/Scale/bin"

echo "Installed!"
