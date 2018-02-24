UNIX BUILD NOTES
====================
Some notes on how to build Scash in Linux/Unix.

To Build
---------------------

```
cd src
make
```

This will build scashd if the dependencies are met.

Dependencies
---------------------

These dependencies are required:

 Library     | Purpose          | Description
 ------------|------------------|----------------------
 libssl      | Crypto           | Random Number Generation, Elliptic Curve Cryptography
 libboost    | Utility          | Library for threading, data structures, etc
 libevent    | Networking       | OS independent asynchronous networking
 libdb4.8    | Berkeley DB      | Wallet storage (only needed when wallet enabled)

Optional dependencies:

 Library     | Purpose          | Description
 ------------|------------------|----------------------
 qt          | GUI              | GUI toolkit (only needed when GUI enabled)
 
System requirements
--------------------

C++ compilers are memory-hungry. It is recommended to have at least 1 GB of
memory available when compiling Scash. With 512MB of memory or less
compilation will take much longer due to swap thrashing.

Dependency Build Instructions: Ubuntu & Debian
----------------------------------------------
Build requirements:

    sudo apt-get install build-essential libtool autotools-dev automake pkg-config libssl-dev libevent-dev bsdmainutils

On at least Ubuntu 14.04+ and Debian 7+ there are generic names for the
individual boost development packages, so the following can be used to only
install necessary parts of boost:

    sudo apt-get install libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-program-options-dev libboost-test-dev libboost-thread-dev

If that doesn't work, you can install all boost development packages with:

    sudo apt-get install libboost-all-dev

BerkeleyDB is required for the wallet. db4.8 packages are available [here](https://launchpad.net/~bitcoin/+archive/bitcoin).
You can add the repository and install using the following commands:

    sudo add-apt-repository ppa:bitcoin/bitcoin
    sudo apt-get update
    sudo apt-get install libdb4.8-dev libdb4.8++-dev

Ubuntu and Debian have their own libdb-dev and libdb++-dev packages, but these will install
BerkeleyDB 5.1 or later, which break binary wallet compatibility with the distributed executables which
are based on BerkeleyDB 4.8. If you do not care about wallet compatibility,
pass `--with-incompatible-bdb` to configure.

Dependencies for the GUI: Ubuntu & Debian
-----------------------------------------

If you want to build Scash-qt, make sure that the required packages for Qt development
are installed. Either Qt 5 or Qt 4 are necessary to build the GUI.
If both Qt 4 and Qt 5 are installed, Qt 5 will be used. Pass `--with-gui=qt4` to configure to choose Qt4.
To build without GUI pass `--without-gui`.

To build with Qt 5 (recommended) you need the following:

    sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools 

Alternatively, to build with Qt 4 you need the following:

    sudo apt-get install libqt4-dev libprotobuf-dev

Once these are installed, they will be found by configure and a scash-qt executable will be
built by default.

Troubleshooting
-----

In case of getting this message while UI compilation:

    fatal error: QMainWindow: No such file or directory

Just use 

    qmake-qt4
    
Instead of qmake.


In case of getting lots of C++ related errors like this:

    error: expected primary-expression before ‘const’

Open Makefile and add -std=c++11 to CXXFLAGS. This line should be like:

    CXXFLAGS  = -std=c++11 -m64 -pipe ...

Notes
-----
The release is built with GCC and then "strip scashd" to strip the debug
symbols, which reduces the executable size by about 90%.


Boost
-----
If you need to build Boost yourself:

	sudo su
	./bootstrap.sh
	./bjam install
