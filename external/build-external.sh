#! /bin/bash

# YAML

curl -O -J -L https://github.com/jbeder/yaml-cpp/archive/release-0.5.3.tar.gz
tar xf yaml-cpp-release-0.5.3.tar.gz

patch -p0 < yaml-cpp-cmake-fix.patch

cd yaml-cpp-release-0.5.3
mkdir build
cd build

cmake -DBoost_NO_BOOST_CMAKE=TRUE -DYAML_CPP_BUILD_TOOLS=OFF -DYAML_CPP_BUILD_CONTRIB=OFF -DCMAKE_INSTALL_PREFIX:PATH=../../ ..

make -j4
make install

cd ../..
rm yaml-cpp-release-0.5.3.tar.gz

# TCLAP
curl -L "http://downloads.sourceforge.net/project/tclap/{tclap-1.2.1.tar.gz}?r=http%3A%2F%2Fsourceforge.net%2Fprojects%2Ftclap%2Ffiles%2F&ts=1431017326&use_mirror=freefr" -o "#1"
tar xf tclap-1.2.1.tar.gz

cd tclap-1.2.1
./configure --prefix=$PWD/../

make -j4
make install

cd ..
rm tclap-1.2.1.tar.gz
