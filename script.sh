#!/bin/bash

module load libraries/openmpi-1.6-gcc-4.4.6
mpirun ./main $1 $2
