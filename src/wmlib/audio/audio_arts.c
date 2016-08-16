/*  This file is part of the KDE project
    Copyright (C) 2006 Alexander Kern <alex.kern@gmx.de>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    Linux digital audio functions.
 */

#ifdef USE_ARTS

#include "audio.h"

#include <artsc.h>

arts_stream_t arts_stream = NULL;

int arts_open(void);
int arts_close(void);
int arts_stop(void);
int arts_play(struct cdda_block *blk);
int arts_state(struct cdda_block *blk);
struct audio_oops* setup_arts(const char *dev, const char *ctl);

/*
 * Initialize the audio device.
 */
int
arts_open(void)
{
  int err;

  DEBUGLOG("arts_open\n");

  if(!(arts_stream = arts_play_stream(44100, 16, 2, "cddaslave"))) {
    ERRORLOG("cannot open ARTS stream for playback\n");
    return -1;
  }
  /* 1000 ms because we read 75 frames = 1 sec */
  if((err = arts_stream_set(arts_stream, ARTS_P_BUFFER_TIME, 1000)) < 0) {
    ERRORLOG("arts_stream_set failed (%s)\n", arts_error_text(err));
    return -1;
  }
  return 0;
}

/*
 * Close the audio device.
 */
int
arts_close(void)
{
  arts_stop();

  DEBUGLOG("arts_close\n");
  arts_close_stream(arts_stream);

  arts_free();

  return 0;
}

/*
 * Play some audio and pass a status message upstream, if applicable.
 * Returns 0 on success.
 */
int
arts_play(struct cdda_block *blk)
{
  int err;

  if((err = arts_write(arts_stream, blk->buf, blk->buflen)) < 0) {
    ERRORLOG("arts_write failed (%s)\n", arts_error_text(err));
    blk->status = WM_CDM_CDDAERROR;
    return -1;
  }

  return 0;
}

/*
 * Stop the audio immediately.
 */
int
arts_stop(void)
{
  DEBUGLOG("arts_stop\n");

  return 0;
}

static struct audio_oops arts_oops = {
  .wmaudio_open    = arts_open,
  .wmaudio_close   = arts_close,
  .wmaudio_play    = arts_play,
  .wmaudio_stop    = arts_stop,
  .wmaudio_state   = NULL,
  .wmaudio_balvol  = NULL
};

struct audio_oops*
setup_arts(const char *dev, const char *ctl)
{
  int err;

  if((err = arts_init())) {
    ERRORLOG("cannot initialize ARTS audio subsystem (%s)\n", arts_error_text(err));
    return NULL;
  }

  arts_open();

  return &arts_oops;
}
#endif
