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
#include <arpa/inet.h> /* For htonl(3) */
#include <stdio.h>
#include <unistd.h>
#include "include/wm_config.h"
#include "include/wm_struct.h"
#include "include/wm_cdda.h"
#include "include/wm_cdrom.h"
#include "include/wm_helpers.h"
#include "include/wm_scsi.h"
#include "audio/audio.h"

#include <pthread.h>

static pthread_t thread_read;
static pthread_t thread_play;

/* CDDABLKSIZE give us the 588 samples 4 bytes each(16 bit x 2 channel)
   by rate 44100 HZ, 588 samples are 1/75 sec
   if we read 15 frames(8820 samples), we get in each block, data for 1/5 sec */
#define COUNT_CDDA_FRAMES_PER_BLOCK 15

/* Only Linux and Sun define the number of blocks explicitly; assume all
   other systems are like Linux and have 10 blocks.
*/
#ifndef COUNT_CDDA_BLOCKS
#define COUNT_CDDA_BLOCKS 10
#endif

static struct wm_cdda_block blks[COUNT_CDDA_BLOCKS];
static pthread_mutex_t blks_mutex[COUNT_CDDA_BLOCKS];
static pthread_cond_t wakeup_audio;

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

static int cdda_status(struct wm_drive *d, int oldmode,
  int *mode, int *frame, int *track, int *ind)
{
    if (d->cddax) {
        if(d->status)
          *mode = d->status;
        else
          *mode = oldmode;

        if (*mode == WM_CDM_PLAYING) {
            *track = d->track;
            *ind = d->index;
            *frame = d->frame;
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

static int cdda_play(struct wm_drive *d, int start, int end)
{
    if (d->cddax) {
        d->command = WM_CDM_STOPPED;
        oops->wmaudio_stop();

        /* wait before reader, stops */
        while(d->status != d->command)
            wm_susleep(1000);

		d->current_position = start;
		d->ending_position = end;

        d->track =  -1;
        d->index =  0;
        d->frame = start;
        d->status = d->command = WM_CDM_PLAYING;

        return 0;
    }

    return -1;
}

static int cdda_pause(struct wm_drive *d)
{
    if (d->cddax) {
        if(WM_CDM_PLAYING == d->command) {
            d->command = WM_CDM_PAUSED;
            if(oops->wmaudio_pause)
                oops->wmaudio_pause();
        } else {
            d->command = WM_CDM_PLAYING;
        }

        return 0;
    }

    return -1;
}

static int cdda_stop(struct wm_drive *d)
{
    if (d->cddax) {
        d->command = WM_CDM_STOPPED;
        oops->wmaudio_stop();
        return 0;
    }

    return -1;
}

static int cdda_set_volume(struct wm_drive *d, int left, int right)
{
    if (d->cddax) {
         if(oops->wmaudio_balvol && !oops->wmaudio_balvol(1, &left, &right))
            return 0;
    }

    return -1;
}

static int cdda_get_volume(struct wm_drive *d, int *left, int *right)
{
    if (d->cddax) {
        if(oops->wmaudio_balvol && !oops->wmaudio_balvol(0, left, right))
            return 0;
    }

    return -1;
}

  #if 0
/*
 * Tell the CDDA slave to start (or stop) saving to a file.
 */
void
cdda_save(struct wm_drive *, char *)
{

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
				wmcdda_close(cdda_device);
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

}
#endif

static int get_next_block(int x)
{
    int y = ++x;
    return (y < COUNT_CDDA_BLOCKS)?y:0;
}

static void *cdda_fct_read(void* arg)
{
    struct wm_drive *d = (struct wm_drive *)arg;
    int i, j, wakeup;
    long result;

    while (d->blocks) {
        while(d->command != WM_CDM_PLAYING) {
            d->status = d->command;
            wm_susleep(1000);
        }

        i = 0;
        (void) pthread_mutex_lock(&blks_mutex[i]);
        wakeup = 1;

        while(d->command == WM_CDM_PLAYING) {
            result = gen_cdda_read(d, &blks[i]);
            if (result <= 0 && blks[i].status != WM_CDM_TRACK_DONE) {
                ERRORLOG("cdda: wmcdda_read failed, stop playing\n");
                d->command = WM_CDM_STOPPED;
                break;
            } else {
                if (output)
                    fwrite(blks[i].buf, blks[i].buflen, 1, output);
            }

            j = get_next_block(i);
            (void) pthread_mutex_lock(&blks_mutex[j]);

            if(wakeup) {
                wakeup = 0;
                pthread_cond_signal(&wakeup_audio);
            }

            (void) pthread_mutex_unlock(&blks_mutex[i]);
            /* audio can start here */

            i = j;
        }

        (void) pthread_mutex_unlock(&blks_mutex[i]);
    }

    return 0;
}

static void *cdda_fct_play(void* arg)
{
    struct wm_drive *d = (struct wm_drive *)arg;
    int i = 0;

    while (d->blocks) {
        if(d->command != WM_CDM_PLAYING) {
            i = 0;
            (void) pthread_mutex_lock(&blks_mutex[i]);
            pthread_cond_wait(&wakeup_audio, &blks_mutex[i]);
        } else {
            i = get_next_block(i);
            (void) pthread_mutex_lock(&blks_mutex[i]);
        }

        if (oops->wmaudio_play(&blks[i])) {
            oops->wmaudio_stop();
            ERRORLOG("cdda: wmaudio_play failed\n");
            d->command = WM_CDM_STOPPED;
        }
        if (oops->wmaudio_state)
            oops->wmaudio_state(&blks[i]);

        d->frame = blks[i].frame;
        d->track = blks[i].track;
        d->index = blks[i].index;
        if ((d->status = blks[i].status) == WM_CDM_TRACK_DONE)
            d->command = WM_CDM_STOPPED;

        (void) pthread_mutex_unlock(&blks_mutex[i]);
    }

    return 0;
}

/*
 * Try to initialize the CDDA slave.  Returns 0 on success.
 */
int wm_cdda_init(struct wm_drive *d)
{
	int ret = 0;

	if (d->cddax) {
		wm_cdda_destroy(d);

		wm_susleep(1000);
		d->blocks = 0;
		wm_susleep(1000);
	}

	memset(blks, 0, sizeof(blks));

	d->blocks = blks;
	d->frames_at_once = COUNT_CDDA_FRAMES_PER_BLOCK;
	d->numblocks = COUNT_CDDA_BLOCKS;
	d->status = WM_CDM_UNKNOWN;

	if ((ret = gen_cdda_init(d)))
		return ret;

	if ((ret = gen_cdda_open(d)))
		return ret;

	wm_scsi_set_speed(d, 4);

	oops = setup_soundsystem(d->soundsystem, d->sounddevice, d->ctldevice);
	if (!oops) {
		ERRORLOG("cdda: setup_soundsystem failed\n");
		gen_cdda_close(d);
		return -1;
	}

	if(pthread_create(&thread_read, NULL, cdda_fct_read, d)) {
		ERRORLOG("error by create pthread");
		oops->wmaudio_close();
		gen_cdda_close(d);
		return -1;
	}

	if(pthread_create(&thread_play, NULL, cdda_fct_play, d)) {
		ERRORLOG("error by create pthread");
		oops->wmaudio_close();
		gen_cdda_close(d);
		return -1;
	}

	d->proto.get_drive_status = cdda_status;
	d->proto.pause = cdda_pause;
	d->proto.resume = NULL;
	d->proto.stop = cdda_stop;
	d->proto.play = cdda_play;
	d->proto.set_volume = cdda_set_volume;
	d->proto.get_volume = cdda_get_volume;
	d->proto.scale_volume = NULL;
	d->proto.unscale_volume = NULL;

	d->cddax = (void *)1;

	return 0;
}

int wm_cdda_destroy(struct wm_drive *d)
{
    if (d->cddax) {
		wm_scsi_set_speed(d, -1);

		d->command = WM_CDM_STOPPED;
		oops->wmaudio_stop();
		wm_susleep(2000);
		gen_cdda_close(d);
		oops->wmaudio_close();

		d->numblocks = 0;
        d->blocks = NULL;
        wait(NULL);
        d->cddax = NULL;
    }
    return 0;
}
