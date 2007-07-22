#ifndef WM_CDDA_H
#define WM_CDDA_H
/*
 * This file is part of WorkMan, the civilized CD player library
 * Copyright (C) 1991-1997 by Steven Grimm <koreth@midwinter.com>
 * Copyright (C) by Dirk FÃ¶rsterling <milliByte@DeathsDoor.com>
 * Copyright (C) 2004-2006 Alexander Kern <alex.kern@gmx.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "wm_cdrom.h"
#include "wm_config.h"
#include "wm_struct.h"
/*
 * cdda_block status codes.
 */

/*
 * Enable or disable CDDA building depending on platform capabilities, and
 * determine endianness based on architecture.  (Gross!)
 *
 * For header-comfort, the macros LITTLE_ENDIAN and BIG_ENDIAN had to be
 * renamed. At least Linux does have bytesex.h and endian.h for easy
 * byte-order examination.
 */

#ifdef HAVE_MACHINE_ENDIAN_H
	#include <machine/endian.h>
	#if BYTE_ORDER == LITTLE_ENDIAN
		#define WM_LITTLE_ENDIAN 1
		#define WM_BIG_ENDIAN 0
	#else
		#define WM_LITTLE_ENDIAN 0
		#define WM_BIG_ENDIAN 1
	#endif
#elif defined(__sun) || defined(sun)
# ifdef SYSV
#  include <sys/types.h>
#  include <sys/cdio.h>
#  ifndef CDROMCDDA
#    error what to do?
#  endif
#  ifdef i386
#   define WM_LITTLE_ENDIAN 1
#   define WM_BIG_ENDIAN 0
#  else
#   define WM_BIG_ENDIAN 1
#   define WM_LITTLE_ENDIAN 0
#  endif
# endif

/* Linux only allows definition of endianness, because there's no
 * standard interface for CDROM CDDA functions that aren't available
 * if there is no support.
 */
#elif defined(__linux__)
/*# include <bytesex.h>*/
# include <endian.h>
/*
 * XXX could this be a problem? The results are only 0 and 1 because
 * of the ! operator. How about other linux compilers than gcc ?
 */
# define WM_LITTLE_ENDIAN !(__BYTE_ORDER - __LITTLE_ENDIAN)
# define WM_BIG_ENDIAN !(__BYTE_ORDER - __BIG_ENDIAN)
#elif defined WORDS_BIGENDIAN
	#define WM_LITTLE_ENDIAN 0
	#define WM_BIG_ENDIAN 1
#else
	#define WM_LITTLE_ENDIAN 1
	#define WM_BIG_ENDIAN 0
#endif

/*
 * The following code shouldn't take effect now.
 * In 1998, the WorkMan platforms don't support __PDP_ENDIAN
 * architectures.
 *
 */

#if !defined(WM_LITTLE_ENDIAN)
#  if !defined(WM_BIG_ENDIAN)
#    error yet unsupported architecture
	foo bar this is to stop the compiler.
#  endif
#endif

/*
 * Information about a particular block of CDDA data.
 */
struct cdda_block {
    unsigned char status;
    unsigned char track;
    unsigned char index;
    unsigned char reserved;

    int   frame;
    char *buf;
    long  buflen;
};

struct cdda_device {
    int        fd;
    int        cdda_slave;
    const char *devname;

    unsigned char status;
    unsigned char track;
    unsigned char index;
    unsigned char command;

    int frame;
    int frames_at_once;

    /* Average volume levels, for level meters */
    unsigned char lev_chan0;
    unsigned char lev_chan1;

    /* Current volume setting (0-255) */
    unsigned char volume;

    /* Current balance setting (0-255, 128 = balanced) */
    unsigned char balance;

    struct cdda_block *blocks;
    int numblocks;

    struct cdda_proto *proto;
};

#endif /* WM_CDDA_H */
