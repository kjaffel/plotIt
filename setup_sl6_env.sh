#! /bin/bash

# gcc, gdb, python, root as from CMSSW_7_4_10
echo "Sourcing root"
source /cvmfs/cms.cern.ch/slc6_amd64_gcc491/cms/cmssw/CMSSW_7_4_10/external/slc6_amd64_gcc491/bin/thisroot.sh

echo "Sourcing gcc / boost"
# gdb is already loaded by root
# python is already loaded by root
source /cvmfs/cms.cern.ch/slc6_amd64_gcc491/external/gcc/4.9.1-cms/etc/profile.d/init.sh
source /cvmfs/cms.cern.ch/slc6_amd64_gcc491/external/boost/1.57.0-cms/etc/profile.d/init.sh
export BOOST_ROOT
