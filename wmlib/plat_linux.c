/*
 * $Id: plat_linux.c 608128 2006-11-26 20:40:07Z kernalex $
 *
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
 * Linux-specific drive control routines.  Very similar to the Sun module.
 */

#if defined(__linux__)

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
/* Try to get around bug #29274 */
#include <linux/version.h>
#if 0
/* this breaks the build on ia64 and s390 for example.
   sys/types.h is already included and should provide __u64.
   please tell where we really need this and let's try to find
   a working #if case for everyone ... adrian@suse.de */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,50)) || (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,21) && LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
#undef __GNUC__
typedef unsigned long long __u64;
#endif
#endif

#include "include/wm_config.h"
#include "include/wm_struct.h"
#include "include/wm_cdtext.h"
#include <config.h>
#if defined(BSD_MOUNTTEST)
  #include <mntent.h>
#else
  /*
   * this is for glibc 2.x which defines ust structure in
   * ustat.h not stat.h
   */
  #ifdef __GLIBC__
    #include <sys/ustat.h>
  #endif
#endif


#include <sys/time.h>
#include <sys/ioctl.h>

#ifndef __GNUC__
#define __GNUC__ 1
#endif
/* needed for vanilla kernel headers, which do provide __u64 only
   for ansi */
#undef __STRICT_ANSI__
/* needed for non-ansi kernel headers */
#define asm __asm__
#define inline __inline__
#include <asm/types.h>
#include <linux/cdrom.h>
#undef asm
#undef inline

#include "include/wm_cdda.h"
#include "include/wm_struct.h"
#include "include/wm_platform.h"
#include "include/wm_cdrom.h"
#include "include/wm_scsi.h"
#include "include/wm_helpers.h"

#ifdef OSS_SUPPORT
#include <linux/soundcard.h>
#define CD_CHANNEL SOUND_MIXER_CD
#endif

#define WM_MSG_CLASS WM_MSG_CLASS_PLATFORM

#define max(a,b) ((a) > (b) ? (a) : (b))

#ifdef LINUX_SCSI_PASSTHROUGH
/* this is from <scsi/scsi_ioctl.h> */
# define SCSI_IOCTL_SEND_COMMAND 1
#endif

int gen_cdda_init( struct wm_drive *d );

int	min_volume = 0;
int	max_volume = 255;

#ifdef OSS_SUPPORT
int mixer;
char mixer_dev_name[20] = "/dev/mixer";
#endif

/*-------------------------------------------------------*
 *
 *
 *                   CD-ROM drive functions.
 *
 *
 *-------------------------------------------------------*/

/*--------------------------------------------------------*
 * Initialize the drive.  A no-op for the generic driver.
 *--------------------------------------------------------*/
int
gen_init( struct wm_drive *d )
{
  return (0);
} /* gen_init() */

/*---------------------------------------------------------------------------*
 * Open the CD device and figure out what kind of drive is attached.
 *---------------------------------------------------------------------------*/
int
wmcd_open( struct wm_drive *d )
{
  int fd;
  char vendor[32], model[32], rev[32];

  if (d->cd_device == NULL)
    d->cd_device = DEFAULT_CD_DEVICE;


  if (d->fd >= 0) { /* Device already open? */
/*      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_open(): [device is open (fd=%d)]\n", d->fd);*/
      return (0);
  }

  fd = open(d->cd_device, O_RDONLY | O_NONBLOCK);
  wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_open(): device=%s fd=%d\n", d->cd_device, fd);

  if (fd < 0)
    return -errno;

  /* Now fill in the relevant parts of the wm_drive structure. */
  d->fd = fd;

  /*
   * See if we can do digital audio.
   */
  if(d->cdda && gen_cdda_init(d)) {
    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_open(): failed in gen_cdda_init\n");
    gen_close(d);
    return -1;
  }

  /* Can we figure out the drive type? */
  if (wm_scsi_get_drive_type(d, vendor, model, rev)) {
    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_open(): inquiry failed\n");
    strcpy(vendor, "Generic");
    strcpy(model, "drive type");
    strcpy(rev, "");
  }

  if(find_drive_struct(vendor, model, rev) < 0) {
    gen_close(d);
    return -1;
  }

  if(d->proto->gen_init)
    return (d->proto->gen_init)(d);

  return 0;
} /* wmcd_open() */

