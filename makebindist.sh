#!/bin/sh

VERSION=`awk '$2 == "VERSION" { gsub("\"", "", $3); print $3 }' config.h`

function makebindist {
	HOST=$1
	ARCH=$2
	BITS=$3
	EXTRA=$4
	./configure --host=x86_64-linux-gnu CFLAGS=-m$BITS --prefix=/tmp/$ARCH $EXTRA &&
	make clean && make install &&
	tar -zcvf bint-$VERSION-$ARCH.tar.gz -C /tmp/$ARCH . &&
	rm -rf /tmp/$ARCH
}

# Linux
makebindist x86_64-linux-gnu Linux_x86_64 64
makebindist x86_64-linux-gnu Linux_i386 32

# Mac OS X
makebindist i686-apple-darwin10 Darwin_x86_64 64 ac_cv_func_malloc_0_nonnull=yes
makebindist i686-apple-darwin10 Darwin_i386 32 ac_cv_func_malloc_0_nonnull=yes

# Windows
makebindist x86_64-w64-mingw32 win_x86_64 64 ac_cv_func_malloc_0_nonnull=yes
makebindist i686-w64-mingw32 win_i386 32 ac_cv_func_malloc_0_nonnull=yes
