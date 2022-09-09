<img title="Unlock the Power of Fandom" src="https://cdn.discordapp.com/attachments/666275125724839976/1014700582633734224/D0AE2DD9-0B7A-47E6-B1C2-60D39D13005D.gif"><img/>
### Fuego is open-source decentralized P2P privacy cryptocurrency built by sound money advocates and freedom enthusiasts.

Based on the CryptoNote protocol & philosophy.

* <p align="left"><a href="https://fango.money">Website</a><p align="left">
* <p align="left"><a href="http://explorer.fango.money">Explorer</a><p align="left">
* <p align="left"><a href="https://fangotango.hopto.org">Explorer</a><p align="left">
 

 ______________________________
 


##### Master Status   

[![Build Status](https://travis-ci.org/FandomGold/fandomgold.svg?branch=master)](https://travis-ci.org/FandomGold/fandomgold) 

<sup>"Working software is the primary measure of progress." [‣]</sup>


[‣]:http://agilemanifesto.org/

#### Building On *nix

1. Dependencies: GCC 4.7.3 or later, CMake 2.8.6 or later, and Boost 1.55.

You may download them from:

* http://gcc.gnu.org/
* http://www.cmake.org/
* http://www.boost.org/


*** Alternatively, it may be possible to install them using a package manager by
executing the following command.
 ```
 sudo apt-get install build-essential git cmake libboost-all-dev
```

2. Clone Fuego repository
```
git clone https://github.com/FandomGold/fuego

```
3. Open folder with copied repository
```
cd fuego
```
4. Building (Compiling)
    (resulting programs will be found in build/release/src)

```
make
```

5. Starting Fuego daemon
```
cd fuego/build/release/src `
./fuegod
````
try --help from within dæmon for a full list of available commands
or <code>./fuegod --help</code> when outside of dæmon 
_________________________________________________________
For the most user-friendly graphical interface experience

see [Fuego Desktop Wallet](https://github.com/fandomgold/fuego-wallet). 
_________________________________________________________

_________________________________________________________
**Advanced options:**

* Parallel build: run `make -j<number of threads>` instead of `make`.
* Debug build: run `make build-debug`.
* Test suite: run `make test-release` to run tests in addition to building. Running `make test-debug` will do the same to the debug version.
* Building with Clang: it may be possible to use Clang instead of GCC, but this may not work everywhere. To build, run `export CC=clang CXX=clang++` before running `make`.

**************************************************************************************************
### Windows 10

#### Prerequisites

- Install [Visual Studio 2019 Community Edition](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&rel=16)
- Install [CMake](https://cmake.org/download/)
- When installing Visual Studio, you need to install **Desktop development with C++** and the **MSVC v142 - VS 2019 C++ x64/x86 build tools** components. The option to install the v142 build tools can be found by expanding the "Desktop development with C++" node on the right. You will need this for the project to build correctly.
- Install [Boost 1.73.0](https://sourceforge.net/projects/boost/files/boost-binaries/1.73.0/boost_1_73_0-msvc-14.2-64.exe/download), **ensuring** you download the installer for **MSVC 14.2**.

#### Building

From the start menu, open 'x64 Native Tools Command Prompt for vs2019' or run "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsMSBuildCmd.bat" from any command prompt.

```bash
git clone https://github.com/FandomGold/fuego
cd fuego
mkdir build
cmake .. -G "Visual Studio 16 2019" -A x64 -DBOOST_LIBRARYDIR="c:\local\boost_1_73_0\lib64-msvc-14.2"
msbuild fuegoX.sln /p:Configuration=Release /m
```

If the build is successful the binaries will be in the `src/Release` folder.

### macOS

#### Prerequisites

In order to install prerequisites, [XCode](https://developer.apple.com/xcode/) and [Homebrew](https://brew.sh/) needs to be installed.
Once both are ready, open Terminal app and run the following command to install additional tools:

```bash
$ xcode-select --install
```

On newer macOS versions (v10.14 and higher) this step is done through Software Update in System Preferences.

After that, proceed with installing dependencies:

```bash
$ brew install git python cmake gcc boost
```

#### Building

When all dependencies are installed, build Fuego core binaries:

```bash
$ git clone https://github.com/FandomGold/fuego
$ cd fuego
$ mkdir build && cd $_
$ cmake ..
$ make
```

If the build is successful the binaries will be located in `src` directory.
*******************************

Join the ever-growing community of Fuego holders. 
 
Social channels include [Discord](https://discordapp.com/invite/5UJcJJg), [Reddit](https://reddit.com/r/Fango), or [Twitter](https://twitter.com/usexfg).

