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
 * Sun-specific drive control routines.
 */

#if defined(sun) || defined(__sun)

#include "include/wm_config.h"
#include "include/wm_helpers.h"
#include "include/wm_cdrom.h"
#include "include/wm_cdtext.h"

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <ustat.h>
#include <unistd.h>
#include <signal.h>
#ifdef solbourne
# include <mfg/dklabel.h>
# include <mfg/dkio.h>
# include <sys/unistd.h>
# include <dev/srvar.h>
#else /* A real Sun */
# ifdef SYSV
#  include <poll.h>
#  include <stdlib.h>
#  include <sys/cdio.h>
#  include <sys/socket.h>
#  include <sys/scsi/impl/uscsi.h>
#  include "include/wm_cdda.h"
# else
#  include <sys/buf.h>
#  include <sun/dkio.h>
#  include <scsi/targets/srdef.h>
#  include <scsi/impl/uscsi.h>
#  include <scsi/generic/commands.h>
# endif
#endif

#include "include/wm_struct.h"

#define WM_MSG_CLASS WM_MSG_CLASS_PLATFORM

int	min_volume = 0;
int	max_volume = 255;

static const char *sun_cd_device = NULL;
extern int	intermittent_dev;

int	current_end;

#if defined(SYSV) && defined(SIGTHAW)
#ifdef __GNUC__
void sigthawinit(void) __attribute__ ((constructor));
#else
#pragma init(sigthawinit)
#endif /* GNUC */

static int last_left, last_right;
static struct wm_drive *thecd = NULL;

/*
 * Handling for Sun's Suspend functionality
 */
static void
thawme(int sig)
{
// Just leave this line in as a reminder for a missing
// functionality in the GUI.
// change_mode(NULL, WM_CDM_STOPPED, NULL);
  codec_init();
  if( thecd )
    gen_set_volume(thecd, last_left, last_right);
} /* thawme() */

void
sigthawinit( void )
{
  struct sigaction sa;

  sa.sa_handler = thawme;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  sigaction(SIGTHAW, &sa, NULL);
} /* sigthawinit() */

#endif /* SYSV && SIGTHAW */

/*
 * find_cdrom
 *
 * Determine the name of the CD-ROM device.
 *
 * Use the first of /vol/dev/aliases/cdrom0, /dev/rdsk/c0t6d0s2, and /dev/rsr0
 * that exists.  (Check for /vol/dev/aliases, not cdrom0, since it won't be
 * there if there's no CD in the drive.)  This is done so a single SunOS 4.x
 * binary can be used on any 4.x or higher Sun system.
 */
const char*
find_cdrom()
{
  if (access("/vol/dev/aliases", X_OK) == 0)
    {
      /* Volume manager.  Device might not be there. */
      intermittent_dev = 1;

      /* If vold is running us, it'll tell us the device name. */
      sun_cd_device = getenv("VOLUME_DEVICE");
      /*
      ** the path of the device has to include /dev
      ** otherwise we are vulnerable to race conditions
      ** Thomas Biege <thomas@suse.de>
      */
      if (sun_cd_device == NULL ||
	  strncmp("/vol/dev/", sun_cd_device, 9) ||
	  strstr(sun_cd_device, "/../") )
	return "/vol/dev/aliases/cdrom0";
      else
        return sun_cd_device;
    }
  else if (access("/dev/rdsk/c0t6d0s2", F_OK) == 0)
    {
      /* Solaris 2.x w/o volume manager. */
      return "/dev/rdsk/c0t6d0s2";
    }
  else if (access("/dev/rcd0", F_OK) == 0)
    {
      return "/dev/rcd0";
    }
  else if (access("/dev/rsr0", F_OK) == 0)
    return "/dev/rsr0";
  else
    {
      fprintf(stderr, "Could not find a CD device!\n");
      return NULL;
    }
} /* find_cdrom() */

/*
 * Initialize the drive.  A no-op for the generic driver.
 */
