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
 *
 * plat_freebsd.c
 *
 * FreeBSD-specific drive control routines.
 *
 * Todd Pfaff, 3/20/94
 *
 */

#if defined(__FreeBSD__) || defined(__FreeBSD) || defined(__NetBSD__) || defined (__NetBSD)

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include "include/wm_config.h"

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/cdio.h>

#if defined(__OpenBSD__)
# define MSF_MINUTES 1
# define MSF_SECONDS 2
# define MSF_FRAMES 3
# include <sys/scsiio.h>
# include "/sys/scsi/scsi_all.h"
# include "/sys/scsi/scsi_cd.h"
#elif defined(__NetBSD__)
#include <sys/scsiio.h>
#include <dev/scsipi/scsipi_cd.h>
#else
# define LEFT_PORT 0
# define RIGHT_PORT 1
# if __FreeBSD_version < 300000
#  include <scsi.h>
# endif
#endif

#include "include/wm_struct.h"
#include "include/wm_platform.h"
#include "include/wm_cdrom.h"
#include "include/wm_scsi.h"
#include "include/wm_helpers.h"
#include "include/wm_cdtext.h"

#define WM_MSG_CLASS WM_MSG_CLASS_PLATFORM

void *malloc();

int	min_volume = 10;
int	max_volume = 255;


/*--------------------------------------------------------*
 * Initialize the drive.  A no-op for the generic driver.
 *--------------------------------------------------------*/
int
gen_init(struct wm_drive *d)
{
	return (0);
} /* gen_init() */


/*-------------------------------------------------------------------*
 * Open the CD device and figure out what kind of drive is attached.
 *-------------------------------------------------------------------*/
int
gen_open( struct wm_drive *d )
{
  if (d->fd >= 0) {
    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "gen_open(): [device is open (fd=%d)]\n", d->fd);
    return 0;
  }

  d->fd = open(d->cd_device, 0);
  if (d->fd < 0) {
    if (errno == EACCES) {
      return -EACCES;
	}

    /* No CD in drive. */
    return 1;
  }

  return 0;
} /* gen_open() */

/*---------------------------------------------*
 * Send an arbitrary SCSI command to a device.
 *
 *---------------------------------------------*/
int
gen_scsi(struct wm_drive *d, unsigned char *cdb, int cdblen,
	void *retbuf, int retbuflen, int getreply)
{
  return (-1);
} /* wm_scsi() */

int
gen_close( struct wm_drive *d )
{
  if(d->fd != -1) {
    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "closing the device\n");
    close(d->fd);
    d->fd = -1;
  }
  return 0;
}

/*--------------------------------------------------------------------------*
 * Get the current status of the drive: the current play mode, the absolute
 * position from start of disc (in frames), and the current track and index
 * numbers if the CD is playing or paused.
 *--------------------------------------------------------------------------*/
int
gen_get_drive_status(struct wm_drive *d, int oldmode,
                     int *mode, int *pos, int *track, int *index)
{
  struct ioc_read_subchannel	sc;
  struct cd_sub_channel_info	scd;

  /* If we can't get status, the CD is ejected, so default to that. */
  *mode = WM_CDM_EJECTED;

  sc.address_format	= CD_MSF_FORMAT;
  sc.data_format		= CD_CURRENT_POSITION;
  sc.track		= 0;
  sc.data_len		= sizeof(scd);
  sc.data			= (struct cd_sub_channel_info *)&scd;

  /* Is the device open? */
  if (d->fd < 0)
    {
      switch (d->proto.open(d))
	{
	case -1:	/* error */
	  return (-1);

	case 1:		/* retry */
	  return (0);
	}
    }

  if (ioctl(d->fd, CDIOCREADSUBCHANNEL, &sc))
    {
      /*
       * #ifdef __NetBSD__
       *
       * Denis Bourez <denis@rsn.fdn.fr> told me, that closing the
       * device is mandatory for FreeBSD, too.
       */
      /* we need to release the device so the kernel will notice
	 reloaded media */
      d->proto.close(d);
      /*
       * #endif
       */
      return (0);	/* ejected */
    }

  switch (scd.header.audio_status)
    {
    case CD_AS_PLAY_IN_PROGRESS:
      *mode = WM_CDM_PLAYING;
    dopos:
	*pos = scd.what.position.absaddr.msf.minute * 60 * 75 +
	scd.what.position.absaddr.msf.second * 75 +
	scd.what.position.absaddr.msf.frame;
	*track = scd.what.position.track_number;
	*index = scd.what.position.index_number;
      break;

    case CD_AS_PLAY_PAUSED:
      if (oldmode == WM_CDM_PLAYING || oldmode == WM_CDM_PAUSED)
	{
	  *mode = WM_CDM_PAUSED;
	  goto dopos;
	}
      else
	*mode = WM_CDM_STOPPED;
      break;

    case CD_AS_PLAY_COMPLETED:
      *mode = WM_CDM_TRACK_DONE; /* waiting for next track. */
      break;

    case CD_AS_NO_STATUS:
    case 0:
      *mode = WM_CDM_STOPPED;
      break;
    }

  return (0);
} /* gen_get_drive_status() */


