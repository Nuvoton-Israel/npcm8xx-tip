# Introduction
This project is the Cerberus porting to Nuvoton TIP.

## Important Note
This repo is NOT compile-able by itself.
to get the full sources follow the instructions below.

# TIP firmware local build instructions
1. Set up build environment under WSL2 Ubuntu or Linux
- Install cmake version 3.16.3
- Download ARM GNU toolchain "9-2019-q4-major" and setup toolchain properly. Here is an example of the toolchain setup:
	- Create a folder to hold toolchain, such as "/work/cross-compiler/gcc-arm-none-eabi-9-2019-q4-major"
	- Add toolchain directory into Linux PATH permanently.
		```bash
		export PATH=$PATH:/work/cross-compiler/gcc-arm-none-eabi-9-2019-q4-major/bin
		echo $PATH
		vim ~/.bashrc
		Add this line "export PATH=$PATH:/work/cross-compiler/gcc-arm-none-eabi-9-2019-q4-major/bin" to the end of the file and save it.
		```

2. Get the source
- To get the Cerberus source code, you need to have `repo` installed.
- Details about `repo` can be accessed from the project readme located at https://gerrit.googlesource.com/git-repo/+/refs/heads/master/README.md
	Example `repo` installation:
	```bash
	mkdir ~/bin
	curl https://storage.googleapis.com/git-repo-downloads/repo > ~/bin/repo
	chmod a+rx ~/bin/repo
	PATH=${PATH}:~/bin
	```
- Download the source using HTTP:
	```bash
	mkdir NuvotonCerberus
	cd NuvotonCerberus
	repo init -u https://github.com/Nuvoton-Israel/npcm8xx-tip-fw-manifest.git -b master
	repo sync
	```
- Download the source using SSH:
	```bash
	mkdir NuvotonCerberus
	cd NuvotonCerberus
	repo init -u git@github.com:Nuvoton-Israel/npcm8xx-tip-fw-manifest.git -m default-ssh.xml -b master
	repo sync
	```
3. Build TIP FW
- Build L0:
	Create build folder under source root.
	```bash
	mkdir build_boot
	cd build_boot
	cmake -G Ninja -B . -S ../projects/nuvoton/tip/npcm850/L0
	ninja
	```
- Build L1:
	Create build folder under source root.
	```bash
	mkdir build_cerberus
	cd build_cerberus
	cmake -G Ninja -B . -S ../projects/nuvoton/tip/npcm850/L1
	ninja
	```
Once build is successful, *.bin, *.map. *.elf are generated in folder build_boot and build_cerberus.
