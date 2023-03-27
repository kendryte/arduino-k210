#!/bin/bash

# usage
#   1. clone rt-thread lastest repo
#   2. copy lib/.config & lib/rtconfig.h to rtt/bsp/k210
#   3. copy this script to rtt/bsp/k210
#   4. execute

set -e

LIB_PATH=./lib

rm -rf ${LIB_PATH} && mkdir ${LIB_PATH}

cp .config rtconfig.h ${LIB_PATH}

scons --clean && scons --verbose >  ${LIB_PATH}/build.log 2>&1 | tee

libs=("Compiler" "CPlusPlus" "CPU" "DeviceDrivers" "Kernel")

for lib in ${libs[@]};do
    scons --buildlib=${lib} --verbose > ${LIB_PATH}/build_${lib}.log 2>&1 | tee
    mv lib${lib}_gcc.a ${LIB_PATH}
done

echo "Done."

set +e
