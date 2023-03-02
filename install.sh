if [[ ! -e ./Dragon/build/dragon ]]; then
    echo "---------"
    echo "Downloading dragon..."
    git clone https://github.com/StonkDragon/Dragon
    cd Dragon
    make compile
    cd ..
fi

echo "---------"
echo "Installing..."

./Dragon/build/dragon build -conf install
