/***************************************************************************
                          cdda.c  -  description
                             -------------------
    begin                : Mon Jan 27 2003
    copyright            : (C) 2003 by Alex Kern
    email                : alex.kern@gmx.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This is a common cddamaster piece of code                             *
 *                                                                         *
 ***************************************************************************/

#include <string.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include "include/wm_config.h"
#include "include/wm_struct.h"
#include "include/wm_cdda.h"
#include "include/wm_cdrom.h"
#include "audio/audio.h"
#include <pthread.h>

#if defined(BUILD_CDDA)
struct cdda_block blk;
struct cdda_device dev;
pthread_t thread;

void* cdda_fct(void* arg);

int playing = 0; /* Should the CD be playing now? */
int pausing = 0;

/*
 * Loudness setting, plus the floating volume multiplier and decaying-average
 * volume level.
 */
unsigned int loudness = 0, volume = 32768, level;

/*
 * This is non-null if we're saving audio to a file.
 */
FILE *output = NULL;

/*
 * These are driverdependent oops
 *
 */
struct audio_oops *oops = NULL;

/*
 * Audio file header format.
 */
typedef unsigned long u_32;
struct auheader {
	u_32 magic;
	u_32 hdr_size;
	u_32 data_size;
	u_32 encoding;
	u_32 sample_rate;
	u_32 channels;
};

/* had to change #ifdef to #if   -> see wm_cdda.h */
#ifdef __FreeBSD__
/* Phungus not with htonl on FreeBSD */
#include <sys/param.h>
#else
#if WM_BIG_ENDIAN
# ifndef htonl
#  define htonl(x) (x)
# endif
#else
extern unsigned long htonl(unsigned long);
#endif
#endif

/*
 * Try to initialize the CDDA slave.  Returns 0 on success.
 */
int
gen_cdda_init( struct wm_drive *d )
{
	if (d->cdda_slave > -1)
		return 0;

	memset(&blk, 0, sizeof(struct cdda_block));

	dev.fd = -1;
	dev.block = &blk;
	wmcdda_init(&dev, dev.block);

	oops = setup_soundsystem(d->soundsystem, d->sounddevice, d->ctldevice);
	if (!oops) {
		ERRORLOG("cdda: setup_soundsystem failed\n");
		wmcdda_close(&dev);
		return -1;
	}

	if(pthread_create(&thread, NULL, cdda_fct, &dev)) {
		ERRORLOG("error by create pthread");
		oops->wmaudio_close();
		wmcdda_close(&dev);
		return -1;
	}

	d->cdda_slave = 0;
	return 0;
}

int
cdda_get_drive_status( struct wm_drive *d, int oldmode,
  int *mode, int *frame, int *track, int *ind )
{
	if (d->cdda_slave > -1) {
		*mode = blk.status;
		if (*mode == WM_CDM_PLAYING) {
			*track = blk.track;
			*ind = blk.index;
			*frame = blk.frame;
		} else if (*mode == WM_CDM_CDDAERROR) {
			/*
			 * An error near the end of the CD probably
			 * just means we hit the end.
			 */
			*mode = WM_CDM_TRACK_DONE;
		}
		return 0;
	}

	return -1;
}

int
cdda_play( struct wm_drive *d, int start, int end, int realstart )
{
	if (d->cdda_slave > -1) {
		blk.command = WM_CDM_STOPPED;

		wmcdda_setup(start, end, realstart);

		level = 2500;
		volume = 1 << 15;

		blk.track =  -1;
		blk.index =  0;
		blk.frame = start;
		blk.command = WM_CDM_PLAYING;

		return 0;
	}

	return -1;
}

int
cdda_pause( struct wm_drive *d )
{
	if (d->cdda_slave > -1) {
		if(WM_CDM_PLAYING == blk.status) {
			blk.command = WM_CDM_PAUSED;
		} else {
			blk.command = WM_CDM_PLAYING;
		}

		return 0;
	}
	
	return -1;
}

int
cdda_stop( struct wm_drive *d )
{
	if (d->cdda_slave > -1) {
		blk.command = WM_CDM_STOPPED;
		oops->wmaudio_stop();
		return 0;
	}

	return -1;
}

int
cdda_eject( struct wm_drive *d )
{
	if (d->cdda_slave > -1) {
		blk.command = WM_CDM_STOPPED;
		oops->wmaudio_stop();
		wmcdda_close(&dev);
		return 0;
	}

	return -1;
}

int
cdda_set_volume( struct wm_drive *d, int left, int right )
{
	if (d->cdda_slave > -1) {
		int bal, vol;

		bal = (right - left) + 100;
		bal *= 255;
		bal /= 200;
		if (right > left)
			vol = right;
		else
			vol = left;
		vol *= 255;
		vol /= 100;

		if(oops->wmaudio_balance)
			oops->wmaudio_balance(bal);
		if(oops->wmaudio_volume)
			oops->wmaudio_volume(vol);

		return 0;
	}

	return -1;
}