/*-------------------------------------*
 * Get the number of tracks on the CD.
 *-------------------------------------*/
int
gen_get_trackcount(struct wm_drive *d, int *tracks)
{
  struct ioc_toc_header	hdr;

  if (ioctl(d->fd, CDIOREADTOCHEADER, &hdr) == -1)
    return (-1);

  *tracks = hdr.ending_track - hdr.starting_track + 1;

  return (0);
} /* gen_get_trackcount() */

/*-----------------------------------------------------------------------*
 * Get the start time and mode (data or audio) of a track.
 *
 * XXX - this should get cached, but that means keeping track of ejects.
 *-----------------------------------------------------------------------*/
int
gen_get_trackinfo(struct wm_drive *d, int track, int *data, int *startframe)
{
  struct ioc_read_toc_entry	toc;
  struct cd_toc_entry		toc_buffer;

  bzero((char *)&toc_buffer, sizeof(toc_buffer));
  toc.address_format = CD_MSF_FORMAT;
  toc.starting_track = track;
  toc.data_len = sizeof(toc_buffer);
  toc.data = &toc_buffer;

  if (ioctl(d->fd, CDIOREADTOCENTRYS, &toc))
    return (-1);

  *data = ((toc_buffer.control & 0x4) != 0);

  *startframe = toc_buffer.addr.msf.minute*60*75 +
    toc_buffer.addr.msf.second * 75 +
    toc_buffer.addr.msf.frame;

  return (0);
} /* gen_get_trackinfo() */

/*-------------------------------------*
 * Get the number of frames on the CD.
 *-------------------------------------*/
int
gen_get_cdlen(struct wm_drive *d, int *frames)
{
  int		tmp;
  struct ioc_toc_header		hdr;
  int status;

#define LEADOUT 0xaa			/* see scsi.c.  what a hack! */
  return gen_get_trackinfo(d, LEADOUT, &tmp, frames);
} /* gen_get_cdlen() */


/*------------------------------------------------------------*
 * Play the CD from one position to another (both in frames.)
 *------------------------------------------------------------*/
int
gen_play(struct wm_drive *d, int start, int end)
{
  struct ioc_play_msf	msf;

  msf.start_m	= start / (60*75);
  msf.start_s	= (start % (60*75)) / 75;
  msf.start_f	= start % 75;
  msf.end_m	= end / (60*75);
  msf.end_s	= (end % (60*75)) / 75;
  msf.end_f	= end % 75;

  if (ioctl(d->fd, CDIOCSTART))
    return (-1);

  if (ioctl(d->fd, CDIOCPLAYMSF, &msf))
    return (-2);

  return (0);
} /* gen_play() */

/*---------------*
 * Pause the CD.
 *---------------*/
int
gen_pause( struct wm_drive *d )
{
  return (ioctl(d->fd, CDIOCPAUSE));
} /* gen_pause() */

/*-------------------------------------------------*
 * Resume playing the CD (assuming it was paused.)
 *-------------------------------------------------*/
int
gen_resume( struct wm_drive *d )
{
  return (ioctl(d->fd, CDIOCRESUME));
} /* gen_resume() */

/*--------------*
 * Stop the CD.
 *--------------*/
int
gen_stop( struct wm_drive *d)
{
  return (ioctl(d->fd, CDIOCSTOP));
} /* gen_stop() */

/*----------------------------------------*
 * Eject the current CD, if there is one.
 *----------------------------------------*/
int
gen_eject( struct wm_drive *d )
{
  /* On some systems, we can check to see if the CD is mounted. */
  struct stat	stbuf;
  struct statfs	buf;
  int rval;

  if (fstat(d->fd, &stbuf) != 0)
    return (-2);

  /* Is this a mounted filesystem? */
  if (fstatfs(stbuf.st_rdev, &buf) == 0)
    return (-3);

  rval = ioctl(d->fd, CDIOCALLOW);

  if (rval == 0)
    rval = ioctl(d->fd, CDIOCEJECT);

  if (rval == 0)
    rval = ioctl(d->fd, CDIOCPREVENT);

  d->proto.close(d);

  return rval;
} /* gen_eject() */

