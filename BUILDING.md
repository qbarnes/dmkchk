# Building Dmkchk

There are four ways to build `dmkchk`:

   * [The usual way](#building-dmkchk-natively), which creates a native binary for the system you're running on,
   * [Cross-building](#cross-building-dmkchk),
   * [Cross-building using OCI containers](#cross-building-dmkchk-using-oci-containers) (docker or podman), or
   * [Cross-building with GitHub Actions](#cross-building-with-github-actions)


## Building Dmkchk Natively

To build `dmkchk` for the system you're running on, you may need
to install additional packages before you build.

### Installing packages

First, you'll need to install packages for C development.

On Fedora and RHEL-like distros, run:

```
$ sudo dnf install -y @c-development
```

On Debian, Ubuntu, and related distros, run:

```
$ sudo apt-get update
$ sudo apt-get install -y build-essential
```

Then when cloning this repo, make sure to use the `--recursive` flag like so:

```
$ git clone --recursive git@github.com:qbarnes/dmkchk.git
```

### Building natively

To build, change directory into the cloned repo and run:
```
$ make
```

The binary will be under the `build` directory.


## Cross-Building Dmkchk

Cross-building creates `dmkchk` binaries for these six platforms all
at once:

   * Linux (x86_64, 32-bit and 64-bit ARM),
   * MS-DOS, and
   * 32-bit and 64-bit Microsoft Windows

Cross-building is only supported while running on x86_64 Linux.

To cross-build, you'll need to install some additional software first.

### Installing software

On Fedora and RHEL-like distros, run:

```
$ sudo dnf install -y @c-development mingw32-gcc mingw64-gcc
```

On Debian, Ubuntu, and related distros, run:

```
$ sudo apt-get update
$ sudo apt-get install -y build-essential mingw-w64 gcc-aarch64-linux-gnu gcc-arm-linux-gnueabi
```

For MS-DOS cross-compiling, the unpackaged DJGPP compiler is required.
To install, run:
```
$ wget https://github.com/andrewwutw/build-djgpp/releases/download/v3.1/djgpp-linux64-gcc1020.tar.bz2
$ sudo tar -xvC /usr/local -f djgpp-linux64-gcc1020.tar.bz2
```

Add the cross-compiler to your search path with:
```
$ export PATH=/usr/local/djgpp/bin:$PATH
```

Then when cloning this repo, make sure to use the `--recursive` flag like so:

```
$ git clone --recursive git@github.com:qbarnes/dmkchk.git
```

### Cross-building

To cross-build for all platforms, change directory into the cloned
repo and run:
```
$ make -f Makefile.cross
```

Binaries will be found under the `build.*` directories.

Unfortunately, Fedora and related distros don't fully support
cross-building for ARM.  Packages for ARM cross-compilers are
available, but without the necessary cross-environment.
To exclude the ARM platforms when cross-building, run:

```
$ make -f Makefile.cross BUILDS="LINUX_X86_64 MSDOS MSWIN32 MSWIN64"
```


## Cross-Building Dmkchk using OCI Containers

Cross-building `dmkchk` with containers builds binaries for all six
platforms like cross-building above, but does not require installing
any additional software packages on your system beyond `git`,
`make`, and OCI tools (`docker` or `podman`).  On Fedora, building
with containers also allows ARM cross-builds.

Cross-building with containers is only supported when running on
x86_64 Linux.

### Installing software

On Fedora and RHEL-like distros, run:

```
$ sudo dnf install -y git make podman
```

On Debian, Ubuntu, and related distros, run:

```
$ sudo apt-get update
$ sudo apt-get install -y git make docker.io
```

Then when cloning this repo, make sure to use the `--recursive` flag like so:

```
$ git clone --recursive git@github.com:qbarnes/dmkchk.git
```

### Using containers for building

To cross-build for all platforms using containers, change directory
into the cloned repo and run:

```
$ make -f Makefile.oci
```

Note that the first time building will take longer to download
and locally cache the containers.

## Cross-Building with GitHub Actions

Building with GitHub Actions requires no need to install any
software on your personal computer, or for that matter, even have a
computer of your own!

If you're not already an existing contributor to `dmkchk`, the
first step to building `dmkchk` with GitHub Actions is to fork the
repository and then follow the directions below on your own fork.

To access your own fork using the example links below, copy them but
replace "qbarnes" with your own GitHub account.

### Building artifacts

To build artifacts, go to the "Build dmkchk" workflow page under
GitHub Actions.  A example link would be
[https://github.com/qbarnes/dmkchk/actions/workflows/dmkchk-build.yml](https://github.com/qbarnes/dmkchk/actions/workflows/dmkchk-build.yml),
but switch to your fork.

On the "Build dmkchk" workflows page, you'll find a button menu
"Run workflow".  Select it.  If desired, change the branch, then
select the "Run workflow" button at the bottom of the menu.  You may
need to reload the web page to see the scheduled workflow appear.

While the workflow is running (or after the build finishes), select
its "Build dmkchk" link to see the status of the run.  When the run
completes successfully, six artifacts will be added at the bottom
of the page.  You can download any or all of these.  They'll each
contain a tarball of files for that platform's build.

### Building releases

To build a release, first ensure the `VERSION` macro in `product.mk`
contains the unique release identifier you want for this release.

Now run the the "Release dmkchk" GitHub Action workflow.  An
example link would be
[https://github.com/qbarnes/dmkchk/actions/workflows/dmkchk-release.yml](https://github.com/qbarnes/dmkchk/actions/workflows/dmkchk-release.yml),
but use your own fork.

On the "Release dmkchk" workflows page, you'll find a button menu
"Run workflow".  Select it.  If desired, change the branch, then
select the "Run workflow" button at the bottom of the menu.  You may
need to reload the web page to see the scheduled workflow appear.

While the workflow is running (or after the build finishes), select
its "Release dmkchk" link.

A common reason for the release workflow to fail is not having
a unique tag.  Either change the `VERSION` macro or remove the
existing tag that conflicts.

To clean up from a failed release workflow run, be sure to delete
the release, if made, and then delete its tag.

When the run completes successfully, you'll find a new tag with the
string from the `VERSION` macro prefixed with a "v" under the repo's
"Tags".  Example link:
[https://github.com/qbarnes/dmkchk/tags](https://github.com/qbarnes/dmkchk/tags).
Also, under the newly created release area for the tag, you'll find
its six release tarballs and source code.  Example link:
[https://github.com/qbarnes/dmkchk/releases/tag/v0.0.2](https://github.com/qbarnes/dmkchk/releases/tag/v0.0.2).

Be sure to edit and update the description of the new release!