/*
 * Re-Open the device if it is open.
 */
int
wmcd_reopen( struct wm_drive *d )
{
  int status;
  int tries = 0;

  do {
    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_reopen\n");
    gen_close(d);
    wm_susleep( 1000 );
    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "calls wmcd_open()\n");
    status = wmcd_open( d ); /* open it as usual */
    wm_susleep( 1000 );
    tries++;
  } while ( (status != 0) && (tries < 10)  );
  return status;
} /* wmcd_reopen() */

/*---------------------------------------------*
 * Send an arbitrary SCSI command to a device.
 *---------------------------------------------*/
int
wm_scsi( struct wm_drive *d, unsigned char *cdb, int cdblen,
	 void *retbuf, int retbuflen, int getreply )
{
#ifdef LINUX_SCSI_PASSTHROUGH

  char *cmd;
  int cmdsize;

  cmdsize = 2 * sizeof(int);
  if (retbuf)
    {
      if (getreply) cmdsize += max(cdblen, retbuflen);
      else cmdsize += (cdblen + retbuflen);
    }
  else cmdsize += cdblen;

  cmd = malloc(cmdsize);
  if (cmd == NULL)
    return (-1);

  ((int*)cmd)[0] = cdblen + ((retbuf && !getreply) ? retbuflen : 0);
  ((int*)cmd)[1] = ((retbuf && getreply) ? retbuflen : 0);

  memcpy(cmd + 2*sizeof(int), cdb, cdblen);
  if (retbuf && !getreply)
    memcpy(cmd + 2*sizeof(int) + cdblen, retbuf, retbuflen);

  if (ioctl(d->fd, SCSI_IOCTL_SEND_COMMAND, cmd))
    {
      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "%s: ioctl() failure\n", __FILE__);
      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "command buffer is:\n");
      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "%02x %02x %02x %02x %02x %02x\n",
        cmd[0],  cmd[1],  cmd[2],  cmd[3],  cmd[4],  cmd[5]);
      free(cmd);
      return (-1);
    }

  if (retbuf && getreply)
    memcpy(retbuf, cmd + 2*sizeof(int), retbuflen);

  free(cmd);
  return 0;

#else /* Linux SCSI passthrough*/
/*----------------------------------------*
 * send packet over cdrom interface
 * kernel >= 2.2.16
 *----------------------------------------*/
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,2,15))

  struct cdrom_generic_command cdc;
  struct request_sense sense;
  int capability;

  wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wm_scsi over CDROM_SEND_PACKET entered\n");

  capability = ioctl(d->fd, CDROM_GET_CAPABILITY);

  if(!(capability & CDC_GENERIC_PACKET))
  {
    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "your CDROM or/and kernel don't support CDC_GENERIC_PACKET ...\n");
    return -1;
  }

  memset(&cdc, 0, sizeof(struct cdrom_generic_command));
  memset(&sense, 0, sizeof(struct request_sense));

  memcpy(cdc.cmd, cdb, cdblen);

  cdc.buffer = retbuf;
  cdc.buflen = retbuflen;
  cdc.stat = 0;
  cdc.sense = &sense;
  cdc.data_direction = getreply?CGC_DATA_READ:CGC_DATA_WRITE;

  /* sendpacket_over_cdrom_interface() */
  return ioctl(d->fd, CDROM_SEND_PACKET, &cdc);
