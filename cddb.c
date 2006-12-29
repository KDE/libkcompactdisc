/*
 * $Id: cddb.c 531626 2006-04-19 17:03:06Z larkang $
 *
 * This file is part of WorkMan, the civilized CD player library
 * Copyright (C) 1991-1997 by Steven Grimm (original author)
 * Copyright (C) by Dirk Försterling (current 'author' = maintainer)
 * The maintainer can be contacted by his e-mail address:
 * milliByte@DeathsDoor.com
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
 *
 * establish connection to cddb server and get data
 * socket stuff gotten from gnu port of the finger command
 *
 * provided by Sven Oliver Moll
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "include/wm_config.h"
#include "include/wm_struct.h"
#include "include/wm_cdinfo.h"
#include "include/wm_helpers.h"
#include "include/wm_cddb.h"

struct wm_cddb cddb;

extern struct wm_cdinfo thiscd;

/* local prototypes */
int cddb_sum(int);

/*
 * Subroutine from cddb_discid
 */
int
cddb_sum(int n)
{
	char	buf[12],
		*p;
	int	ret = 0;

	/* For backward compatibility this algorithm must not change */
	sprintf(buf, "%lu", (unsigned long)n);
	for (p = buf; *p != '\0'; p++)
	  ret += (*p - '0');

	return (ret);
} /* cddb_sum() */


/*
 * Calculate the discid of a CD according to cddb
 */
unsigned long
cddb_discid(void)
{
	int	i,
		t,
		n = 0;

	/* For backward compatibility this algorithm must not change */
	for (i = 0; i < thiscd.ntracks; i++) {

		n += cddb_sum(thiscd.trk[i].start / 75);
	/*
	 * Just for demonstration (See below)
	 *
	 *	t += (thiscd.trk[i+1].start / 75) -
	 *	     (thiscd.trk[i  ].start / 75);
	 */
	}

	/*
	 * Mathematics can be fun. Example: How to reduce a full loop to
	 * a simple statement. The discid algorhythm is so half-hearted
	 * developed that it doesn't even use the full 32bit range.
	 * But it seems to be always this way: The bad standards will be
	 * accepted, the good ones turned down.
         * Boy, you pulled out the /75. This is not correct here, because
         * this calculation needs the integer division for both .start
         * fields.
         */

        t = (thiscd.trk[thiscd.ntracks].start / 75)
          - (thiscd.trk[0].start / 75);
	return ((n % 0xff) << 24 | t << 8 | thiscd.ntracks);
} /* cddb_discid() */

