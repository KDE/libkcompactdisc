/*
 * This file is part of WorkMan, the civilized CD player library
 * Copyright (C) 1991-1997 by Steven Grimm (original author)
 * Copyright (C) by Dirk Försterling (current 'author' = maintainer)
 * The maintainer can be contacted by his e-mail address:
 * <milliByte@DeathsDoor.com>
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
 * Vendor-specific drive control routines for Toshiba XM-3401 series.
 */

#include <stdio.h>
#include <errno.h>
#include "include/wm_config.h"
#include "include/wm_struct.h"
#include "include/wm_scsi.h"

#define SCMD_TOSH_EJECT 0xc4

/* local prototypes */
/* static int min_volume = 0, max_volume = 255; */

/*
 * Undo the transformation above using a binary search (so no floating-point
 * math is required.)
 */
static int unscale_volume(int cd_vol, int max)
{
	int vol = 0, top = max, bot = 0, scaled = 0;

	/*cd_vol = (cd_vol * 100 + (max_volume - 1)) / max_volume;*/

	while (bot <= top)
	{
		vol = (top + bot) / 2;
		scaled = (vol * vol) / max;
		if (cd_vol <= scaled)
			top = vol - 1;
		else
			bot = vol + 1;
	}

	/* Might have looked down too far for repeated scaled values */
	if (cd_vol < scaled)
		vol++;

	if (vol < 0)
		vol = 0;
	else if (vol > max)
		vol = max;

	return (vol);
}

/*
 * Send the Toshiba code to eject the CD.
 */
static int tosh_eject(struct wm_drive *d)
{
	return sendscsi(d, NULL, 0, 0, SCMD_TOSH_EJECT, 1, 0,0,0,0,0,0,0,0,0,0);
}

/*
 * Set the volume.  The low end of the scale is more sensitive than the high
 * end, so make up for that by transforming the volume parameters to a square
 * curve.
 */
static int tosh_scale_volume(int *left, int *right)
{
	*left = (*left * *left * *left) / 10000;
	*right = (*right * *right * *right) / 10000;

	return 0;
}

static int tosh_unscale_volume(int *left, int *right)
{
	*left = unscale_volume(*left, 100);
	*right = unscale_volume(*right, 100);

	return 0;
}

int toshiba_fixup(struct wm_drive *d)
{
	d->proto.eject = tosh_eject;
	d->proto.scale_volume = tosh_scale_volume;
	d->proto.unscale_volume = tosh_unscale_volume;

	return 0;
}
