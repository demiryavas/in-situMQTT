#!/bin/sh

TEST_RUNTIME=`pwd`/../lib; export TEST_RUNTIME

# load libraries from here
LD_LIBRARY_PATH=$TEST_RUNTIME:$LD_LIBRARY_PATH; export LD_LIBRARY_PATH

echo $LD_LIBRARY_PATH

# run the executable
# If an argument is given, set it as home path, else, use current directory
./bin/testhead $1

