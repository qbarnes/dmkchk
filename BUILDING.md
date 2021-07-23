# Building Natively on Linux

You'll need to install the usual development software packages first.
Then to build for Linux, just run:
```
$ make
```

The binary will be under the `build` directory.


# Cross-building for All Platforms on Linux

You'll need to install the DJGPP cross compiler.  Run:
```
$ wget https://github.com/andrewwutw/build-djgpp/releases/download/v3.1/djgpp-linux64-gcc1020.tar.bz2
$ cd /usr/local
$ sudo tar -xvf /tmp/djgpp-linux64-gcc1020.tar.bz2
```

Add the binaries to your search path with:
```
$ export PATH=/usr/local/djgpp/bin:$PATH
```

To cross build for all platforms, run:
```
$ make -f Makefile.cross
```

The binaries will be found under `build.*` directories.

For now, I haven't found a Raspberry Pi cross-build environment for Linux.
