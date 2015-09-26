#ifndef WM_CDROM_H
#define WM_CDROM_H
/*
 * This file is part of WorkMan, the civilized CD player library
 * Copyright (C) 1991-1997 by Steven Grimm <koreth@midwinter.com>
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
 * Prototypes from cdrom.c
 *
 * This is just one more step to a more modular and understandable code.
 */

#include "wm_platform.h"

#define WM_CDIN                 0
#define WM_CDDA                 1

#define WM_ENDTRACK             0

#define WM_BALANCE_SYMMETRED    0
#define WM_BALANCE_ALL_LEFTS   -10
#define WM_BALANCE_ALL_RIGHTS   10

#define WM_VOLUME_MUTE          0
#define WM_VOLUME_MAXIMAL       100

/*
 * for valid values see wm_helpers.h
 */
int    wm_cd_set_verbosity(int);
const char *wm_drive_default_device();

int    wm_cd_init(const char *cd_device, const char *soundsystem,
  const char *sounddevice, const char *ctldevice, void **);
int    wm_cd_destroy(void *);

int    wm_cd_status(void *);
int    wm_cd_getcurtrack(void *);
int    wm_cd_getcurtracklen(void *);
int    wm_get_cur_pos_rel(void *);
int    wm_get_cur_pos_abs(void *);

int    wm_cd_getcountoftracks(void *);
int    wm_cd_gettracklen(void *, int track);
int    wm_cd_gettrackstart(void *, int track);
int    wm_cd_gettrackdata(void *, int track);

int    wm_cd_play(void *, int start, int pos, int end);
int    wm_cd_pause(void *);
int    wm_cd_stop(void *);
int    wm_cd_eject(void *);
int    wm_cd_closetray(void *);



const char *wm_drive_vendor(void *);
const char *wm_drive_model(void *);
const char *wm_drive_revision(void *);
unsigned long wm_cddb_discid(void *);

/*
 * volume is valid WM_VOLUME_MUTE <= vol <= WM_VOLUME_MAXIMAL,
 * balance is valid WM_BALANCE_ALL_LEFTS <= balance <= WM_BALANCE_ALL_RIGHTS
 */
int    wm_cd_volume(void *, int volume, int balance);

/*
 * please notice, that more OSs don't allow to read balance and volume
 * in this case you get -1 for volume and WM_BALANCE_SYMMETRED for balance
 */
int    wm_cd_getvolume(void *);
int    wm_cd_getbalance(void *);

#endif /* WM_CDROM_H */
