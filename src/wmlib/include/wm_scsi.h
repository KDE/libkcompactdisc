#ifndef WM_SCSI_H
#define WM_SCSI_H
/*
 * This file is part of WorkMan, the civilized CD player library
 * Copyright (C) 1991-1997 by Steven Grimm <koreth@midwinter.com>
 * Copyright (C) by Dirk Försterling <milliByte@DeathsDoor.com>
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
 * SCSI prototypes (scsi.c)
 *
 * This is just one more step to a more modular and understandable code.
 */

#include "wm_struct.h"

#define	WM_ERR_SCSI_INQUIRY_FAILED -1

int	wm_scsi_mode_sense( struct wm_drive *d, unsigned char page,
			    unsigned char *buf );
int	sendscsi( struct wm_drive *d, void *buf,
		  unsigned int len, int dir,
		  unsigned char a0, unsigned char a1,
		  unsigned char a2, unsigned char a3,
		  unsigned char a4, unsigned char a5,
		  unsigned char a6, unsigned char a7,
		  unsigned char a8, unsigned char a9,
		  unsigned char a10, unsigned char a11 );
int	wm_scsi_get_drive_type( struct wm_drive *d);
int wm_scsi_get_cdtext( struct wm_drive *d,
	unsigned char **pp_buffer, int *p_buffer_length );
int wm_scsi_set_speed( struct wm_drive *d, int read_speed );

#endif /* WM_SCSI_H */
