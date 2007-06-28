#ifndef WM_PLATFORM_H
#define WM_PLATFORM_H
/*
 * This file is part of WorkMan, the civilized CD player library
 * Copyright (C) 1991-1997 by Steven Grimm <koreth@midwinter.com>
 * Copyright (C) by Dirk FÃ¶rsterling <milliByte@DeathsDoor.com>
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
 * The platform interface
 *
 * This is just one more step to a more modular and understandable code.
 */

#define WM_CDS_ERROR(status) ((status) < 0 ||\
                                (status) == WM_CDM_UNKNOWN)

#define WM_CDS_NO_DISC(status) ((status) < 0 ||\
                                (status) == WM_CDM_UNKNOWN ||\
                                (status) == WM_CDM_EJECTED ||\
                                (status) == WM_CDM_NO_DISC)

#define WM_CDS_DISC_READY(status) ((status) == WM_CDM_TRACK_DONE ||\
                                (status) == WM_CDM_PLAYING ||\
                                (status) == WM_CDM_FORWARD ||\
                                (status) == WM_CDM_PAUSED ||\
                                (status) == WM_CDM_STOPPED ||\
                                (status) == WM_CDM_LOADING ||\
                                (status) == WM_CDM_BUFFERING)

#define WM_CDS_DISC_PLAYING(status) ((status) == WM_CDM_TRACK_DONE ||\
                                (status) == WM_CDM_PLAYING ||\
                                (status) == WM_CDM_FORWARD ||\
                                (status) == WM_CDM_PAUSED)
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
#define WM_CDM_LOADING       13      /* tribute to phonon state mashine */
#define WM_CDM_BUFFERING     14      /* tribute to phonon state mashine */
#define WM_CDM_CDDAACK       0xF0

#endif /* WM_PLATFORM_H */
