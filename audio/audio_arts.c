/*
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
 * Linux digital audio functions.
 *
 *
 * Forget /dev/audio
 * most modern soundcards accept 16LE with 44.1kHz
 * Alexander Kern alex.kern@gmx.de
 */
#include "audio.h"

#ifdef include_ARTS_TRUE

#include <artsc.h>

arts_stream_t arts_stream = NULL;

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
arts_play(char *rawbuf, long buflen, struct cdda_block *blk)
{
  int err;

  if((err = arts_write(arts_stream, rawbuf, buflen)) < 0) {
    ERRORLOG("arts_write failed (%s)\n", arts_error_text(err));
    blk->status = WMCDDA_ERROR;
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

/*
 * Get the current audio state.
 */
int
arts_state(struct cdda_block *blk)
{
  return -1; /* not implemented yet for ARTS */
}

static struct audio_oops arts_oops = {
  .wmaudio_open    = arts_open,
  .wmaudio_close   = arts_close,
  .wmaudio_play    = arts_play,
  .wmaudio_stop    = arts_stop,
  .wmaudio_state   = arts_state,
  .wmaudio_balance = NULL,
  .wmaudio_volume  = NULL
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

#endif /* USE_ARTS */
