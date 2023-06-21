set +xe

git clone https://github.com/ivmai/bdwgc.git bdwgc
cd bdwgc
mkdir out
cd out
cmake -Dbuild_tests=ON ..
cmake --build .
ctest
sudo make install

for f in /opt/Scale/*; do
    if [ -e $f/sclc ] && [ $f != "/opt/Scale/latest" ]; then
        echo "Linking $f/sclc to sclc-${f:11}"
        sudo rm -rf /usr/local/bin/sclc-${f:11}
        sudo ln -s $f/sclc /usr/local/bin/sclc-${f:11}
    fi
done

version=$(dragon config -get-key VERSION)

sudo rm -f /opt/Scale/latest
sudo ln -s /opt/Scale/$version /opt/Scale/latest