int
gen_init( struct wm_drive *d )
{
  codec_init();
  return 0;
} /* gen_init() */


/*
 * Open the CD device and figure out what kind of drive is attached.
 */
int
gen_open( struct wm_drive *d )
{
	static int warned = 0;

	if (d->fd >= 0) {		/* Device already open? */
		wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "gen_open(): [device is open (fd=%d)]\n", d->fd);
		return 0;
    }


	d->fd = open(d->cd_device, 0);
	if (d->fd < 0) {
		/* Solaris 2.2 volume manager moves links around */
		if (errno == ENOENT && intermittent_dev)
			return 1;

		if (errno == EACCES) {
			if (!warned) {
              /* char realname[MAXPATHLEN];
				if (realpath(cd_device, realname) == NULL) {
					perror("realpath");
					return 1;
				} */
				return -EACCES;
			}
		} else if (errno != ENXIO) {
			return -6;
		}

		/* No CD in drive. */
		return 1;
	}

	thecd = d;

  return 0;
}

/*
 * Send an arbitrary SCSI command out the bus and optionally wait for
 * a reply if "retbuf" isn't NULL.
 */
int
gen_scsi( struct wm_drive *d,
	 unsigned char *cdb,
	 int cdblen, void *retbuf,
	 int retbuflen, int getreply )
{
#ifndef solbourne
  char			x;
  struct uscsi_cmd	cmd;

  memset(&cmd, 0, sizeof(cmd));
  cmd.uscsi_cdb = (void *) cdb;
  cmd.uscsi_cdblen = cdblen;
  cmd.uscsi_bufaddr = retbuf ? retbuf : (void *)&x;
  cmd.uscsi_buflen = retbuf ? retbuflen : 0;
  cmd.uscsi_flags = USCSI_ISOLATE | USCSI_SILENT;
  if (getreply)
    cmd.uscsi_flags |= USCSI_READ;

  if (ioctl(d->fd, USCSICMD, &cmd))
    return -1;

  if (cmd.uscsi_status)
    return -1;

  return 0;
#else
  return -1;
#endif
}

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

/* Alarm signal handler. */
static void do_nothing( int x ) { x++; }

/*
 * Get the current status of the drive: the current play mode, the absolute
 * position from start of disc (in frames), and the current track and index
 * numbers if the CD is playing or paused.
 */