#endif /* CDROM_SEND_PACKET */
  printf("ERROR: this binary was compiled without CDROM GENERIC PACKET SUPPORT. kernel version < 2.2.16?\n");
  printf("ERROR: if you have a SCSI CDROM, rebuild it with a #define LINUX_SCSI_PASSTHROUGH\n");
  return (-1);
#endif
} /* wm_scsi */

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

/*--------------------------------*
 * Keep the CD open all the time.
 * disabled, analogous to 1.4b3
 *--------------------------------*
void
keep_cd_open( void )
{
	int	fd;
	struct flock	fl;
	extern	end;


	for (fd = 0; fd < 256; fd++)
		close(fd);

	if (fork())
		exit(0);

#if defined(O_NOFOLLOW)
	if ((fd = open("/tmp/cd.lock", O_RDWR | O_CREAT | O_NOFOLLOW, 0666)) < 0)
#else
	if ((fd = open("/tmp/cd.lock", O_RDWR | O_CREAT, 0666)) < 0)
#endif
		exit(0);
	fl.l_type = F_WRLCK;
	fl.l_whence = 0;
	fl.l_start = 0;
	fl.l_len = 0;
	if (fcntl(fd, F_SETLK, &fl) < 0)
		exit(0);

	if (open(cd_device, 0) >= 0)
	{
		brk(&end);
		pause();
	}

	exit(0);
}
*/

/*--------------------------------------------------------------------------*
 * Get the current status of the drive: the current play mode, the absolute
 * position from start of disc (in frames), and the current track and index
 * numbers if the CD is playing or paused.
 *--------------------------------------------------------------------------*/
int
gen_get_drive_status( struct wm_drive *d, int oldmode,
  int *mode, int *pos, int *track, int *ind )
{
  struct cdrom_subchnl sc;
  int ret;

#ifdef SBPCD_HACK
  static int prevpos = 0;
#endif


  /* Is the device open? */
  if (d->fd < 0) {
    ret = wmcd_open(d);
    if(ret < 0) /* error */
      return ret;

    if(ret == 1) {
      /* retry */
      *mode = WM_CDM_UNKNOWN;
      return 0;
    }
  }

  /* Try to get rid of the door locking    */
  /* Don't care about return value. If it  */
  /* works - fine. If not - ...            */
  ioctl(d->fd, CDROM_LOCKDOOR, 0);

  *mode = WM_CDM_UNKNOWN;

  sc.cdsc_format = CDROM_MSF;

  IFCDDA(d) {
    if(!cdda_get_drive_status(d, oldmode, mode, pos, track, ind)) {
      if(*mode == WM_CDM_STOPPED)
        *mode = WM_CDM_UNKNOWN; /* don't believe */
    }
  } else if(!ioctl(d->fd, CDROMSUBCHNL, &sc)) {
    switch (sc.cdsc_audiostatus) {
    case CDROM_AUDIO_PLAY:
      *mode = WM_CDM_PLAYING;
      *track = sc.cdsc_trk;
      *ind = sc.cdsc_ind;
      *pos = sc.cdsc_absaddr.msf.minute * 60 * 75 +
        sc.cdsc_absaddr.msf.second * 75 +
        sc.cdsc_absaddr.msf.frame;
#ifdef SBPCD_HACK
      if( *pos < prevpos ) {
        if( (prevpos - *pos) < 75 ) {
          *mode = WM_CDM_TRACK_DONE;
        }
      }

      prevpos = *pos;
#endif
      break;

    case CDROM_AUDIO_PAUSED:
      if (oldmode == WM_CDM_PLAYING || oldmode == WM_CDM_PAUSED) {
        *mode = WM_CDM_PAUSED;
        *track = sc.cdsc_trk;
        *ind = sc.cdsc_ind;
        *pos = sc.cdsc_absaddr.msf.minute * 60 * 75 +
          sc.cdsc_absaddr.msf.second * 75 +
          sc.cdsc_absaddr.msf.frame;
      } else
        *mode = WM_CDM_STOPPED;
      break;

    case CDROM_AUDIO_NO_STATUS:
        *mode = WM_CDM_STOPPED;
      break;

    case CDROM_AUDIO_COMPLETED:
      *mode = WM_CDM_TRACK_DONE; /* waiting for next track. */
      break;

    case CDROM_AUDIO_INVALID: /**/
    default:
      *mode = WM_CDM_UNKNOWN;
      break;
    }
  }

  if(WM_CDS_NO_DISC(*mode)) {
    /* verify status of drive */
    ret = ioctl(d->fd, CDROM_DRIVE_STATUS, 0/* slot */);
    if(ret == CDS_DISC_OK)
      ret = ioctl(d->fd, CDROM_DISC_STATUS, 0);
    switch(ret) {
    case CDS_NO_DISC:
      *mode = WM_CDM_NO_DISC;
      break;
    case CDS_TRAY_OPEN:
      *mode = WM_CDM_EJECTED;
      break;
    case CDS_AUDIO:
    case CDS_MIXED:
      *mode = WM_CDM_STOPPED;
      break;
    case CDS_DRIVE_NOT_READY:
    case CDS_NO_INFO:
    case CDS_DATA_1:
    case CDS_DATA_2:
    case CDS_XA_2_1:
    case CDS_XA_2_2:
    default:
      *mode = WM_CDM_UNKNOWN;
    }
  }

  return (0);
} /* gen_get_drive_status */