/*
 * Read the initial volume from the drive, if available.  Each channel
 * ranges from 0 to 100, with -1 indicating data not available.
 */
int
cdda_get_volume( struct wm_drive *d, int *left, int *right )
{
	if (d->cdda_slave > -1) {
		if(!oops->wmaudio_state || oops->wmaudio_state(&blk) == -1) {
			blk.volume = -1;
			blk.balance = 128;
		}

		*left = *right = (blk.volume * 100 + 254) / 255;

		if (blk.balance < 110)
			*right = (((blk.volume * blk.balance + 127) / 128) * 100 + 254) / 255;
		else if (blk.balance > 146)
			*left = (((blk.volume * (255 - blk.balance) + 127) / 128) * 100 + 254) / 255;

		return 0;
	}
 
	return -1;
}

/*
 * Turn off the CDDA slave.
 */
void
cdda_kill( struct wm_drive *d )
{
	if (d->cdda_slave > -1) {
		blk.command = WM_CDM_STOPPED;
		oops->wmaudio_stop();
		sleep(1);
		wmcdda_close(&dev);
		oops->wmaudio_close();
		
		dev.block = NULL;
		
		wait(NULL);
		d->cdda_slave = -1;
	}
}

/*
 * Tell the CDDA slave to set the play direction.
 */
void
cdda_set_direction( struct wm_drive *d, int newdir )
{
	if (d->cdda_slave > -1) {
		wmcdda_direction(newdir);
	}
}

/*
 * Tell the CDDA slave to set the play speed.
 */
void
cdda_set_speed( struct wm_drive *d, int speed )
{
	if (d->cdda_slave > -1) {
		wmcdda_speed(speed);
	}
}

/*
 * Tell the CDDA slave to set the loudness level.
 */
void
cdda_set_loudness( struct wm_drive *d, int loud )
{
	if (d->cdda_slave > -1) {
		loudness = loud;
	}
}

/*
 * Tell the CDDA slave to start (or stop) saving to a file.
 */
void
cdda_save( struct wm_drive *d, char *filename )
{
  #if 0
  int len;

  if (filename == NULL || filename[0] == '\0')
    len = 0;
  else
    len = strlen(filename);
  write(d->cdda_slave, "F", 1);
  write(d->cdda_slave, &len, sizeof(len));
  if (len)
    write(d->cdda_slave, filename, len);


    		read(0, &namelen, sizeof(namelen));
		if (output != NULL) {
			fclose(output);
			output = NULL;
		}
		if (namelen) {
			filename = malloc(namelen + 1);
			if (filename == NULL) {
				perror("cddas");
				wmcdda_close(dev);
				oops->wmaudio_close();
				exit(1);
			}

			read(0, filename, namelen);
			filename[namelen] = '\0';
			output = fopen(filename, "w");
			if (output == NULL) {
				perror(filename);
			} else {
				/* Write an .au file header. */
				hdr.magic = htonl(0x2e736e64);
				hdr.hdr_size = htonl(sizeof(hdr) + 28);
				hdr.data_size = htonl(~0);
				hdr.encoding = htonl(3); /* linear-16 */
				hdr.sample_rate = htonl(44100);
				hdr.channels = htonl(2);

				fwrite(&hdr, sizeof(hdr), 1, output);
				fwrite("Recorded from CD by WorkMan", 28, 1, output);
			}
			free(filename);

#endif
}

void* cdda_fct(void* arg)
{
	struct cdda_device *cddadev = (struct cdda_device*)arg;
	struct cdda_block *blockinfo = cddadev->block;
	int result;

	while (cddadev->block) {
		if(blockinfo->command == WM_CDM_PLAYING) {
			result = wmcdda_read(cddadev, blockinfo);
			if (result <= 0 && blockinfo->status != WM_CDM_TRACK_DONE) {
				ERRORLOG("cdda: wmcdda_read failed\n");
				blockinfo->status = WM_CDM_STOPPED;
			} else {
				result = wmcdda_normalize(cddadev, blockinfo);
				if (output)
				fwrite(cddadev->buf, result, 1, output);

				if (oops->wmaudio_play(cddadev->buf, cddadev->buflen, blockinfo)) {
					oops->wmaudio_stop();
					ERRORLOG("cdda: wmaudio_play failed\n");
					blockinfo->status = WM_CDM_STOPPED;
				}
			}
		} else {
			blockinfo->status = blockinfo->command;
			sleep(1);

		}
	}

	return 0;
}

#endif
