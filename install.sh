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

set -xe

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

dragon build -conf install

sh install_post.sh

echo "---------"
echo "Done..."