/*-------------------------------------*
 * Get the number of tracks on the CD.
 *-------------------------------------*/
int
gen_get_trackcount(struct wm_drive *d, int *tracks)
{
  struct cdrom_tochdr	hdr;

  if (ioctl(d->fd, CDROMREADTOCHDR, &hdr))
    return (-1);

  *tracks = hdr.cdth_trk1;
  return (0);
} /* gen_get_trackcount() */

/*---------------------------------------------------------*
 * Get the start time and mode (data or audio) of a track.
 *---------------------------------------------------------*/
int
gen_get_trackinfo(struct wm_drive *d, int track, int *data, int *startframe)
{
  struct cdrom_tocentry	entry;

  entry.cdte_track = track;
  entry.cdte_format = CDROM_MSF;

  if (ioctl(d->fd, CDROMREADTOCENTRY, &entry))
    return (-1);

  *startframe =	entry.cdte_addr.msf.minute * 60 * 75 +
    entry.cdte_addr.msf.second * 75 +
    entry.cdte_addr.msf.frame;
  *data = entry.cdte_ctrl & CDROM_DATA_TRACK ? 1 : 0;

  return (0);
}

/*-------------------------------------*
 * Get the number of frames on the CD.
 *-------------------------------------*/
int
gen_get_cdlen(struct wm_drive *d, int *frames)
{
  int tmp;

  return gen_get_trackinfo( d, CDROM_LEADOUT, &tmp, frames);
} /* gen_get_cdlen() */


/*------------------------------------------------------------*
 * Play the CD from one position to another (both in frames.)
 *------------------------------------------------------------*/
int
gen_play(struct wm_drive *d, int start, int end, int realstart)
{
  struct cdrom_msf msf;

  CDDARETURN(d) cdda_play(d, start, end, realstart);

  msf.cdmsf_min0 = start / (60*75);
  msf.cdmsf_sec0 = (start % (60*75)) / 75;
  msf.cdmsf_frame0 = start % 75;
  msf.cdmsf_min1 = end / (60*75);
  msf.cdmsf_sec1 = (end % (60*75)) / 75;
  msf.cdmsf_frame1 = end % 75;

  if (ioctl(d->fd, CDROMPLAYMSF, &msf)) {
    if (ioctl(d->fd, CDROMSTART))
      return (-1);
    if (ioctl(d->fd, CDROMPLAYMSF, &msf))
      return (-2);
  }

  /*
   * I hope no drive gets really confused after CDROMSTART
   * If so, I need to make this run-time configurable.
   *
#ifndef FAST_IDE
  if (ioctl( d->fd, CDROMSTART))
      return (-1);
#endif
  if (ioctl( d->fd, CDROMPLAYMSF, &msf ))
      return (-2);
    */

  return (0);
} /* gen_play() */

