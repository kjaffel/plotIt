plotIt
======

An utility to plot ROOT histograms.

## First time setup instructions

```bash
git clone -o upstream git@github.com:cp3-llbb/plotIt.git
cd plotIt/

# Initialize the git remotes
source firstsetup.sh 
# Load the proper environment (a la CMSSW_7_4_10)
source setup_sl6_env.sh

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
