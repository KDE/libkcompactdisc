#ifndef WM_CDROM_H
#define WM_CDROM_H
/*
 * $Id: wm_cdrom.h 587515 2006-09-23 02:48:38Z haeber $
 *
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

#include "wm_struct.h"
#include "wm_cddb.h"

#define WM_CDS_NO_DISC(status) (status < 0 ||\
                                status == WM_CDM_UNKNOWN ||\
                                status == WM_CDM_EJECTED ||\
                                status == WM_CDM_NO_DISC)

#define WM_CDS_DISC_READY(status)(status == WM_CDM_TRACK_DONE ||\
                                status == WM_CDM_PLAYING ||\
                                status == WM_CDM_FORWARD ||\
                                status == WM_CDM_PAUSED ||\
                                status == WM_CDM_STOPPED)

#define WM_CDS_DISC_PLAYING(status)(status == WM_CDM_TRACK_DONE ||\
                                status == WM_CDM_PLAYING ||\
                                status == WM_CDM_FORWARD ||\
                                status == WM_CDM_PAUSED)
#define WM_CDM_BACK       1
#define WM_CDM_TRACK_DONE 1
#define WM_CDM_PLAYING    2
#define WM_CDM_FORWARD    3
#define WM_CDM_PAUSED     4
#define WM_CDM_STOPPED    5
#define WM_CDM_EJECTED    6
#define WM_CDM_DEVICECHANGED 9       /* deprecated */
#define WM_CDM_NO_DISC       10
#define WM_CDM_UNKNOWN       11
#define WM_CDM_CDDAERROR     12
#define WM_CDM_CDDAACK       0xF0

#define WM_CDIN                 0
#define WM_CDDA                 1

#define WM_ENDTRACK             0

#define WM_BALANCE_SYMMETRED    0
#define WM_BALANCE_ALL_LEFTS   -10
#define WM_BALANCE_ALL_RIGHTS   10

#define WM_VOLUME_MUTE          0
#define WM_VOLUME_MAXIMAL       100

struct wm_cdinfo *wm_cd_getref( void );
int wm_get_cur_pos_rel( void );
int wm_get_cur_pos_abs( void );

int    wm_cd_init( int cdin, const char *cd_device, const char *soundsystem,
  const char *sounddevice, const char *ctldevice );
int    wm_cd_destroy( void );
int    wm_cd_status( void );
int    wm_cd_getcurtrack( void );
int    wm_cd_getcurtracklen( void );
int    wm_cd_getcountoftracks( void );
int    wm_cd_gettracklen( int track );
int    wm_cd_play( int start, int pos, int end );
int    wm_cd_play_chunk( int start, int end, int realstart );
int    wm_cd_play_from_pos( int pos );
int    wm_cd_pause( void );
int    wm_cd_stop( void );
int    wm_cd_eject( void );
int    wm_cd_closetray( void );
int    wm_find_trkind( int, int, int );

/*
 * for valid values see wm_helpers.h
 */
int    wm_cd_set_verbosity( int );

const char * wm_drive_vendor( void );
const char * wm_drive_model( void );
const char * wm_drive_revision( void );
const char * wm_drive_device( void );

/*
 * volume is valid WM_VOLUME_MUTE <= vol <= WM_VOLUME_MAXIMAL,
 * balance is valid WM_BALANCE_ALL_LEFTS <= balance <= WM_BALANCE_ALL_RIGHTS
 */
int    wm_cd_volume( int volume, int balance );

/*
 * please notice, that more OSs don't allow to read balance and volume
 * in this case you get -1 for volume and WM_BALANCE_SYMMETRED for balance
 */
int    wm_cd_getvolume( void );
int    wm_cd_getbalance( void );

#endif /* WM_CDROM_H */