int
gen_get_drive_status( struct wm_drive *d,
		      int oldmode,
		      int *mode,
		      int *pos, int *track, int *index )
{
  struct cdrom_subchnl		sc;
  struct itimerval		old_timer, new_timer;
  struct sigaction		old_sig, new_sig;

  /* If we can't get status, the CD is ejected, so default to that. */
  *mode = WM_CDM_EJECTED;

  /* Is the device open? */
  if (d->fd < 0) {
    switch (d->proto.open(d)) {
	case -1:	/* error */
	  return -1;

	case 1:		/* retry */
	  return 0;
	}
  }

  /*
   * Solaris 2.2 hangs on this ioctl if someone else ejects the CD.
   * So we schedule a signal to break out of the hang if the call
   * takes an unreasonable amount of time.  The signal handler and
   * timer are restored immediately to avoid interfering with XView.
   */
  if (intermittent_dev) {
	/*
	* First clear out the timer so XView's signal doesn't happen
	* while we're diddling with the signal handler.
	*/
	timerclear(&new_timer.it_interval);
	timerclear(&new_timer.it_value);
	setitimer(ITIMER_REAL, &new_timer, &old_timer);

	/*
	* Now install the no-op signal handler.
	*/
	new_sig.sa_handler = do_nothing;
	memset(&new_sig.sa_mask, 0, sizeof(new_sig.sa_mask));
	new_sig.sa_flags = 0;
	if (sigaction(SIGALRM, &new_sig, &old_sig))
      perror("sigaction");

	/*
	* And finally, set the timer.
	*/
	new_timer.it_value.tv_sec = 2;
	setitimer(ITIMER_REAL, &new_timer, NULL);
  }

  sc.cdsc_format = CDROM_MSF;

  if (ioctl(d->fd, CDROMSUBCHNL, &sc)) {
    if (intermittent_dev) {
	  sigaction(SIGALRM, &old_sig, NULL);
	  setitimer(ITIMER_REAL, &old_timer, NULL);

	  /* If the device can disappear, let it do so. */
	  d->proto.close(d);
	}

    return 0;
  }

  if (intermittent_dev) {
    sigaction(SIGALRM, &old_sig, NULL);
    setitimer(ITIMER_REAL, &old_timer, NULL);
  }

  switch (sc.cdsc_audiostatus) {
  case CDROM_AUDIO_PLAY:
    *mode = WM_CDM_PLAYING;
    *track = sc.cdsc_trk;
    *index = sc.cdsc_ind;
    *pos = sc.cdsc_absaddr.msf.minute * 60 * 75 +
      sc.cdsc_absaddr.msf.second * 75 +
      sc.cdsc_absaddr.msf.frame;
    break;

  case CDROM_AUDIO_PAUSED:
  case CDROM_AUDIO_INVALID:
  case CDROM_AUDIO_NO_STATUS:
    if (oldmode == WM_CDM_PLAYING || oldmode == WM_CDM_PAUSED)
      {
	*mode = WM_CDM_PAUSED;
	*track = sc.cdsc_trk;
	*index = sc.cdsc_ind;
	*pos = sc.cdsc_absaddr.msf.minute * 60 * 75 +
	  sc.cdsc_absaddr.msf.second * 75 +
	  sc.cdsc_absaddr.msf.frame;
      }
    else
      *mode = WM_CDM_STOPPED;
    break;

    /* CD ejected manually during play. */
  case CDROM_AUDIO_ERROR:
    break;

  case CDROM_AUDIO_COMPLETED:
    *mode = WM_CDM_TRACK_DONE; /* waiting for next track. */
    break;

  default:
    *mode = WM_CDM_UNKNOWN;
    break;
  }

  return 0;
} /* gen_get_drive_status() */

/*
 * Get the number of tracks on the CD.
 */
int
gen_get_trackcount( struct wm_drive *d, int *tracks )
{
  struct cdrom_tochdr	hdr;

  if (ioctl(d->fd, CDROMREADTOCHDR, &hdr))
    return -1;

  *tracks = hdr.cdth_trk1;
  return 0;
} /* gen_get_trackcount() */

/*
 * Get the start time and mode (data or audio) of a track.
 */
int
gen_get_trackinfo( struct wm_drive *d, int track, int *data, int *startframe)
{
  struct cdrom_tocentry	entry;

  entry.cdte_track = track;
  entry.cdte_format = CDROM_MSF;

  if (ioctl(d->fd, CDROMREADTOCENTRY, &entry))
    return -1;

  *startframe =	entry.cdte_addr.msf.minute * 60 * 75 +
    entry.cdte_addr.msf.second * 75 +
    entry.cdte_addr.msf.frame;
  *data = entry.cdte_ctrl & CDROM_DATA_TRACK ? 1 : 0;

  return 0;
} /* gen_get_trackinfo() */

/*
 * Get the number of frames on the CD.
 */
int
gen_get_cdlen(struct wm_drive *d, int *frames )
{
  int		tmp;

  return (gen_get_trackinfo(d, CDROM_LEADOUT, &tmp, frames));
} /* gen_get_cdlen() */

/*
 * Play the CD from one position to another.
 *
 *	d		Drive structure.
 *	start		Frame to start playing at.
 *	end		End of this chunk.
 */
