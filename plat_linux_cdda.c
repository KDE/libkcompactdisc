/*
 * $Id$
 *
 * This file is part of WorkMan, the civilized CD player library
 * (c) 1991-1997 by Steven Grimm (original author)
 * (c) by Dirk Försterling (current 'author' = maintainer)
 * The maintainer can be contacted by his e-mail address:
 * milliByte@DeathsDoor.com 
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * Linux CDDA functions. Derived from the SUN module.
 */

#include "include/wm_cdda.h"
 
#if defined(__linux__) && defined(BUILD_CDDA)

#include "include/wm_struct.h"
#include "include/wm_cdda.h"
#ifndef __GNUC__
#define __GNUC__ 1
#endif

/* don't undef ansi for the other includes */
#ifdef __STICT_ANSI__
#undef __STRICT_ANSI__
#include <asm/types.h>
#define __STRICT_ANSI__
#else
#include <asm/types.h>
#endif

/* ugly workaround for broken glibc shipped in SuSE 9.0 */
#define inline __inline__
#define asm __asm__
#include <linux/cdrom.h>
#undef inline
#undef asm
/* types.h and cdio.h are included by wm_cdda.h */

#include <stdio.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define WM_MSG_CLASS WM_MSG_CLASS_PLATFORM

#define CDDABLKSIZE 2352

/* Address of next block to read. */
static int current_position;

/* Address of first and last blocks to read. */
static int starting_position;
static int ending_position;

static struct cdrom_read_audio cdda;
static long wmcdda_normalize(struct cdda_block *block);


/*
 * Initialize the CDDA data buffer and open the appropriate device.
 *
 */
int
wmcdda_init(struct cdda_device* pdev)
{
    int i;

    if (pdev->fd > -1)
        return -1;

    for (i = 0; i < pdev->numblocks; i++) {
        /* in Linux const */
        pdev->blocks[i].buflen = pdev->frames_at_once * CDDABLKSIZE;
        pdev->blocks[i].buf = malloc(pdev->blocks[i].buflen);
        if (!pdev->blocks[i].buf) {
            ERRORLOG("wmcdda_init ENOMEM\n");
            return -ENOMEM;
        }
    }
      
    pdev->fd = open(pdev->devname?pdev->devname:"/dev/cdrom", O_RDONLY | O_NONBLOCK);

    if (pdev->fd > -1) {
        cdda.addr_format = CDROM_LBA;
        cdda.addr.lba = 200;
        cdda.nframes = 1;
        cdda.buf = (unsigned char*)pdev->blocks[0].buf;

        pdev->status = WM_CDM_STOPPED;
        if((ioctl(pdev->fd, CDROMREADAUDIO, &cdda) < 0)) {
            if (errno == ENXIO) {
                /* CD ejected! */
                pdev->status = WM_CDM_EJECTED;
                return 0;
            } else {
                /* Sometimes it fails once, dunno why */
                pdev->status = WM_CDM_CDDAERROR;
                return 0;
            }
        }
    } else {
        ERRORLOG("canot open device, errno %i\n", errno);
        pdev->status = WM_CDM_UNKNOWN;
        return -1;
    }

    pdev->status = WM_CDM_UNKNOWN;
    return 0;
}

/*
 * Close the CD-ROM device in preparation for exiting.
 */
int
wmcdda_close(struct cdda_device* pdev)
{
    int i;

    if(-1 == pdev->fd)
        return -1;

    close(pdev->fd);
    pdev->fd = -1;

    for (i = 0; i < pdev->numblocks; i++) {
        free(pdev->blocks[i].buf);
        pdev->blocks[i].buf = 0;
        pdev->blocks[i].buflen = 0;
    }

    return 0;
}

/*
 * Set up for playing the CD.  Actually this doesn't play a thing, just sets a
 * couple variables so we'll know what to do when we're called.
 */
int
wmcdda_setup(int start, int end, int realstart)
{
    current_position = start;
    ending_position = end;
    starting_position = realstart;

    return 0;
}

/*
 * Read some blocks from the CD.  Stop if we hit the end of the current region.
 *
 * Returns number of bytes read, -1 on error, 0 if stopped for a benign reason.
 */
long
wmcdda_read(struct cdda_device* pdev, struct cdda_block *block)
{
    if (pdev->fd < 0 && wmcdda_init(pdev)) {
        return -1;
    }

    /* Hit the end of the CD, probably. */
    if (current_position >= ending_position) {
        block->status = WM_CDM_TRACK_DONE;
        return 0;
    }

    cdda.addr_format = CDROM_LBA;
    cdda.addr.lba = current_position;
    if (ending_position && current_position + pdev->frames_at_once > ending_position)
        cdda.nframes = ending_position - current_position;
    else
        cdda.nframes = pdev->frames_at_once;
   
    cdda.buf = (unsigned char*)block->buf;

    if (ioctl(pdev->fd, CDROMREADAUDIO, &cdda) < 0) {
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
    block->frame  = current_position;
    block->status = WM_CDM_PLAYING;
    block->buflen = cdda.nframes * CDDABLKSIZE;

    current_position = current_position + cdda.nframes;
    
    return wmcdda_normalize(block);
}

/*
 * Normalize a bunch of CDDA data.  Basically this means doing byte-swapping, since the CD audio is in
 * littleendian format.
 */
long
wmcdda_normalize(struct cdda_block *block)
{
#if WM_BIG_ENDIAN
    int i;
    int blocks = block->buflen / CDDABLKSIZE;
    char *rawbuf = block->buf;
    char *dest = block->buf;

    while (blocks--) {
        for (i = 0; i < CDDABLKSIZE / 2; i++) {
            *dest++ = rawbuf[1];
            *dest++ = rawbuf[0];
            rawbuf += 2;
        }
    }
#endif
    return block->buflen;
}

/*
 * Set the playback direction.
 */
void
wmcdda_direction(int newdir)
{
}

/*
 * Do system-specific stuff to get ready to play at a particular speed.
 */
void
wmcdda_speed(int speed)
{
}

#endif
