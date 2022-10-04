#! /bin/bash

gcc -Wall -c ../nic_link/src/nic_lib.c -o nic_lib.o
mv nic_lib.o src
cp ../nic_link/src/nic_lib.h src
cd src
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
gcc -Wall -pthread -o sample j_net.c -lnic_lib -lpigpiod_if2
