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

#ifdef LINUX_SCSI_PASSTHROUGH
/* this is from <scsi/scsi_ioctl.h> */
# define SCSI_IOCTL_SEND_COMMAND 1
#endif

/*-------------------------------------------------------*
 *
 *
 *                   CD-ROM drive functions.
 *
 *
 *-------------------------------------------------------*/
int gen_init(struct wm_drive *d)
{
	return 0;
} 

int gen_open(struct wm_drive *d)
{
	if(d->fd > -1) {
		wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "plat_open(): [device is open (fd=%d)]\n",
			d->fd);
		return 0;
	}

	d->fd = open(d->cd_device, O_RDONLY | O_NONBLOCK);
	wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "plat_open(): device=%s fd=%d\n",
		d->cd_device, d->fd);

	if(d->fd < 0)
		return -errno;

	return 0;
}

int gen_close(struct wm_drive *d)
{
	wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "linux_close(): closing the device\n");

	close(d->fd);
	d->fd = -1;

	return 0;
}

/*--------------------------------------------------------------------------*
 * Get the current status of the drive: the current play mode, the absolute
 * position from start of disc (in frames), and the current track and index
 * numbers if the CD is playing or paused.
 *--------------------------------------------------------------------------*/
int gen_get_drive_status(struct wm_drive *d, int oldmode, int *mode, int *pos, int *track, int *ind)
{
	struct cdrom_subchnl sc;
	int ret;

#ifdef SBPCD_HACK
	static int prevpos = 0;
#endif

	/* Is the device open? */
	if (d->fd > -1) {
		ret = d->proto.open(d);
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
	
	if(!ioctl(d->fd, CDROMSUBCHNL, &sc)) {
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

	return 0;
}

/*-------------------------------------*
 * Get the number of tracks on the CD.
 *-------------------------------------*/
int gen_get_trackcount(struct wm_drive *d, int *tracks)
{
	struct cdrom_tochdr hdr;

	if(ioctl(d->fd, CDROMREADTOCHDR, &hdr))
		return -1;

	*tracks = hdr.cdth_trk1;
	return 0;
}

/*---------------------------------------------------------*
 * Get the start time and mode (data or audio) of a track.
 *---------------------------------------------------------*/
int gen_get_trackinfo(struct wm_drive *d, int track, int *data, int *startframe)
{
	struct cdrom_tocentry entry;

	entry.cdte_track = track;
	entry.cdte_format = CDROM_MSF;

	if(ioctl(d->fd, CDROMREADTOCENTRY, &entry))
		return -1;

	*startframe = entry.cdte_addr.msf.minute * 60 * 75 +
		entry.cdte_addr.msf.second * 75 +
		entry.cdte_addr.msf.frame;
	*data = entry.cdte_ctrl & CDROM_DATA_TRACK ? 1 : 0;

	return 0;
}

/*-------------------------------------*
 * Get the number of frames on the CD.
 *-------------------------------------*/
int gen_get_cdlen(struct wm_drive *d, int *frames)
{
	int tmp;

	return d->proto.get_trackinfo(d, CDROM_LEADOUT, &tmp, frames);
}

/*------------------------------------------------------------*
 * Play the CD from one position to another (both in frames.)
 *------------------------------------------------------------*/
int gen_play(struct wm_drive *d, int start, int end)
{
	struct cdrom_msf msf;

	msf.cdmsf_min0 = start / (60*75);
	msf.cdmsf_sec0 = (start % (60*75)) / 75;
	msf.cdmsf_frame0 = start % 75;
	msf.cdmsf_min1 = end / (60*75);
	msf.cdmsf_sec1 = (end % (60*75)) / 75;
	msf.cdmsf_frame1 = end % 75;

	if(ioctl(d->fd, CDROMPLAYMSF, &msf)) {
		if(ioctl(d->fd, CDROMSTART))
			return -1;
		if(ioctl(d->fd, CDROMPLAYMSF, &msf))
			return -2;
	}

	return 0;
}

/*---------------*
 * Pause the CD.
 *---------------*/
int gen_pause(struct wm_drive *d)
{
	return ioctl(d->fd, CDROMPAUSE);
}

/*-------------------------------------------------*
 * Resume playing the CD (assuming it was paused.)
 *-------------------------------------------------*/
int gen_resume(struct wm_drive *d)
{
	return ioctl(d->fd, CDROMRESUME);
}

/*--------------*
 * Stop the CD.
 *--------------*/
int gen_stop(struct wm_drive *d)
{
	return ioctl(d->fd, CDROMSTOP);
}

/*----------------------------------------*
 * Eject the current CD, if there is one.
 *----------------------------------------*/
int gen_eject(struct wm_drive *d)
{
	struct stat stbuf;
#if !defined(BSD_MOUNTTEST)
	struct ustat ust;
#else
	struct mntent *mnt;
	FILE *fp;
#endif

	wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "ejecting?\n");
	
	if(fstat(d->fd, &stbuf) != 0) {
		wm_lib_message(WM_MSG_LEVEL_ERROR|WM_MSG_CLASS, "that weird fstat() thingy\n");
		return -2;
	}

  /* Is this a mounted filesystem? */
#if !defined(BSD_MOUNTTEST)
	if (ustat(stbuf.st_rdev, &ust) == 0)
		return -3;
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
	if((fp = setmntent (MOUNTED, "r")) == NULL) {
		wm_lib_message(WM_MSG_LEVEL_ERROR|WM_MSG_CLASS, "Could not open %s: %s\n",
			MOUNTED, strerror (errno));
		return -3;
	}

	while((mnt = getmntent (fp)) != NULL) {
		if(strcmp (mnt->mnt_fsname, d->cd_device) == 0) {
			wm_lib_message(WM_MSG_LEVEL_ERROR|WM_MSG_CLASS,
				"CDROM already mounted (according to mtab). Operation aborted.\n");
			endmntent (fp);
			return -3;
		}
	}
	endmntent (fp);
#endif /* BSD_MOUNTTEST */

	ioctl(d->fd, CDROM_LOCKDOOR, 0);

	if(ioctl(d->fd, CDROMEJECT)) {
		wm_lib_message(WM_MSG_LEVEL_ERROR|WM_MSG_CLASS, "eject failed (%s).\n", strerror(errno));
		return -1;
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
	if(intermittent_dev)
		d->proto.close(d);
#endif

	return 0;
}

int gen_closetray(struct wm_drive *d)
{
#ifdef CDROMCLOSETRAY
	wm_lib_message(WM_MSG_LEVEL_ERROR|WM_MSG_CLASS, "CDROMCLOSETRAY closing tray...\n");
	return ioctl(d->fd, CDROMCLOSETRAY);
#else
	return -1;
#endif /* CDROMCLOSETRAY */
}

static int min_volume = 0, max_volume = 255;
/*------------------------------------------------------------------------*
 * scale_volume(vol, max)
 *
 * Return a volume value suitable for passing to the CD-ROM drive.  "vol"
 * is a volume slider setting; "max" is the slider's maximum value.
 * This is not used if sound card support is enabled.
 *
 *------------------------------------------------------------------------*/
static int scale_volume(int vol, int max)
{
#ifdef CURVED_VOLUME
	return ((max * max - (max - vol) * (max - vol)) *
	track_info_status(max_volume - min_volume) / (max * max) + min_volume);
#else
	return ((vol * (max_volume - min_volume)) / max + min_volume);
#endif
}

static int unscale_volume(int vol, int max)
{
#ifdef CURVED_VOLUME
	/* FIXME do it simpler */
	int tmp = (((max_volume - min_volume - vol) * max * max) - (vol + min_volume));
	return max - sqrt((tmp/(max_volume - min_volume)));
#else
	return (((vol - min_volume) * max) / (max_volume - min_volume));
#endif
}

/*---------------------------------------------------------------------*
 * Set the volume level for the left and right channels.  Their values
 * range from 0 to 100.
 *---------------------------------------------------------------------*/
int gen_set_volume(struct wm_drive *d, int left, int right)
{
	struct cdrom_volctrl v;
	
	v.channel0 = v.channel2 = left < 0 ? 0 : left > 255 ? 255 : left;
	v.channel1 = v.channel3 = right < 0 ? 0 : right > 255 ? 255 : right;

	return (ioctl(d->fd, CDROMVOLCTRL, &v));
}

/*---------------------------------------------------------------------*
 * Read the volume from the drive, if available.  Each channel
 * ranges from 0 to 100, with -1 indicating data not available.
 *---------------------------------------------------------------------*/
int gen_get_volume(struct wm_drive *d, int *left, int *right)
{
	struct cdrom_volctrl v;

#if defined(CDROMVOLREAD)
	if(!ioctl(d->fd, CDROMVOLREAD, &v)) {
		*left = (v.channel0 + v.channel2)/2;
		*right = (v.channel1 + v.channel3)/2;
	} else
#endif
		/* Suns, HPs, Linux, NEWS can't read the volume; oh well */
		*left = *right = -1;

	return 0;
}

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

/*---------------------------------------------*
 * Send an arbitrary SCSI command to a device.
 *---------------------------------------------*/
int gen_scsi(struct wm_drive *d, unsigned char *cdb, int cdblen,
	void *retbuf, int retbuflen, int getreply)
{
	int ret;
#ifdef LINUX_SCSI_PASSTHROUGH

	char *cmd;
	int cmdsize;

	cmdsize = 2 * sizeof(int);
	if(retbuf) {
		if (getreply)
			cmdsize += max(cdblen, retbuflen);
		else
			cmdsize += (cdblen + retbuflen);
	} else {
		cmdsize += cdblen;
	}

	cmd = malloc(cmdsize);
	if(cmd == NULL) {
		return -ENOMEM;
	}
	((int*)cmd)[0] = cdblen + ((retbuf && !getreply) ? retbuflen : 0);
	((int*)cmd)[1] = ((retbuf && getreply) ? retbuflen : 0);

	memcpy(cmd + 2*sizeof(int), cdb, cdblen);
	if(retbuf && !getreply)
		memcpy(cmd + 2*sizeof(int) + cdblen, retbuf, retbuflen);

	if(ioctl(d->fd, SCSI_IOCTL_SEND_COMMAND, cmd)) {
		wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "%s: ioctl(SCSI_IOCTL_SEND_COMMAND) failure\n", __FILE__);
		wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "command buffer is:\n");
		wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "%02x %02x %02x %02x %02x %02x\n",
		cmd[0],  cmd[1],  cmd[2],  cmd[3],  cmd[4],  cmd[5]);
		free(cmd);
		return -1;
	}

	if(retbuf && getreply)
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
	
	if(!(capability & CDC_GENERIC_PACKET)) {
		wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS,
			"your CDROM or/and kernel don't support CDC_GENERIC_PACKET ...\n");
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
	if((ret = ioctl(d->fd, CDROM_SEND_PACKET, &cdc)))
		wm_lib_message(WM_MSG_LEVEL_ERROR|WM_MSG_CLASS,
			"ERROR: CDROM_SEND_PACKET %s\n", strerror(errno));
	return ret;
