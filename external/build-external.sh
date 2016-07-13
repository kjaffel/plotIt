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
curl -L "https://github.com/eile/tclap/archive/tclap-1-2-1-release-final.tar.gz" -o "tclap-1.2.1.tar.gz"
tar xf tclap-1.2.1.tar.gz
mv tclap-tclap-1-2-1-release-final tclap-1.2.1

cd tclap-1.2.1

./autotools.sh
./configure --prefix=$PWD/../

make -j4
make install

cd ..
rm tclap-1.2.1.tar.gz
