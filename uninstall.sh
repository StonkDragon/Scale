sudo rm -rf /opt/Scale
sudo rm -rf /usr/local/bin/sclc

VSCODE_EXTENSIONS_PATH=""

if [ -e "$HOME/.vscode/extensions" ]; then
    VSCODE_EXTENSIONS_PATH="$HOME/.vscode/extensions"
elif [ -e "$HOME/.vscode-insiders/extensions" ]; then
    VSCODE_EXTENSIONS_PATH="$HOME/.vscode-insiders/extensions"
fi

if [ ! -z "$VSCODE_EXTENSIONS_PATH" ]; then
    if [ -e "$VSCODE_EXTENSIONS_PATH/scale.vscodeextension" ]; then
        rm -rf "$VSCODE_EXTENSIONS_PATH/scale.vscodeextension"
    fi
fi