/*---------------*
 * Pause the CD.
 *---------------*/
int
gen_pause(struct wm_drive *d)
{
  CDDARETURN(d) cdda_pause(d);
  return (ioctl(d->fd, CDROMPAUSE));
}

/*-------------------------------------------------*
 * Resume playing the CD (assuming it was paused.)
 *-------------------------------------------------*/
int
gen_resume(struct wm_drive *d)
{
  CDDARETURN(d) cdda_pause(d);
  return (ioctl(d->fd, CDROMRESUME));
}

/*--------------*
 * Stop the CD.
 *--------------*/
int
gen_stop(struct wm_drive *d)
{
  CDDARETURN(d) cdda_stop(d);
  return (ioctl(d->fd, CDROMSTOP));
}


/*----------------------------------------*
 * Eject the current CD, if there is one.
 *----------------------------------------*/
int
gen_eject(struct wm_drive *d)
{
  struct stat	stbuf;
#if !defined(BSD_MOUNTTEST)
  struct ustat	ust;
#else
  struct mntent *mnt;
  FILE *fp;
#endif

  wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "ejecting?\n");

  if (fstat(d->fd, &stbuf) != 0) {
      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "that weird fstat() thingy\n");
      return (-2);
  }

  /* Is this a mounted filesystem? */
#if !defined(BSD_MOUNTTEST)
  if (ustat(stbuf.st_rdev, &ust) == 0)
    return (-3);
#else
  /*
   * This is the same test as in the WorkBone interface.
   * I should eliminate it there, because there is no need
   * for it in the UI
   */
  /* check if drive is mounted (from Mark Buckaway's cdplayer code) */
  /* Changed it again (look at XPLAYCD from ????                    */
  /* It's better to check the device name rather than one device is */
  /* mounted as iso9660. That prevents "no playing" if you have more*/
  /* than one CD-ROM, and one of them is mounted, but it's not the  */
  /* audio CD                                              -dirk    */
  if ((fp = setmntent (MOUNTED, "r")) == NULL)
    {
      wm_lib_message(WM_MSG_LEVEL_ERROR|WM_MSG_CLASS, "Could not open %s: %s\n", MOUNTED, strerror (errno));
      return(-3);
    }
  while ((mnt = getmntent (fp)) != NULL)
    {
      if (strcmp (mnt->mnt_fsname, d->cd_device) == 0)
	{
	  wm_lib_message(WM_MSG_LEVEL_ERROR|WM_MSG_CLASS, "CDROM already mounted (according to mtab). Operation aborted.\n");
	  endmntent (fp);
	  return(-3);
	}
    }
  endmntent (fp);
#endif /* BSD_MOUNTTEST */

  IFCDDA(d) {
    cdda_eject(d);
  }

  ioctl( d->fd, CDROM_LOCKDOOR, 0 );

  if (ioctl(d->fd, CDROMEJECT))
    {
      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "eject failed (%s).\n", strerror(errno));
      return (-1);
    }

  /*------------------
   * Things in "foobar_one" are left over from 1.4b3
   * I put them here for further observation. In 1.4b3, however,
   * that workaround didn't help at least for /dev/sbpcd
   * (The tray closed just after ejecting because re-opening the
   * device causes the tray to close)
   *------------------*/
#ifdef foobar_one
  extern int intermittent_dev
    /*
     * Some drives (drivers?) won't recognize a new CD if we leave the
     * device open.
     */
    if (intermittent_dev)
      gen_close(d);
#endif

  return (0);
} /* gen_eject() */

/*----------------------------------------*
 * Close the CD tray
 *----------------------------------------*/