int
gen_play( struct wm_drive *d, int start, int end)
{
  struct cdrom_msf		msf;
  unsigned char			cmdbuf[10];

  current_end = end;

  msf.cdmsf_min0 = start / (60*75);
  msf.cdmsf_sec0 = (start % (60*75)) / 75;
  msf.cdmsf_frame0 = start % 75;
  msf.cdmsf_min1 = end / (60*75);
  msf.cdmsf_sec1 = (end % (60*75)) / 75;
  msf.cdmsf_frame1 = end % 75;

  codec_start();
  if (ioctl(d->fd, CDROMSTART))
    return -1;
  if (ioctl(d->fd, CDROMPLAYMSF, &msf))
    return -2;

  return 0;
} /* gen_play() */

/*
 * Pause the CD.
 */
int
gen_pause( struct wm_drive *d )
{
  codec_stop();
  return (ioctl(d->fd, CDROMPAUSE));
} /* gen_pause() */

/*
 * Resume playing the CD (assuming it was paused.)
 */
int
gen_resume( struct wm_drive *d )
{
  codec_start();
  return (ioctl(d->fd, CDROMRESUME));
} /* gen_resume() */

/*
 * Stop the CD.
 */
int
gen_stop( struct wm_drive *d )
{
  codec_stop();
  return (ioctl(d->fd, CDROMSTOP));
} /* gen_stop() */

/*
 * Eject the current CD, if there is one.
 */
int
gen_eject( struct wm_drive *d )cddax
{
  struct stat	stbuf;
  struct ustat	ust;

  if (fstat(d->fd, &stbuf) != 0)
    return -2;

  /* Is this a mounted filesystem? */
  if (ustat(stbuf.st_rdev, &ust) == 0)
    return -3;

  if (ioctl(d->fd, CDROMEJECT))
    return -1;

  /* Close the device if it needs to vanish. */
  if (intermittent_dev) {
	d->proto.close(d);
	/* Also remember to tell the cddaslave since volume
	manager switches links around on us */
	if (d->cdda_slave > -1) {
	  write(d->cdda_slave, "E", 1);
	}
  }

  return 0;
} /* gen_eject() */

/*----------------------------------------*
 * Close the CD tray
 *
 * Please edit and send changes to
 * milliByte@DeathsDoor.com
 *----------------------------------------*/

int
gen_closetray(struct wm_drive *d)
{
	return -1;
} /* gen_closetray() */

/*
 * Set the volume level for the left and right channels.  Their values
 * range from 0 to 100.
 */
int
gen_set_volume( struct wm_drive *d, int left, int right )
{
  struct cdrom_volctrl v;

#if defined(SIGTHAW) && defined(SYSV)
  last_left = left;
  last_right = right;
  thecd = d;
#endif

  CDDARETURN(d) cdda_set_volume(d, left, right);

  left = (left * (max_volume - min_volume)) / 100 + min_volume;
  right = (right * (max_volume - min_volume)) / 100 + min_volume;

  v.channel0 = left < 0 ? 0 : left > 255 ? 255 : left;
  v.channel1 = right < 0 ? 0 : right > 255 ? 255 : right;

  return (ioctl(d->fd, CDROMVOLCTRL, &v));
} /* gen_set_volume() */

/*
 * Read the volume from the drive, if available.  Each channel
 * ranges from 0 to 100, with -1 indicating data not available.
 */
int
gen_get_volume( struct wm_drive *d, int *left, int *right )
{
  CDDARETURN(d) cdda_get_volume(d, left, right);

  *left = *right = -1;

  return (wm_scsi2_get_volume(d, left, right));
} /* gen_get_volume() */

#define CDDABLKSIZE 2368
#define SAMPLES_PER_BLK 588

/*
 * This is the fastest way to convert from BCD to 8-bit.
 */
static unsigned char unbcd[256] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0,0,0,0,0,0,
 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,  0,0,0,0,0,0,
 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,  0,0,0,0,0,0,
 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,  0,0,0,0,0,0,
 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,  0,0,0,0,0,0,
 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,  0,0,0,0,0,0,
 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,  0,0,0,0,0,0,
 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,  0,0,0,0,0,0,
 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,  0,0,0,0,0,0,
 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,  0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
/*
 * Try to initialize the CDDA slave.  Returns 0 on success.
 */