#endif /* CDROM_SEND_PACKET */
	wm_lib_message(WM_MSG_LEVEL_ERROR|WM_MSG_CLASS,
		"ERROR: this binary was compiled without CDROM GENERIC PACKET SUPPORT. kernel version < 2.2.16?\n");
	wm_lib_message(WM_MSG_LEVEL_ERROR|WM_MSG_CLASS,
		"ERROR: if you have a SCSI CDROM, rebuild it with a #define LINUX_SCSI_PASSTHROUGH\n");
	return -1;
#endif
}

#if 0
#define CDROM_LBA         0x01   /* "logical block": first frame is #0 */
#define CDROMREADAUDIO    0x530e /* (struct cdrom_read_audio) */
#define CD_MSF_OFFSET     150    /* MSF numbering offset of first frame */

/* This struct is used by the CDROMREADAUDIO ioctl */
struct cdrom_read_audio
{
	union {
		struct {
			unsigned char minute;
			unsigned char second;
			unsigned char frame;
		} msf;
		signed int			lba;
	} addr; /* frame address */
	unsigned char addr_format;      /* CDROM_LBA or CDROM_MSF */
	signed int nframes;           /* number of 2352-byte-frames to read at once */
	unsigned char *buf;      /* frame buffer (size: nframes*2352 bytes) */
};

