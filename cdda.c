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

#if defined(BUILD_CDDA)
struct cdda_block blk;

int
cdda_get_drive_status( struct wm_drive *d, int oldmode,
  int *mode, int *frame, int *track, int *ind )
{
  if (d->cdda_slave > -1) {
    unsigned char cmd[1] = { 'I' };

    write(d->cdda_slave, cmd, 1);
    cdda_get_ack(d->cdda_slave);

    *mode = blk.status & ~WM_CDM_CDDAACK;

    if (*mode == WMCDDA_PLAYING) {
      *track = blk.track;
      *ind = blk.index;
      *frame = blk.minute * 60 * 75 +
        blk.second * 75 +
        blk.frame;
    } else if (*mode == WMCDDA_ERROR) {
      /*
       * An error near the end of the CD probably
       * just means we hit the end.
       */
      *mode = WM_CDM_TRACK_DONE;
    }
#ifdef DEBUG
    fprintf(stderr, "cdda_get_drive_status oldmode %s mode %s: frame %i\n",
      gen_status(oldmode), gen_status(*mode), *frame);
#endif

    return 0;
  } else
    return -1;
}

/*
 * Wait for an acknowledgement from the CDDA slave.
 */
int
cdda_get_ack(int fd)
{
  do {
    if (read(fd, &blk, sizeof(blk)) <= 0) {
      printf("cdda_get_ack: pipe broken \n");
      return 0;
    }
  } while (!(blk.status & WMCDDA_ACK));
  return 1;
}

int
cdda_play( struct wm_drive *d, int start, int end, int realstart )
{
  unsigned char cmdbuf[10];

  if (d->cdda_slave > -1) {
    cmdbuf[0] = 'P';
    cmdbuf[1] = start / (60 * 75);
    cmdbuf[2] = (start % (60*75)) / 75;
    cmdbuf[3] = start % 75;
    cmdbuf[4] = end / (60*75);
    cmdbuf[5] = (end % (60*75)) / 75;
    cmdbuf[6] = end % 75;
    cmdbuf[7] = realstart / (60 * 75);
    cmdbuf[8] = (realstart % (60*75)) / 75;
    cmdbuf[9] = realstart % 75;

    /* Write the play command and make sure the slave has it. */
    write(d->cdda_slave, cmdbuf, 10);
    cdda_get_ack(d->cdda_slave);

    return 0;
  } else
    return -1;
}

int
cdda_pause( struct wm_drive *d )
{
  if (d->cdda_slave > -1) {
    int dummy, mode = WM_CDM_PLAYING;
    unsigned char cmd[1] = { 'A' };

    write(d->cdda_slave, cmd, 1);
    cdda_get_ack(d->cdda_slave);
    /*
    while (mode != WM_CDM_PAUSED)
      gen_get_drive_status(d, WM_CDM_PAUSED, &mode, &dummy, &dummy,
        &dummy);
    */
    return 0;
  } else
    return -1;
}

int
cdda_stop( struct wm_drive *d )
{
  unsigned char cmd[1] = { 'S' };
  if (d->cdda_slave > -1) {
    write(d->cdda_slave, cmd, 1);
    cdda_get_ack(d->cdda_slave);

    /*
     * The WMCDDA_STOPPED status message will be caught by
     * gen_get_drive_status.
     */

    return 0;
  } else
    return -1;
}

int
cdda_eject( struct wm_drive *d )
{
  unsigned char cmd[1] = { 'E' };
  if (d->cdda_slave > -1) {
    write(d->cdda_slave, cmd, 1);
    cdda_get_ack(d->cdda_slave);

    /*
     * The WMCDDA_EJECTED status message will be caught by
     * gen_get_drive_status.
     */

    return 0;
  } else
    return -1;
}

int
cdda_set_volume( struct wm_drive *d, int left, int right )
{
  if (d->cdda_slave > -1) {
    int bal, vol;
    unsigned char cmd[2];

    bal = (right - left) + 100;
    bal *= 255;
    bal /= 200;
    if (right > left)
      vol = right;
    else
      vol = left;
    vol *= 255;
    vol /= 100;

    cmd[0] = 'B';
    cmd[1] = bal;
    write(d->cdda_slave, cmd, 2);
    cmd[0] = 'V';
    cmd[1] = vol;
    write(d->cdda_slave, cmd, 2);
    /*
     * Don't wait for the ack, or the user won't be able to drag
     * the volume slider smoothly.
     */

    return 0;
  } else
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
    write(d->cdda_slave, "G", 1);
    cdda_get_ack(d->cdda_slave);
    read(d->cdda_slave, &blk, sizeof(blk));

    *left = *right = (blk.volume * 100 + 254) / 255;

    if (blk.balance < 110)
      *right = (((blk.volume * blk.balance + 127) / 128) *
        100 + 254) / 255;
    else if (blk.balance > 146)
      *left = (((blk.volume * (255 - blk.balance) +
        127) / 128) * 100 + 254) / 255;

    return (0);
  } else
    return -1;
}

/*
 * Turn off the CDDA slave.
 */
void
cdda_kill( struct wm_drive *d )
{
  if (d->cdda_slave > -1) {
    write(d->cdda_slave, "Q", 1);
    cdda_get_ack(d->cdda_slave);
    wait(NULL);
    d->cdda_slave = -1;
    /* codec_start(); */
  }
}

/*
 * Tell the CDDA slave to set the play direction.
 */
void
cdda_set_direction( struct wm_drive *d, int newdir )
{
  unsigned char	buf[2];
  
  if (d->cdda_slave > -1)
    {
      buf[0] = 'd';
      buf[1] = newdir;
      write(d->cdda_slave, buf, 2);
      cdda_get_ack(d->cdda_slave);
    }
} /* gen_set_direction() */

/*
 * Tell the CDDA slave to set the play speed.
 */
void
cdda_set_speed( struct wm_drive *d, int speed )
{
  unsigned char	buf[2];
  
  if (d->cdda_slave > -1)
    {
      buf[0] = 's';
      buf[1] = speed;
      write(d->cdda_slave, buf, 2);
      cdda_get_ack(d->cdda_slave);
    }
} /* gen_set_speed() */

/*
 * Tell the CDDA slave to set the loudness level.
 */
void
cdda_set_loudness( struct wm_drive *d, int loud )
{
  unsigned char	buf[2];
  
  if (d->cdda_slave > -1)
    {
      buf[0] = 'L';
      buf[1] = loud;
      write(d->cdda_slave, buf, 2);
      cdda_get_ack(d->cdda_slave);
    }
} /* gen_set_loudness() */

/*
 * Tell the CDDA slave to start (or stop) saving to a file.
 */
void
cdda_save( struct wm_drive *d, char *filename )
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
  cdda_get_ack(d->cdda_slave);
}

#endif