int
gen_cdda_init( struct wm_drive *d )
{
	enable_cdda_controls(1);
	return 0;
}

/*
 * Initialize the CDDA data buffer and open the appropriate device.
 *
 */
int gen_cdda_open(struct wm_drive *d)
{
	int i;
	struct cdrom_cdda cdda;

	if (d->fd < 0)
		return -1;

	for (i = 0; i < d->numblocks; ++i) {
		d->blocks[i].buflen = d->frames_at_once * CDDABLKSIZE;
		d->blocks[i].buf = malloc(d->blocks[i].buflen);
		if (!d->blocks[i].buf) {
			ERRORLOG("plat_cdda_open: ENOMEM\n");
			return -ENOMEM;
		}
	}

	cdda.cdda_addr = 200;
	cdda.cdda_length = 1;
	cdda.cdda_data = d->blocks[0].buf;
	cdda.cdda_subcode = CDROM_DA_SUBQ;

	d->status = WM_CDM_STOPPED;
	if((ioctl(d->fd, CDROMCDDA, &cdda) < 0)) {
		return -1;
	}

	return 0;
}

/*
 * Normalize a bunch of CDDA data.  Basically this means ripping out the
 * Q subchannel data and doing byte-swapping, since the CD audio is in
 * littleendian format.
 *
 * Scanning is handled here too.
 *
 * XXX - do byte swapping on Intel boxes?
 */
long
sun_normalize(struct cdda_block *block)
{
	int	i, nextq;
	long buflen = block->buflen;
	int	blocks = buflen / CDDABLKSIZE;
	unsigned char *rawbuf = block->buf;
	unsigned char *dest = rawbuf;
	unsigned char tmp;
	long *buf32 = (long *)rawbuf, tmp32;

/*
 * this was #ifndef LITTLEENDIAN
 * in wmcdda it was called LITTLE_ENDIAN. Was this a flaw?
 */
#if WM_BIG_ENDIAN
	if (blocks--) {
		for (i = 0; i < SAMPLES_PER_BLK * 2; ++i) {
			/* Only need to use temp buffer on first block. */
			tmp = *rawbuf++;
			*dest++ = *rawbuf++;
			*dest++ = tmp;
		}
	}
#endif

	while (blocks--) {
		/* Skip over Q data. */
		rawbuf += 16;

		for (i = 0; i < SAMPLES_PER_BLK * 2; i++) {
#if WM_LITTLE_ENDIAN
			*dest++ = *rawbuf++;
			*dest++ = *rawbuf++;
#else
			*dest++ = rawbuf[1];
			*dest++ = rawbuf[0];
			rawbuf += 2;
#endif
		}
	}

	buflen -= ((buflen / CDDABLKSIZE) * 16);

	return buflen;
}

/*
 * Read some blocks from the CD.  Stop if we hit the end of the current region.
 *
 * Returns number of bytes read, -1 on error, 0 if stopped for a benign reason.
 */