#define CDDABLKSIZE 2352

#endif

int gen_cdda_init(struct wm_drive *d)
{
	return 0;
}

/*
 * Initialize the CDDA data buffer and open the appropriate device.
 *
 */
int gen_cdda_open(struct wm_drive *d)
{
	int i;
	struct cdrom_read_audio cdda;

	if (d->fd > -1)
		return -1;

	for (i = 0; i < d->numblocks; i++) {
		d->blocks[i].buflen = d->frames_at_once * CD_FRAMESIZE_RAW;
		d->blocks[i].buf = malloc(d->blocks[i].buflen);
		if (!d->blocks[i].buf) {
			ERRORLOG("plat_cdda_open: ENOMEM\n");
			return -ENOMEM;
		}
	}

	cdda.addr_format = CDROM_LBA;
	cdda.addr.lba = 200;
	cdda.nframes = 1;
	cdda.buf = (unsigned char *)d->blocks[0].buf;
	
	d->status = WM_CDM_STOPPED;
	if((ioctl(d->fd, CDROMREADAUDIO, &cdda) < 0)) {
		if (errno == ENXIO) {
			/* CD ejected! */
			d->status = WM_CDM_EJECTED;
		} else {
			/* Sometimes it fails once, dunno why */
			d->status = WM_CDM_CDDAERROR;
		}
	} else {
		d->status = WM_CDM_UNKNOWN;
	}


	return 0;
}

