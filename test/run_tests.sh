#! /bin/bash

root -l -b -q generate_files.C &> /dev/null

python tests.py