int gen_cdda_read(struct wm_drive *d, struct wm_cdda_block *block)
{
	struct cdrom_cdda cdda;
	int blk;
	unsigned char *q;
	unsigned char* rawbuf = block->buf;

	if (d->fd < 0)
		return -1;

	/* Hit the end of the CD, probably. */
    if ((direction > 0 && d->current_position >= d->ending_position) ||
        (direction < 0 && d->current_position < d->starting_position)) {
        block->status = WM_CDM_TRACK_DONE;
        return 0;
    }

    cdda.cdda_addr = d->current_position - 150;
    if (d->ending_position && d->current_position + d->frames_at_once > d->ending_position)
        cdda.cdda_length = d->ending_position - d->current_position;
    else
        cdda.cdda_length = d->frames_at_once;
    cdda.cdda_data = (unsigned char*)block->buf;
    cdda.cdda_subcode = CDROM_DA_SUBQ;

    if (ioctl(d->fd, CDROMCDDA, &cdda) < 0) {
        if (errno == ENXIO)	{ /* CD ejected! */
            block->status = WM_CDM_EJECTED;
            return -1;
        }

        /* Sometimes it fails once, dunno why */
        if (ioctl(d->fd, CDROMCDDA, &cdda) < 0) {
            if (ioctl(d->fd, CDROMCDDA, &cdda) < 0)  {
                if (ioctl(d->fd, CDROMCDDA, &cdda) < 0) {
                    perror("CDROMCDDA");
                    block->status = WM_CDM_CDDAERROR;
                    return -1;
                }
            }
        }
    }

	d->current_position = d->current_position + cdda.cdda_length * direction;

#if 0
	/*
 	 * New valid Q-subchannel information?  Update the block
	 * status.
	 */
    for (blk = 0; blk < d->numblocks; ++blk) {
        q = &rawbuf[blk * CDDABLKSIZE + SAMPLES_PER_BLK * 4];
        if (*q == 1) {
            block->track =  unbcd[q[1]];
            block->index =  unbcd[q[2]];
            /*block->minute = unbcd[q[7]];
            block->second = unbcd[q[8]];*/
            block->frame =  unbcd[q[9]];
            block->status = WM_CDM_PLAYING;
            block->buflen = cdda.cdda_length;
        }
    }
#endif
    return sun_normalize(block);
}

/*
 * Close the CD-ROM device in preparation for exiting.
 */
int gen_cdda_close(struct wm_drive *d)
{
	int i;

	if (d->fd < 0)
		return -1;

	for (i = 0; i < d->numblocks; i++) {
		free(d->blocks[i].buf);
		d->blocks[i].buf = 0;
		d->blocks[i].buflen = 0;
	}

	return 0;
}

/*
 * The following code activates the internal CD audio passthrough on
 * SPARCstation 5 systems (and possibly others.)
 *
 * Thanks to <stevep@ctc.ih.att.com>, Roger Oscarsson <roger@cs.umu.se>
 * and Steve McKinty <>
 *
 * Most CD drives have a headphone socket on the front, but it
 * is often more convenient to route the audio though the
 * built-in audio device. That way the user can leave their
 * headphones plugged-in to the base system, for use with
 * other audio stuff like ShowMeTV
 */

#ifdef CODEC /* { */
#ifdef SYSV /* { */

# include <sys/ioctl.h>
# include <sys/audioio.h>
# include <stdlib.h>

#else /* } { */

# include <sun/audioio.h>
# define AUDIO_DEV_SS5STYLE 5
typedef int audio_device_t;

#endif /* } */
#endif /* } */

/*
 * Don't do anything with /dev/audio if we can't set it to high quality.
 * Also, don't do anything real if it's not Solaris.
 */
#if !defined(AUDIO_ENCODING_LINEAR) || !defined(CODEC) || !defined(SYSV) /* { */
codec_init() { return 0; }
codec_start() { return 0; }
codec_stop() { return 0; }
#else

#ifndef AUDIO_INTERNAL_CD_IN
#define AUDIO_INTERNAL_CD_IN	0x4
#endif

static char* devname = 0;
static char* ctlname = 0;
static int ctl_fd = -1;
static int port = AUDIO_LINE_IN;
int internal_audio = 1;

