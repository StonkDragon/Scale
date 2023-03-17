if [[ ":$PATH:" != *":/usr/local/bin:"* ]] && [[ ":$PATH:" != *":/usr/local/bin/:"* ]]; then
    echo "Your path is missing /usr/local/bin, you might want to add it."
    exit 1
fi

if [ `uname` = "Darwin" ]; then
    if ! which gcc >/dev/null; then
        echo "Please install the Xcode command line tools"
        exit 1
    fi
fi

if ! which g++ >/dev/null; then
    echo "Please install g++"
    exit 1
fi

set -e

if ! which dragon >/dev/null; then
    echo "---------"
    echo "Downloading dragon..."
    git clone https://github.com/StonkDragon/Dragon
    cd Dragon

    if ! which make >/dev/null; then
        echo "Please install make"
        exit 1
    fi

    echo "Installing dragon. This may require root privileges..."

    make compile
    sudo cp build/dragon /usr/local/bin/dragon
    cd ..
    rm -rf Dragon
fi

echo "---------"
echo "Installing..."

dragon build -conf install

for f in /opt/Scale/*; do
    if [ -e $f/sclc ]; then
        echo "Linking $f/sclc to sclc-${f:11}"
        sudo rm -rf /usr/local/bin/sclc-${f:11}
        sudo ln -s $f/sclc /usr/local/bin/sclc-${f:11}
    fi
done

echo "---------"
echo "Checking for VSCode installation..."

VSCODE_EXTENSIONS_PATH=""

if [ -e "$HOME/.vscode/extensions" ]; then
    VSCODE_EXTENSIONS_PATH="$HOME/.vscode/extensions"
elif [ -e "$HOME/.vscode-insiders/extensions" ]; then
    VSCODE_EXTENSIONS_PATH="$HOME/.vscode-insiders/extensions"
else
    echo "No VSCode installation found"
fi

if [ ! -z "$VSCODE_EXTENSIONS_PATH" ]; then
    echo "Found VSCode extension folder at '$VSCODE_EXTENSIONS_PATH'"
    cp -r ./scale.vscodeextension "$VSCODE_EXTENSIONS_PATH"
fi
