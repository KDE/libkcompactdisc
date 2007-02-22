/*
 * This file is part of WorkMan, the civilized CD player library
 * Copyright (C) Alexander Kern <alex.kern@gmx.de>
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
 */

#include <string.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include "include/wm_config.h"
#include "include/wm_struct.h"
#include "include/wm_cdda.h"
#include "include/wm_cdrom.h"
#include "include/wm_helpers.h"
#include "audio/audio.h"
#include <pthread.h>

static pthread_t thread_read;
static pthread_t thread_play;

int get_next_block(int x);
void *cdda_fct_read(void* arg);
void *cdda_fct_play(void* arg);

/* CDDABLKSIZE give us the 588 samples 4 bytes each(16 bit x 2 channel)
   by rate 44100 HZ, 588 samples are 1/75 sec
   if we read 15 frames(8820 samples), we get in each block, data for 1/5 sec */
#define NUMFRAMES 15
/* by 5 blocks in chain, the total chain contents data for 1 sec */
#define NUMBLOCKS 5

static struct cdda_block blks[NUMBLOCKS];
static pthread_mutex_t blks_mutex[NUMBLOCKS];

static struct cdda_device dev;
static pthread_cond_t wakeup_audio;

/*
 * Loudness setting, plus the floating volume multiplier and decaying-average
 * volume level.
 */
static unsigned int loudness = 0, volume = 32768, level;

/*
 * This is non-null if we're saving audio to a file.
 */
static FILE *output = NULL;

/*
 * These are driverdependent oops
 *
 */
static struct audio_oops *oops = NULL;

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
    int ret = 0;

    if (d->cdda_slave > -1) {
        cdda_stop(d);

        wm_susleep(1000);
        dev.blocks = 0;
        wm_susleep(1000);
    }

    memset(&blks, 0, sizeof(blks));

    dev.fd = -1;
    dev.frames_at_once = NUMFRAMES;
    dev.blocks = blks;
    dev.numblocks = NUMBLOCKS;
    dev.status = WM_CDM_UNKNOWN;
    dev.devname = d->cd_device;

    if ((ret = wmcdda_init(&dev)))
        return ret;

    oops = setup_soundsystem(d->soundsystem, d->sounddevice, d->ctldevice);
    if (!oops) {
        ERRORLOG("cdda: setup_soundsystem failed\n");
        wmcdda_close(&dev);
        return -1;
    }

    if(pthread_create(&thread_read, NULL, cdda_fct_read, &dev)) {
        ERRORLOG("error by create pthread");
        oops->wmaudio_close();
        wmcdda_close(&dev);
        return -1;
    }

    if(pthread_create(&thread_play, NULL, cdda_fct_play, &dev)) {
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
        if(dev.status)
          *mode = dev.status;
        else
          *mode = oldmode;

        if (*mode == WM_CDM_PLAYING) {
            *track = dev.track;
            *ind = dev.index;
            *frame = dev.frame;
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
        dev.command = WM_CDM_STOPPED;

        /* wait before reader, stops */
        while(dev.status != dev.command)
            wm_susleep(1000);

        wmcdda_setup(start, end, realstart);

        level = 2500;
        volume = 1 << 15;

        dev.track =  -1;
        dev.index =  0;
        dev.frame = start;
        dev.status = dev.command = WM_CDM_PLAYING;

        return 0;
    }

    return -1;
}

int
cdda_pause( struct wm_drive *d )
{
    if (d->cdda_slave > -1) {
        if(WM_CDM_PLAYING == dev.command) {
            dev.command = WM_CDM_PAUSED;
        } else {
            dev.command = WM_CDM_PLAYING;
        }

        return 0;
    }

    return -1;
}

int
cdda_stop( struct wm_drive *d )
{
    if (d->cdda_slave > -1) {
        dev.command = WM_CDM_STOPPED;
        oops->wmaudio_stop();
        return 0;
    }

    return -1;
}

int
cdda_eject( struct wm_drive *d )
{
    if (d->cdda_slave > -1) {
        dev.command = WM_CDM_EJECTED;
        oops->wmaudio_stop();
        /*wmcdda_close(&dev);*/
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
        if(!oops->wmaudio_state) {
            dev.volume = -1;
            dev.balance = 128;
        }

        *left = *right = (dev.volume * 100 + 254) / 255;

        if (dev.balance < 110)
            *right = (((dev.volume * dev.balance + 127) / 128) * 100 + 254) / 255;
        else if (dev.balance > 146)
            *left = (((dev.volume * (255 - dev.balance) + 127) / 128) * 100 + 254) / 255;

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
        dev.command = WM_CDM_STOPPED;
        oops->wmaudio_stop();
        wm_susleep(2000);
        wmcdda_close(&dev);
        oops->wmaudio_close();

        dev.blocks = NULL;
        wait(NULL);
        d->cdda_slave = -1;
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

int get_next_block(int x)
{
    int y = ++x;
    return (y < NUMBLOCKS)?y:0;
}

void *cdda_fct_read(void* arg)
{
    struct cdda_device *cddadev = (struct cdda_device*)arg;
    int i, j, wakeup;
    long result;

    while (cddadev->blocks) {
        while(cddadev->command != WM_CDM_PLAYING) {
            cddadev->status = cddadev->command;
            wm_susleep(1000);
        }

        i = 0;
        pthread_mutex_lock(&blks_mutex[i]);
        wakeup = 1;

        while(cddadev->command == WM_CDM_PLAYING) {
            result = wmcdda_read(cddadev, &blks[i]);
            if (result <= 0 && blks[i].status != WM_CDM_TRACK_DONE) {
                ERRORLOG("cdda: wmcdda_read failed, stop playing\n");
                cddadev->command = WM_CDM_STOPPED;
                break;
            } else {
                if (output)
                    fwrite(blks[i].buf, blks[i].buflen, 1, output);
            }

            j = get_next_block(i);
            pthread_mutex_lock(&blks_mutex[j]);

            if(wakeup) {
                wakeup = 0;
                pthread_cond_signal(&wakeup_audio);
            }

            pthread_mutex_unlock(&blks_mutex[i]);
            /* audio can start here */

            i = j;
        }

        pthread_mutex_unlock(&blks_mutex[i]);
    }

    return 0;
}

void *cdda_fct_play(void* arg)
{
    struct cdda_device *cddadev = (struct cdda_device*)arg;
    int i = 0;

    while (cddadev->blocks) {
        if(cddadev->command != WM_CDM_PLAYING) {
            i = 0;
            pthread_mutex_lock(&blks_mutex[i]);
            pthread_cond_wait(&wakeup_audio, &blks_mutex[i]);
        } else {
            i = get_next_block(i);
            pthread_mutex_lock(&blks_mutex[i]);
        }

        if (oops->wmaudio_play(&blks[i])) {
            oops->wmaudio_stop();
            ERRORLOG("cdda: wmaudio_play failed\n");
            cddadev->command = WM_CDM_STOPPED;
        }
        cddadev->frame = blks[i].frame;
        cddadev->track = blks[i].track;
        cddadev->index = blks[i].index;
        cddadev->status = blks[i].status;

        pthread_mutex_unlock(&blks_mutex[i]);
    }

    return 0;
}
