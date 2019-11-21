#! /bin/bash

# YAML

curl -O -L https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.6.2.tar.gz
tar xf yaml-cpp-0.6.2.tar.gz

cd yaml-cpp-yaml-cpp-0.6.2
mkdir build
cd build

cmake -DYAML_CPP_BUILD_TOOLS=OFF -DYAML_CPP_BUILD_CONTRIB=OFF -DCMAKE_INSTALL_PREFIX:PATH=$(pwd)/../../ ..

make -j4
make install

cd ../..
rm yaml-cpp-0.6.2.tar.gz

# TCLAP
curl -L "https://github.com/mirror/tclap/archive/tclap-1-2-1-release-final.tar.gz" -o "tclap-1.2.1.tar.gz"
tar xf tclap-1.2.1.tar.gz
mv tclap-tclap-1-2-1-release-final tclap-1.2.1

cd tclap-1.2.1

./autotools.sh
./configure --prefix=$PWD/../

make -j4
make install

cd ..
rm tclap-1.2.1.tar.gz
