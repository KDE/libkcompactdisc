/*  This file is part of the KDE project
    Copyright (C) 2006 Alexander Kern <alex.kern@gmx.de>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef __AUDIO_H__
#define __AUDIO_H__

#ifndef NULL
#define NULL 0
#endif
struct wm_cdda_block;

struct audio_oops {
  int (*wmaudio_open)(void);
  int (*wmaudio_close)(void);
  int (*wmaudio_play)(struct wm_cdda_block*);
  int (*wmaudio_pause)(void);
  int (*wmaudio_stop)(void);
  int (*wmaudio_state)(struct wm_cdda_block*);
  int (*wmaudio_balvol)(int, int *, int *);
};

#ifdef __cplusplus
    extern "C" {
#endif

struct audio_oops *setup_soundsystem(const char *, const char *, const char *);

#ifdef __cplusplus
    }
#endif

#endif /* __AUDIO_H__ */