/*----------------------------------------*
 * Close the CD tray
 *----------------------------------------*/

int
gen_closetray(struct wm_drive *d)
{
#ifdef CAN_CLOSE
	if(!d->proto.close(d)) {
      return(d->proto.open(d));
    } else {
      return(-1);
    }
#else
  /* Always succeed if the drive can't close */
  return(0);
#endif /* CAN_CLOSE */
} /* gen_closetray() */


/*---------------------------------------------------------------------------*
 * scale_volume(vol, max)
 *
 * Return a volume value suitable for passing to the CD-ROM drive.  "vol"
 * is a volume slider setting; "max" is the slider's maximum value.
 *
 * On Sun and DEC CD-ROM drives, the amount of sound coming out the jack
 * increases much faster toward the top end of the volume scale than it
 * does at the bottom.  To make up for this, we make the volume scale look
 * sort of logarithmic (actually an upside-down inverse square curve) so
 * that the volume value passed to the drive changes less and less as you
 * approach the maximum slider setting.  The actual formula looks like
 *
 *     (max^2 - (max - vol)^2) * (max_volume - min_volume)
 * v = --------------------------------------------------- + min_volume
 *                           max^2
 *
 * If your system's volume settings aren't broken in this way, something
 * like the following should work:
 *
 *	return ((vol * (max_volume - min_volume)) / max + min_volume);
 *---------------------------------------------------------------------------*/
static int
scale_volume(int vol, int max)
{
  return ((vol * (max_volume - min_volume)) / max + min_volume);
} /* scale_volume() */

/*---------------------------------------------------------------------------*
 * unscale_volume(cd_vol, max)
 *
 * Given a value between min_volume and max_volume, return the volume slider
 * value needed to achieve that value.
 *
 * Rather than perform floating-point calculations to reverse the above
 * formula, we simply do a binary search of scale_volume()'s return values.
 *--------------------------------------------------------------------------*/
static int
unscale_volume( int cd_vol, int max )
{
  int	vol = 0, top = max, bot = 0, scaled;

  while (bot <= top)
    {
      vol = (top + bot) / 2;
      scaled = scale_volume(vol, max);
      if (cd_vol == scaled)
	break;
      if (cd_vol < scaled)
	top = vol - 1;
      else
	bot = vol + 1;
    }

  if (vol < 0)
    vol = 0;
  else if (vol > max)
    vol = max;

  return (vol);
} /* unscale_volume() */

/*---------------------------------------------------------------------*
 * Set the volume level for the left and right channels.  Their values
 * range from 0 to 100.
 *---------------------------------------------------------------------*/
int
gen_set_volume(struct wm_drive *d, int left, int right)
{
  struct ioc_vol vol;

  if (left < 0)	/* don't laugh, I saw this happen once! */
    left = 0;
  if (right < 0)
    right = 0;

  bzero((char *)&vol, sizeof(vol));

  vol.vol[LEFT_PORT] = left;
  vol.vol[RIGHT_PORT] = right;

  if (ioctl(d->fd, CDIOCSETVOL, &vol))
    return (-1);

  return (0);
} /* gen_set_volume() */

/*---------------------------------------------------------------------*
 * Read the initial volume from the drive, if available.  Each channel
 * ranges from 0 to 100, with -1 indicating data not available.
 *---------------------------------------------------------------------*/
int
gen_get_volume( struct wm_drive *d, int *left, int *right )
{
  struct ioc_vol vol;

  if (d->fd >= 0)
    {
      bzero((char *)&vol, sizeof(vol));

      if (ioctl(d->fd, CDIOCGETVOL, &vol))
	*left = *right = -1;
      else
	{
	  *left = vol.vol[LEFT_PORT];
	  *right = vol.vol[RIGHT_PORT];
	}
    } else {
      *left = *right = -1;
    }
  return (0);
} /* gen_get_volume() */

int gen_scale_volume(int *left, int *right)
{
	/* Adjust the volume to make up for the CD-ROM drive's weirdness. */
	*left = scale_volume(*left, 100);
	*right = scale_volume(*right, 100);

	return 0;
}

int gen_unscale_volume(int *left, int *right)
{
	*left = unscale_volume(*left, 100);
	*right = unscale_volume(*right, 100);

	return 0;
}

#endif
