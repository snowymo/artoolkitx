#! /bin/bash

#
# artoolkitX master build script.
#
# This script builds the core libraries, utilities, and examples.
# Parameters control target platform(s) and options.
#
# Copyright 2018, artoolkitX Contributors.
# Author(s): Philip Lamb, Thorsten Bux, John Wolf, Dan Bell.
#

# Get our location.
OURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

function usage {
    echo "Usage: $(basename $0) [--debug] (macos | windows | ios | linux | android | linux-raspbian | docs)... [tests] [examples] [unity]"
    exit 1
}

if [ $# -eq 0 ]; then
    usage
fi

# -e = exit on errors
set -e

# -x = debug
#set -x

# Parse parameters
while test $# -gt 0
do
    case "$1" in
        macos) BUILD_MACOS=1
            ;;
        ios) BUILD_IOS=1
            ;;
        linux) BUILD_LINUX=1
            ;;
        linux-raspbian) BUILD_LINUX_RASPBIAN=1
            ;;
        android) BUILD_ANDROID=1
            ;;
        windows) BUILD_WINDOWS=1
            ;;
		examples) BUILD_EXAMPLES=1
		    ;;
        docs) BUILD_DOCS=1
            ;;
        --debug) DEBUG=
            ;;
        --*) echo "bad option $1"
            usage
            ;;
        *) echo "bad argument $1"
            usage
            ;;
    esac
    shift
done

# Set OS-dependent variables.
OS=`uname -s`
ARCH=`uname -m`
TAR='/usr/bin/tar'
if [ "$OS" = "Linux" ]
then
    CPUS=`/usr/bin/nproc`
    TAR='/bin/tar'
    # Identify Linux OS. Sets useful variables: ID, ID_LIKE, VERSION, NAME, PRETTY_NAME.
    source /etc/os-release
    # Windows Subsystem for Linux identifies itself as 'Linux'. Additional test required.
    if grep -qE "(Microsoft|WSL)" /proc/version &> /dev/null ; then
        OS='Windows'
    fi
elif [ "$OS" = "Darwin" ]
then
    CPUS=`/usr/sbin/sysctl -n hw.ncpu`
elif [ "$OS" = "CYGWIN_NT-6.1" ]
then
    # bash on Cygwin.
    CPUS=`/usr/bin/nproc`
    OS='Windows'
elif [ "$OS" = "MINGW64_NT-10.0" ]
then
    # git-bash on Windows.
    CPUS=`/usr/bin/nproc`
    OS='Windows'
else
    CPUS=1
fi

# Function to allow check for required packages.
function check_package {
	# Variant for distros that use debian packaging.
	if (type dpkg-query >/dev/null 2>&1) ; then
		if ! $(dpkg-query -W -f='${Status}' $1 | grep -q '^install ok installed$') ; then
			echo "Warning: required package '$1' does not appear to be installed. To install it use 'sudo apt-get install $1'."
		fi
	# Variant for distros that use rpm packaging.
	elif (type rpm >/dev/null 2>&1) ; then
		if ! $(rpm -qa | grep -q $1) ; then
			echo "Warning: required package '$1' does not appear to be installed. To install it use 'sudo dnf install $1'."
		fi
	fi
}


if [ "$OS" = "Darwin" ] || [ "$OS" = "Linux" ] || [ "$OS" = "Windows" ] ; then
# ======================================================================
#  Build platforms hosted by macOS/Linux/Windows
# ======================================================================

# Documentation
if [ $BUILD_DOCS ] ; then
    if [ "$OS" = "Linux" ] ; then
        check_package doxygen
    fi

    (cd "../Documentation"
    rm -rf APIreference/ARX/html APIreference/ARX/xml
    cd doxygen
    doxygen Doxyfile
    )
fi
# /BUILD_DOCS

fi
# /Darwin||Linux||Windows

if [ "$OS" = "Windows" ] ; then
# ======================================================================
#  Build platforms hosted by Windows
# ======================================================================

# Windows
if [ $BUILD_WINDOWS ] ; then

    cp $OURDIR/depends/windows/lib/x64/opencv* $OURDIR/../SDK/bin
    cp $OURDIR/depends/windows/lib/x64/SDL2.dll $OURDIR/../SDK/bin

    if [ $BUILD_EXAMPLES ] ; then
    cd $OURDIR

    (cd "../Examples/Square tracking example/Windows"
        mkdir -p build-windows
        cd build-windows
        cmake.exe .. -DCMAKE_CONFIGURATION_TYPES=${DEBUG+Debug}${DEBUG-Release} "-GVisual Studio 15 2017 Win64"
        cmake.exe --build . --config ${DEBUG+Debug}${DEBUG-Release}  --target install
        #Copy needed dlls into the corresponding Visual Studio directory to allow running examples from inside the Visual Studio GUI
        mkdir -p ${DEBUG+Debug}${DEBUG-Release}
        cp $OURDIR/depends/windows/lib/x64/opencv*.dll ./${DEBUG+Debug}${DEBUG-Release}
        cp $OURDIR/depends/windows/lib/x64/SDL2.dll ./${DEBUG+Debug}${DEBUG-Release}
        cp $OURDIR/../SDK/bin/ARX*.dll ./${DEBUG+Debug}${DEBUG-Release}
        cp $OURDIR/../SDK/bin/*.patt ./${DEBUG+Debug}${DEBUG-Release}
    )
    (cd "../Examples/2d tracking example/Windows"
        mkdir -p build-windows
        cd build-windows
        cmake.exe .. -DCMAKE_CONFIGURATION_TYPES=${DEBUG+Debug}${DEBUG-Release} "-GVisual Studio 15 2017 Win64"
        cmake.exe --build . --config ${DEBUG+Debug}${DEBUG-Release}  --target install
        #Copy needed dlls into the corresponding Visual Studio directory to allow running examples from inside the Visual Studio GUI
        mkdir -p ${DEBUG+Debug}${DEBUG-Release}
        cp $OURDIR/depends/windows/lib/x64/opencv*.dll ./${DEBUG+Debug}${DEBUG-Release}
        cp $OURDIR/depends/windows/lib/x64/SDL2.dll ./${DEBUG+Debug}${DEBUG-Release}
        cp $OURDIR/../SDK/bin/ARX*.dll ./${DEBUG+Debug}${DEBUG-Release}
        cp $OURDIR/../SDK/bin/pinball.jpg ./${DEBUG+Debug}${DEBUG-Release}
        cp $OURDIR/../SDK/bin/database.xml.gz ./${DEBUG+Debug}${DEBUG-Release}
    )
    fi
fi
# /BUILD_WINDOWS

fi
# /Windows
