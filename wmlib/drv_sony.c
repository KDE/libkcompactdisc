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
 * Vendor-specific drive control routines for Sony CDU-8012 series.
 */

#include <stdio.h>
#include <errno.h>
#include "include/wm_config.h"
#include "include/wm_struct.h"
#include "include/wm_scsi.h"

#define PAGE_AUDIO 0x0e

static int max_volume = 255;

/*
 * On the Sony CDU-8012 drive, the amount of sound coming out the jack
 * increases much faster toward the top end of the volume scale than it
 * does at the bottom.  To make up for this, we make the volume scale look
 * sort of logarithmic (actually an upside-down inverse square curve) so
 * that the volume value passed to the drive changes less and less as you
 * approach the maximum slider setting.  Additionally, only the top half
 * of the volume scale is valid; the bottom half is all silent.  The actual
 * formula looks like
 *
 *     max^2 - (max - vol)^2   max
 * v = --------------------- + ---
 *            max * 2           2
 *
 * Where "max" is the maximum value of the volume scale, usually 100.
 */
static int scale_volume(int vol, int max)
{
	vol = (max*max - (max - vol) * (max - vol)) / max;
	return ((vol + max) / 2);
}

/*
 * Given a value between min_volume and max_volume, return the standard-scale
 * volume value needed to achieve that hardware value.
 *
 * Rather than perform floating-point calculations to reverse the above
 * formula, we simply do a binary search of scale_volume()'s return values.
 */
static int unscale_volume(int vol, int max)
{
	int ret_vol = 0, top = max, bot = 0, scaled = 0;

	vol = (vol * 100 + (max_volume - 1)) / max_volume;

	while (bot <= top)
	{
		ret_vol = (top + bot) / 2;
		scaled = scale_volume(ret_vol, max);
		if (vol <= scaled)
			top = ret_vol - 1;
		else
			bot = ret_vol + 1;
	}

	/* Might have looked down too far for repeated scaled values */
	if (vol < scaled)
		ret_vol++;

	if (ret_vol < 0)
		ret_vol = 0;
	else if (ret_vol > max)
		ret_vol = max;

	return ret_vol;
}

/*
 * Set the volume using the wacky scale outlined above.  The Sony drive
 * responds to the standard set-volume command.
 *
 * Get the volume.  Sun's CD-ROM driver doesn't support this operation, even
 * though their drive does.  Dumb.
 */
static int sony_get_volume( struct wm_drive *d, int *left, int *right )
{
	unsigned char mode[16];

	/* Get the current audio parameters first. */
	if (wm_scsi_mode_sense(d, PAGE_AUDIO, mode))
		return -1;

	*left = mode[9];
	*right = mode[11];

	return 0;
}

static int sony_scale_volume(int *left, int *right)
{
	*left = scale_volume(*left, 100);
	*right = scale_volume(*right, 100);

	return 0;
}

static int sony_unscale_volume(int *left, int *right)
{
	*left = unscale_volume(*left, 100);
	*right = unscale_volume(*right, 100);

	return 0;
}

int sony_fixup(struct wm_drive *d)
{
	d->proto.get_volume = sony_get_volume;
	d->proto.scale_volume = sony_scale_volume;
	d->proto.unscale_volume = sony_unscale_volume;

	return 0;
}
