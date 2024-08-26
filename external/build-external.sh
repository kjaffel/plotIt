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
curl -L "https://sourceforge.net/projects/tclap/files/tclap-1.4.0-rc1.tar.bz2/download" -o "tclap-1.4.0-rc1.tar.bz2"
tar xf tclap-1.4.0-rc1.tar.bz2
cd tclap-1.4.0-rc1

mkdir build
cd build

cmake ..
cmake --build .
cmake --install . --prefix=$PWD/../../

cd ../..
rm tclap-1.4.0-rc1.tar.bz2
