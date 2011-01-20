#!/bin/bash -ex

export PATH=/usr/lib/ccache/bin:${PATH}

#	--enable-linux-user \
#	--disable-system \
#	--target-list="armeb-linux-user arm-linux-user cris-linux-user microblaze-linux-user x86_64-linux-user bfin-linux-user" \

./configure \
	--enable-debug \
	--prefix=/usr \
	--target-list="bfin-linux-user" \
	--disable-strip \
	--disable-werror \
	--disable-sdl \
	--disable-curses \
	--disable-vnc-tls \
	--disable-system \
	--disable-curl \
	--disable-vnc-sasl \
	--disable-bluez
make -s -j4
