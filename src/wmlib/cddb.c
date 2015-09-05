/*
 * This file is part of WorkMan, the civilized CD player library
 * Copyright (C) 1991-1997 by Steven Grimm (original author)
 * Copyright (C) by Dirk Försterling <milliByte@DeathsDoor.com>
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
#include "include/wm_helpers.h"
#include "include/wm_cddb.h"
#include "include/wm_cdrom.h"

/*
 * Subroutine from cddb_discid
 */
static int cddb_sum(int n)
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
unsigned long cddb_discid(struct wm_drive *pdrive)
{
	int	i,
		t,
		n = 0, tracks;

    tracks = wm_cd_getcountoftracks(pdrive);
	if(!tracks)
		return (unsigned)-1;

	/* For backward compatibility this algorithm must not change */
	for (i = 0; i < tracks; i++) {

		n += cddb_sum(wm_cd_gettrackstart(pdrive, i + 1));
	/*
	 * Just for demonstration (See below)
	 *
	 *	t += (wm_cd_getref()->trk[i+1].start / 75) -
	 *	     (wm_cd_getref()->trk[i  ].start / 75);
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

        t = (wm_cd_gettrackstart(pdrive, tracks + 1)) - (wm_cd_gettrackstart(pdrive, 1));
	return ((n % 0xff) << 24 | t << 8 | tracks);
} /* cddb_discid() */

