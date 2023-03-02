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

if [ ! -e ./Dragon/build/dragon ]; then
    echo "---------"
    echo "Downloading dragon..."
    git clone https://github.com/StonkDragon/Dragon
    cd Dragon
    make compile
    sudo cp build/dragon /usr/local/bin/dragon
    cd ..
fi

echo "---------"
echo "Installing..."

./Dragon/build/dragon build -conf install

echo "---------"
echo "Checking for VSCode installation..."

VSCODE_EXTENSIONS_PATH=""

if [ -e "$HOME/.vscode/extensions" ]; then
    VSCODE_EXTENSIONS_PATH="$HOME/.vscode/extensions"
elif [ -e "$HOME/.vscode-insiders/extensions" ]; then
    VSCODE_EXTENSIONS_PATH="$HOME/.vscode-insiders/extensions"
fi

if [ ! -z "$VSCODE_EXTENSIONS_PATH" ]; then
    echo "Found VSCode extension folder at '$VSCODE_EXTENSIONS_PATH'"
    echo "Is this correct? [yn]"
    read answer
    if [ "$answer" != "${answer#[Yy]}" ]; then
        cp -r ./scale.vscodeextension "$VSCODE_EXTENSIONS_PATH"
    fi
fi
