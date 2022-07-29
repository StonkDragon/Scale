#!/bin/sh

echo "Installing..."

rm -rf ~/Scale

echo "Copying files"
cp -r ./Scale ~/Scale
if [ -f /usr/local/bin/sclc ]; then
    sudo rm -rf /usr/local/bin/sclc
fi
sudo cp ./sclc /usr/local/bin/sclc
sudo chmod +x /usr/local/bin/sclc

echo "Installed!"
