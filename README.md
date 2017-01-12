plotIt
======

[![Build Status](https://travis-ci.org/cp3-llbb/plotIt.svg)](https://travis-ci.org/cp3-llbb/plotIt)

An utility to plot ROOT histograms.

## First time setup instructions

```bash
git clone -o upstream git@github.com:cp3-llbb/plotIt.git
cd plotIt/

# Initialize the git remotes
source firstsetup.sh 
# Within-CMSSW and on ingrid specific install
cms_env # specific to ingrid, aka 'module purge; module load grid/grid_environment_sl6; module load crab/crab3; module load cms/cmssw;'
cmsenv
source setup_for_cms_env.sh
# For a non-CMSSW and non-ingrid install (beware there is no cmsenv at all in this case):
# source setup_sl6_env.sh

# Build externals
cd external
./build-external.sh
# Build the executable itself
cd ..
make -j 4
```

## Test run (command line)
```bash
# Load the proper environment (if not already done)
source setup_sl6_env.sh
# Create some dumb root files to play with
cd test
root -l -b -q generate_files.C
# Now plot stuff
./../plotIt -o plots/ example.yml
# Go to the plots directory to observe the beautiful plots
```