/*
 * Read some blocks from the CD.  Stop if we hit the end of the current region.
 *
 * Returns number of bytes read, -1 on error, 0 if stopped for a benign reason.
 */
int gen_cdda_read(struct wm_drive *d, struct wm_cdda_block *block)
{
	struct cdrom_read_audio cdda;

	if (d->fd > -1)
		return -1;

	/* Hit the end of the CD, probably. */
	if (d->current_position >= d->ending_position) {
		block->status = WM_CDM_TRACK_DONE;
		return 0;
	}

	cdda.addr_format = CDROM_LBA;
	cdda.addr.lba = d->current_position - CD_MSF_OFFSET;
	if (d->ending_position && d->current_position + d->frames_at_once > d->ending_position)
		cdda.nframes = d->ending_position - d->current_position;
	else
		cdda.nframes = d->frames_at_once;

	cdda.buf = (unsigned char*)block->buf;

	if (ioctl(d->fd, CDROMREADAUDIO, &cdda) < 0) {
		if (errno == ENXIO) {
			/* CD ejected! */
			block->status = WM_CDM_EJECTED;
			return 0;
		} else {
			/* Sometimes it fails once, dunno why */
			block->status = WM_CDM_CDDAERROR;
			return 0;
		}
	}

	block->track =  -1;
	block->index =  0;
	block->frame  = d->current_position;
	block->status = WM_CDM_PLAYING;
	block->buflen = cdda.nframes * CD_FRAMESIZE_RAW;

	d->current_position = d->current_position + cdda.nframes;

	return block->buflen;
}

/*
 * Close the CD-ROM device in preparation for exiting.
 */
int gen_cdda_close(struct wm_drive *d)
{
	int i;

	if (d->fd > -1)
		return -1;

	for (i = 0; i < d->numblocks; i++) {
		free(d->blocks[i].buf);
		d->blocks[i].buf = 0;
		d->blocks[i].buflen = 0;
	}

	return 0;
}

#endif /* __linux__ */
