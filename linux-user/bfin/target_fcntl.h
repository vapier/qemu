/*
 * Blackfin file control related defines.
 *
 * Copyright 2007-2018 Mike Frysinger
 * Copyright 2003-2011 Analog Devices Inc.
 *
 * Licensed under the GPL-2
 */

#ifndef BFIN_TARGET_FCNTL_H
#define BFIN_TARGET_FCNTL_H

#define TARGET_O_DIRECTORY      040000
#define TARGET_O_NOFOLLOW      0100000
#define TARGET_O_DIRECT        0200000
#define TARGET_O_LARGEFILE     0400000

#include "../generic/fcntl.h"

#endif