codec_init( void )
{
  register int i;
  char* ctlname;
  audio_info_t foo;
  audio_device_t aud_dev;

  if (internal_audio == 0)
    {
      ctl_fd = -1;
      return 0;
    }

  if (!(devname = getenv("AUDIODEV"))) devname = "/dev/audio";
  ctlname = strcat(strcpy(malloc(strlen(devname) + 4), devname), "ctl");
  if ((ctl_fd = open(ctlname, O_WRONLY, 0)) < 0)
    {
      perror(ctlname);
      return -1;
    }
  if (ioctl(ctl_fd, AUDIO_GETDEV, &aud_dev) < 0)
    {
      close(ctl_fd);
      ctl_fd = -1;
      return -1;
    }
  /*
   * Instead of filtering the "OLD_SUN_AUDIO", try to find the new ones.
   * Not sure if this is all correct.
   */
#ifdef SYSV
  if (strcmp(aud_dev.name, "SUNW,CS4231") &&
      strcmp(aud_dev.name, "SUNW,sb16") &&
      strcmp(aud_dev.name, "SUNW,sbpro"))
#else
    if (aud_dev != AUDIO_DEV_SS5STYLE)
#endif
      {
	close(ctl_fd);
	ctl_fd = -1;
	return 0;					/* but it's okay */
      }

  /*
   * Does the chosen device have an internal CD port?
   * If so, use it. If not then try and use the
   * Line In port.
   */
  if (ioctl(ctl_fd, AUDIO_GETINFO, &foo) < 0)
    {
      perror("AUDIO_GETINFO");
      close(ctl_fd);
      ctl_fd = -1;
      return -1;
    }
  if (foo.record.avail_ports & AUDIO_INTERNAL_CD_IN)
    port = AUDIO_INTERNAL_CD_IN;
  else
    port = AUDIO_LINE_IN;

  /*
   * now set it up to use it. See audio(7I)
   */

  AUDIO_INITINFO(&foo);
  foo.record.port = port;
  foo.record.balance = foo.play.balance = AUDIO_MID_BALANCE;

  if (d->cdda_slave > -1)
    foo.monitor_gain = 0;
  else
    foo.monitor_gain = AUDIO_MAX_GAIN;
  /*
   * These next ones are tricky. The voulme will depend on the CD drive
   * volume (set by the knob on the drive and/or by workman's volume
   * control), the audio device record gain and the audio device
   * play gain.  For simplicity we set the latter two to something
   * reasonable, but we don't force them to be reset if the user
   * wants to change them.
   */
  foo.record.gain = (AUDIO_MAX_GAIN * 80) / 100;
  foo.play.gain = (AUDIO_MAX_GAIN * 40) / 100;

  ioctl(ctl_fd, AUDIO_SETINFO, &foo);
  return 0;
}

static int
kick_codec( void )
{
  audio_info_t foo;
  int dev_fd;
  int retval = 0;

  /*
   * Open the audio device, not the control device. This
   * will fail if someone else has taken it.
   */

  if ((dev_fd = open(devname, O_WRONLY|O_NDELAY, 0)) < 0)
    {
      perror(devname);
      return -1;
    }

  AUDIO_INITINFO(&foo);
  foo.record.port = port;
  foo.monitor_gain = AUDIO_MAX_GAIN;

  /* These can only be set on the real device */
  foo.play.sample_rate = 44100;
  foo.play.channels = 2;
  foo.play.precision = 16;
  foo.play.encoding = AUDIO_ENCODING_LINEAR;

  if ((retval = ioctl(dev_fd, AUDIO_SETINFO, &foo)) < 0)
    perror(devname);

  close(dev_fd);
  return retval;
} /* kick_codec() */

codec_start( void )
{
  audio_info_t foo;

  if (ctl_fd < 0)
    return 0;

  if (ioctl(ctl_fd, AUDIO_GETINFO, &foo) < 0)
    return -1;

  if (foo.play.channels != 2) return kick_codec();
  if (foo.play.encoding != AUDIO_ENCODING_LINEAR) return kick_codec();
  if (foo.play.precision != 16) return kick_codec();
  if (foo.play.sample_rate != 44100) return kick_codec();

  if (foo.record.channels != 2) return kick_codec();
  if (foo.record.encoding != AUDIO_ENCODING_LINEAR) return kick_codec();
  if (foo.record.precision != 16) return kick_codec();
  if (foo.record.sample_rate != 44100) return kick_codec();

  if (foo.monitor_gain != AUDIO_MAX_GAIN) return kick_codec();
  if (foo.record.port != port) return kick_codec();

  return 0;
} /* codec_start() */

codec_stop( void ) { return 0; }

#endif /* CODEC } */

#endif /* sun */
