#!/bin/bash

# This script builds TIP FW (L0 and L1) locally.  It also installs the gcc compiler, if not already installed
# on the local machine.

# Copyright (c) Microsoft Corporation. All rights reserved.
# Copyright (c) Nuvoton Technology Corporation. All rights reserved.


#check if the gcc compiler exists on the build machine
GCC_ARM_COMPILER_VERSION=9-2019-q4-major

export PATH=$PATH:$(pwd)/tools/gcc-arm-none-eabi-$GCC_ARM_COMPILER_VERSION/bin
arm-none-eabi-gcc --version
if [ $? -ne 0 ]; then
	pushd .
	mkdir -p $(pwd)/tools
	cd tools/

	wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2019q4/gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2
	tar -xvf gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2
	export PATH=$PATH:$(pwd)/tools/gcc-arm-none-eabi-$GCC_ARM_COMPILER_VERSION/bin

	popd
fi

#check if the compiler installation was successful.
arm-none-eabi-gcc --version
if [ $? -ne 0 ]; then
	exit 1
fi

pushd .
if [[ -d "build_L0" ]]; then
	rm -rf build_L0
fi

mkdir -p build_L0
cd build_L0

cmake -GNinja ../npcm850/L0
ninja

popd

if [[ -d "build_L1" ]]; then
	rm -rf build_L1
fi

mkdir -p build_L1
cd build_L1

cmake -GNinja ../npcm850/L1
ninja

