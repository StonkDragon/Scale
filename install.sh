if [ `uname` = "Darwin" ]; then
    if ! which gcc >/dev/null; then
        echo "Please install the Xcode command line tools"
        exit 1
    fi
fi

if ! command -v which &> /dev/null; then
    echo "Please install which"
    exit 1
fi

failedACommand=0

if ! which g++ >/dev/null; then
    echo "Please install g++"
    failedACommand=1
fi

if ! which git >/dev/null; then
    echo "Please install git"
    failedACommand=1
fi

if ! which make >/dev/null; then
    echo "Please install make"
    failedACommand=1
fi

if ! which cmake >/dev/null; then
    echo "Please install cmake"
    failedACommand=1
fi

if [ "$failedACommand" = "1" ]; then
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
    build/dragon build
    cd ..
fi
rm -rf Dragon

echo "---------"
echo "Installing..."

install-boehm-gc() {
    git clone https://github.com/ivmai/bdwgc.git bdwgc
    cd bdwgc
    mkdir out
    cd out
    cmake -Dbuild_tests=ON ..
    cmake --build .
    ctest
    sudo make install
}

dragon build -conf install

for f in /opt/Scale/*; do
    if [ -e $f/sclc ]; then
        echo "Linking $f/sclc to sclc-${f:11}"
        sudo rm -rf /usr/local/bin/sclc-${f:11}
        sudo ln -s $f/sclc /usr/local/bin/sclc-${f:11}
    fi
done

version=$(dragon config -get-key VERSION)

sudo rm -f /opt/Scale/latest
sudo ln -s /opt/Scale/$version /opt/Scale/latest

echo "---------"
echo "Done..."
