# Introduction
This project is the Cerberus porting to Nuvoton TIP.

# Getting Started
TODO: Guide users through getting your code up and running on their own system. In this section you can talk about:
1.	Installation process
2.	Software dependencies
3.	Latest releases
4.	API references

# TIP firmware local build instructions
1. Set up build environment under WSL2 Ubuntu or Linux
	- Install cmake version 3.16.3
	- Download ARM GNU toolchain "9-2019-q4-major" and setup toolchain properly.
 	Here is an example of the toolchain setup:
	- Create a folder to hold toolchain.  Such as "/work/cross-compiler/gcc-arm-none-eabi-9-2019-q4-major"
	- Add toolchain directory into Linux PATH permanently.
	  export PATH=$PATH:/work/cross-compiler/gcc-arm-none-eabi-9-2019-q4-major/bin
	  echo $PATH
	  vim ~/.bashrc
	  Add  "export PATH=$PATH:/work/cross-compiler/gcc-arm-none-eabi-9-2019-q4-major/bin" to the end of the file and save it.

2. Get the source
To get the Cerberus source code, you need to have `repo` installed. Details about `repo` can
be accessed from the project readme located at https://gerrit.googlesource.com/git-repo/+/refs/heads/master/README.md

Example `repo` installation:
```bash
mkdir ~/bin
curl https://storage.googleapis.com/git-repo-downloads/repo > ~/bin/repo
chmod a+rx ~/bin/repo
PATH=${PATH}:~/bin
```

Download the source using HTTP:
```bash
mkdir NuvotonCerberus
cd NuvotonCerberus
repo init -u https://NuvotonCerberus@dev.azure.com/NuvotonCerberus/Arbel/_git/Cerberus-on-TIP-manifest -b master
repo sync
```

Download the source using SSH:
```bash
mkdir NuvotonCerberus
cd NuvotonCerberus
repo init -u git@ssh.dev.azure.com:v3/NuvotonCerberus/Arbel/Cerberus-on-TIP-manifest -m default-ssh.xml -b master
repo sync
```
3. Build TIP FW
- Build L0:
Create build folder under source root.
```bash
mkdir build_boot
cd build_boot
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -B . -S ../projects/nuvoton/tip/L0
ninja
```
- Build L1:
Create build folder under source root.
```bash
mkdir build_cerberus
cd build_cerberus
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -B . -S ../projects/nuvoton/tip/L1
ninja
```

Once build is successful, *.bin, *.map. *.elf are generated in folder build_boot and build_cerberus.

# Contribute
TODO: Explain how other users and developers can contribute to make your code better.

If you want to learn more about creating good readme files then refer the following [guidelines](https://docs.microsoft.com/en-us/azure/devops/repos/git/create-a-readme?view=azure-devops). You can also seek inspiration from the below readme files:
- [ASP.NET Core](https://github.com/aspnet/Home)
- [Visual Studio Code](https://github.com/Microsoft/vscode)
- [Chakra Core](https://github.com/Microsoft/ChakraCore)