int
gen_closetray(struct wm_drive *d)
{
#ifdef CAN_CLOSE
#ifdef CDROMCLOSETRAY
  wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "CDROMCLOSETRAY closing tray...\n");
  if (ioctl(d->fd, CDROMCLOSETRAY))
    return (-1);
#else
  wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_reopen() closing tray...\n");
  if(!gen_close(d))
    {
      return(wmcd_reopen(d));
    } else {
      return(-1);
    }
#endif /* CDROMCLOSETRAY */
#endif /* CAN_CLOSE */
  /* Always succeed if the drive can't close. */
  return(0);
} /* gen_closetray() */


/*------------------------------------------------------------------------*
 * scale_volume(vol, max)
 *
 * Return a volume value suitable for passing to the CD-ROM drive.  "vol"
 * is a volume slider setting; "max" is the slider's maximum value.
 * This is not used if sound card support is enabled.
 *
 *------------------------------------------------------------------------*/
static int
scale_volume( int vol, int max )
{
#ifdef CURVED_VOLUME
  return ((max * max - (max - vol) * (max - vol)) *
    (max_volume - min_volume) / (max * max) + min_volume);
#else
  return ((vol * (max_volume - min_volume)) / max + min_volume);
#endif
} /* scale_volume() */

static int
unscale_volume( int vol, int max )
{
#ifdef CURVED_VOLUME
  /* FIXME do it simpler */
  int tmp = (((max_volume - min_volume - vol) * max * max) - (vol + min_volume));
  return max - sqrt((tmp/(max_volume - min_volume)));
#else
  return (((vol - min_volume) * max) / (max_volume - min_volume));
#endif
} /* unscale_volume() */

/*---------------------------------------------------------------------*
 * Set the volume level for the left and right channels.  Their values
 * range from 0 to 100.
 *---------------------------------------------------------------------*/
int
gen_set_volume( struct wm_drive *d, int left, int right )
{
  struct cdrom_volctrl v;

  CDDARETURN(d) cdda_set_volume(d, left, right);

  /* Adjust the volume to make up for the CD-ROM drive's weirdness. */
  left = scale_volume(left, 100);
  right = scale_volume(right, 100);

  v.channel0 = v.channel2 = left < 0 ? 0 : left > 255 ? 255 : left;
  v.channel1 = v.channel3 = right < 0 ? 0 : right > 255 ? 255 : right;

  return (ioctl(d->fd, CDROMVOLCTRL, &v));
} /* gen_set_volume() */

/*---------------------------------------------------------------------*
 * Read the volume from the drive, if available.  Each channel
 * ranges from 0 to 100, with -1 indicating data not available.
 *---------------------------------------------------------------------*/
int
gen_get_volume( struct wm_drive *d, int *left, int *right )
{
  struct cdrom_volctrl v;

  CDDARETURN(d) cdda_get_volume(d, left, right);

#if defined(CDROMVOLREAD)
  if(!ioctl(d->fd, CDROMVOLREAD, &v)) {
    *left = unscale_volume((v.channel0 + v.channel2)/2, 100);
    *right = unscale_volume((v.channel1 + v.channel3)/2, 100);
  } else
#endif
    /* Suns, HPs, Linux, NEWS can't read the volume; oh well */
    *left = *right = -1;

  return 0;
} /* gen_get_volume() */

/*------------------------------------------------------------------------*
 * gen_get_cdtext(drive, buffer, length)
 *
 * Return a buffer with cdtext-stream. buffer will be allocated and filled
 *
 * needs send packet interface -> for IDE, linux at 2.2.16
 * depends on scsi.c which depends on wm_scsi defined in here
 * (which also takes care of IDE drives)
 *------------------------------------------------------------------------*/

int
gen_get_cdtext(struct wm_drive *d, unsigned char **pp_buffer, int *p_buffer_lenght)
{
  return wm_scsi_get_cdtext(d, pp_buffer, p_buffer_lenght);
} /* gen_get_cdtext() */

#endif /* __linux__ */
