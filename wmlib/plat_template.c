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
 *
 * This file surely contains nonsense. It's the porter's part to fill
 * the gaps and assure that the resulting code makes sense.
 *
 */

#if [TEMPLATESYSTEM]


#include "include/wm_config.h"
#include "include/wm_struct.h"
#include "include/wm_cdtext.h"

#define WM_MSG_CLASS WM_MSG_CLASS_PLATFORM

/*
 * gen_init();
 *
 */
int
gen_init(struct wm_drive *d)
{
  return (0);
} /* gen_init() */

/*
 * gen_open()
 *
 */
int
gen_open(struct wm_drive *d)
{
  if( ! d )
    {
      errno = EFAULT;
      return -1;
    }

  if(d->fd > -1)			/* device already open? */
    {
      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "gen_open(): [device is open (fd=%d)]\n", d->fd);
      return 0;
    }


  return (0);
} /* gen_open() */

/*
 * gen_scsi()
 *
 */
int
gen_scsi(struct wm_drive *d,
	    uchar_t *cdb, int cdblen,void *retbuf,int retbuflen,int getreply)
{
  return -1;
} /* gen_scsi() */

/*
 * close the CD device
 */

int
gen_close(struct wm_drive *d)
{
    if(d->fd != -1) {
      wm_lib_message(WM_MSG_LEVEL_DEBUG, "closing the device\n");
      close(d->fd);
      d->fd = -1;
    }
    return 0;
} /* gen_close() */

/*
 * gen_get_drive_status()
 *
 */
int
gen_get_drive_status(struct wm_drive *d,
			 int oldmode,
			 int *mode,
			 int *pos,
			 int *track,
			 int *index)
{
  return (0);
} /* gen_get_drive_status() */

/*
 * gen_get_trackcount()
 *
 */
int
gen_get_trackcount(struct wm_drive *d,int *tracks)
{
  return (0);
} /* gen_get_trackcount() */

/*
 * gen_get_trackinfo()
 *
 */
int
gen_get_trackinfo(struct wm_drive *d,int track,int *data,int *startframe)
{
  return (0);
} /* gen_get_trackinfo() */

/*
 * gen_get_cdlen()
 *
 */
int
gen_get_cdlen(struct wm_drive *d,int *frames)
{
  return (0);
} /* gen_get_cdlen() */

/*
 * gen_play()
 *
 */
int
gen_play(struct wm_drive *d,int start,int end)
{
  return (0);
} /* gen_play() */

/*
 * gen_pause()
 *
 */
int
gen_pause(struct wm_drive *d)
{
  return ioctl( 0 );
} /* gen_pause() */

/*
 * gen_resume
 *
 */
int
gen_resume(struct wm_drive *d)
{
  return (0);
} /* gen_resume() */

/*
 * gen_stop()
 *
 */
int
gen_stop(struct wm_drive *d)
{
  return (0);
} /* gen_stop() */

/*
 * gen_eject()
 *
 */
int
gen_eject(struct wm_drive *d)
{
  return (0);
} /* gen_eject() */

/*----------------------------------------*
 * Close the CD tray
 *----------------------------------------*/
int gen_closetray(struct wm_drive *d)
{
  return -1;
} /* gen_closetray() */

int
scale_volume(int vol,int max)
{
  return ((vol * (max_volume - min_volume)) / max + min_volume);
} /* scale_volume() */

int
unscale_volume(int vol,int max)
{
  int n;
  n = ( vol - min_volume ) * max_volume / (max - min_volume);
  return (n <0)?0:n;
} /* unscale_volume() */

/*
 * gen_set_volume()
 *
 */
int
gen_set_volume(struct wm_drive *d,int left,int right)
{
  return (0);
} /* gen_set_volume() */

/*
 * gen_get_volume()
 *
 */
int
gen_get_volume(struct wm_drive *d,int *left,int *right)
{
  return (0);
} /* gen_get_volume() */

#endif /* TEMPLATESYSTEM */
