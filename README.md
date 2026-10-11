# Bytecoin Wallet — bytecoin.world Community Edition

This community-maintained edition builds on the upstream Bytecoin Wallet and is
released independently by the bytecoin.world community. The current stable
release is **3.6.0**.

![3.6.0](https://private-user-images.githubusercontent.com/183026333/670681441-480bf4de-a74e-4943-8a51-9a87847a23ca.png?jwt=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3OTE3MDY2MjYsIm5iZiI6MTc5MTcwNjMyNiwicGF0aCI6Ii8xODMwMjYzMzMvNjcwNjgxNDQxLTQ4MGJmNGRlLWE3NGUtNDk0My04YTUxLTlhODc4NDdhMjNjYS5wbmc_WC1BbXotQWxnb3JpdGhtPUFXUzQtSE1BQy1TSEEyNTYmWC1BbXotQ3JlZGVudGlhbD1BS0lBVkNPRFlMU0E1M1BRSzRaQSUyRjIwMjYxMDExJTJGdXMtZWFzdC0xJTJGczMlMkZhd3M0X3JlcXVlc3QmWC1BbXotRGF0ZT0yMDI2MTAxMVQwODEyMDZaJlgtQW16LUV4cGlyZXM9MzAwJlgtQW16LVNpZ25hdHVyZT1jNjg5NmE3OGY1NDk1MTRhY2UyYzNhNzRiNTIxMWU0MmJiM2IwYmRkZGE5YWRmMTYwZTUxNjdlZmEwMmViZTM0JlgtQW16LVNpZ25lZEhlYWRlcnM9aG9zdCZyZXNwb25zZS1jb250ZW50LXR5cGU9aW1hZ2UlMkZwbmcifQ.bxuJhgmSyq80il5beQXL__-nABhU9DCKHloYzy9FfHc)

<p align="center">
  <a href="https://github.com/bytecoinworld/bytecoin-gui/releases/tag/v3.6.0">
    <font size="22">Download Bytecoin Wallet</font>
  </a>
</p>
## How to build binaries from source code

The CMake build fetches the pinned ZXing-C++ 1.4.0 source to provide local QR
code reading and writing. It requires CMake 3.14 or newer and a C++17 compiler.
The qmake project uses the same library as a prebuilt static dependency: build
and install ZXing-C++ 1.4.0 with reader and writer support enabled, then set
`ZXING_ROOT` to its installation prefix before opening/building the qmake
project. The installation must provide `include/ZXing/ReadBarcode.h` and
`lib/ZXing` (or the platform equivalent). ZXing-C++ is licensed under
Apache-2.0.

### Windows
To build the gui you must have built bytecoin core, so please do all steps from [here](https://github.com/bcndev/bytecoin#building-on-windows) before proceed. Install [QtCreator](https://www.qt.io/download-thank-you?os=windows), open the project file bytecoin-gui/src/bytecoin-gui.pro in QtCreator and build it using MSVS kit (you must have MSVS installed already to build bytecoin core).

For qmake builds, configure the ZXing-C++ 1.4.0 static library with the same
Visual Studio compiler and architecture used by the Qt kit, install it, and set
`ZXING_ROOT` to that installation directory.

For example, configure the ZXing-C++ v1.4.0 checkout with
`-DBUILD_WRITERS=ON -DBUILD_READERS=ON -DBUILD_EXAMPLES=OFF`, set
`CMAKE_INSTALL_PREFIX` to a directory you can write, build its `install`
target, and point `ZXING_ROOT` at that directory.

### MacOS

To build the gui you must have built bytecoin core, so please do all steps from [here](https://github.com/bcndev/bytecoin#building-on-mac-osx) before proceed. Install [QtCreator](https://www.qt.io/download-thank-you?os=macos), open the project file bytecoin-gui/src/bytecoin-gui.pro in QtCreator and build it using clang kit (you must have XCode installed already to build bytecoin core).

For qmake builds, install a static ZXing-C++ 1.4.0 build for the same
architecture and set `ZXING_ROOT` to its installation prefix.

### Linux
```
# To install all required packages on Ubuntu use the following command:
$ sudo apt install qt5-qmake qtbase5-dev qtbase5-dev-tools cmake

$ git clone https://github.com/bcndev/bytecoin.git
$ cd bytecoin
$ mkdir -p build
$ cd build
$ cmake ..
$ make -j4 bytecoin-crypto
$ cd ../..
$ git clone https://github.com/bytecoinworld/bytecoin-gui.git
$ cd bytecoin-gui
$ mkdir -p build
$ cd build
$ cmake .. -DCMAKE_PREFIX_PATH=/path/to/qt
$ make -j4
```
The CMake configuration downloads/builds ZXing-C++ 1.4.0 locally; no payment
address, QR contents, or wallet data is sent to a QR service.
Alternative way:
```
# Install QtCreator:
$ sudo apt install qtcreator

$ git clone https://github.com/bcndev/bytecoin.git
$ cd bytecoin
$ mkdir -p build
$ cd build
$ cmake ..
$ make -j4 bytecoin-crypto
$ cd ../..
$ git clone https://github.com/bytecoinworld/bytecoin-gui.git
```
Now open the project file bytecoin-gui/src/bytecoin-gui.pro in QtCreator and build it.
