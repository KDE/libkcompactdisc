/*  This file is part of the KDE project
    Copyright (C) Alexander Kern <alex.kern@gmx.de>

    Adapted to support both ALSA V0.x and V1.x APIs for PCM calls
     (Philip Nelson <teamdba@scotdb.com> 2004-03-15)

    based on mpg123 , Anders Semb Hermansen <ahermans@vf.telia.no>,
    Jaroslav Kysela <perex@jcu.cz>, Ville Syrjala <syrjala@sci.fi>

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
*/

#include "audio.h"
#include "../include/wm_struct.h"
#include "../include/wm_config.h"

#include <config-alsa.h>

#ifdef HAVE_ALSA

#define _BSD_SOURCE /* strdup */
#define _DEFAULT_SOURCE /* stop glibc whining about the previous line */

#include <alsa/asoundlib.h>

static char *device = NULL;
static snd_pcm_t *handle;

static snd_pcm_format_t format = SND_PCM_FORMAT_S16;    /* sample format */

#if (SND_LIB_MAJOR < 1)
int rate = 44100;                                /* stream rate */
int new_rate;
int channels = 2;                                /* count of channels */
int buffer_time = 2000000;                       /* ring buffer length in us */
int period_time = 100000;                        /* period time in us */
#else
unsigned int rate = 44100;                                /* stream rate */
unsigned int new_rate;
int channels = 2;                                /* count of channels */
unsigned int buffer_time = 2000000;                       /* ring buffer length in us */
unsigned int period_time = 100000;                        /* period time in us */
#endif

#if (SND_LIB_MAJOR < 1)
snd_pcm_sframes_t buffer_size;
snd_pcm_sframes_t period_size;
#else
snd_pcm_uframes_t buffer_size;
snd_pcm_uframes_t period_size;
#endif


int alsa_open(void);
int alsa_close(void);
int alsa_stop(void);
int alsa_play(struct wm_cdda_block *blk);
int alsa_state(struct wm_cdda_block *blk);
struct audio_oops* setup_alsa(const char *dev, const char *ctl);

static int set_hwparams(snd_pcm_hw_params_t *params,
                        snd_pcm_access_t accesspar)
{
       int err, dir;

        /* choose all parameters */
        err = snd_pcm_hw_params_any(handle, params);
        if (err < 0) {
                ERRORLOG("Broken configuration for playback: no configurations available: %s\n", snd_strerror(err));
                return err;
        }
        /* set the interleaved read/write format */
        err = snd_pcm_hw_params_set_access(handle, params, accesspar);
        if (err < 0) {
                ERRORLOG("Access type not available for playback: %s\n", snd_strerror(err));
                return err;
        }
        /* set the sample format */
        err = snd_pcm_hw_params_set_format(handle, params, format);
        if (err < 0) {
                ERRORLOG("Sample format not available for playback: %s\n", snd_strerror(err));
                return err;
        }
        /* set the count of channels */
        err = snd_pcm_hw_params_set_channels(handle, params, channels);
        if (err < 0) {
                ERRORLOG("Channels count (%i) not available for playbacks: %s\n", channels, snd_strerror(err));
                return err;
        }
        /* set the stream rate */
#if (SND_LIB_MAJOR < 1)
        err = new_rate = snd_pcm_hw_params_set_rate_near(handle, params, rate, 0);
#else
        new_rate = rate;
        err = snd_pcm_hw_params_set_rate_near(handle, params, &rate, 0);
#endif
        if (err < 0) {
                ERRORLOG("Rate %iHz not available for playback: %s\n", rate, snd_strerror(err));
                return err;
        }
        if (new_rate != rate) {
                ERRORLOG("Rate does not match (requested %iHz, get %iHz)\n", rate, new_rate);
                return -EINVAL;
        }
        /* set the buffer time */
#if (SND_LIB_MAJOR < 1)
         err = snd_pcm_hw_params_set_buffer_time_near(handle, params, buffer_time, &dir);
#else
        err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &buffer_time, &dir);
#endif
        if (err < 0) {
                ERRORLOG("Unable to set buffer time %i for playback: %s\n", buffer_time, snd_strerror(err));
                return err;
        }
#if (SND_LIB_MAJOR < 1)
         buffer_size = snd_pcm_hw_params_get_buffer_size(params);
#else
        err = snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
        if (err < 0) {
                ERRORLOG("Unable to get buffer size : %s\n", snd_strerror(err));
                return err;
        }
#endif
        DEBUGLOG("buffersize %lu\n", buffer_size);

        /* set the period time */
#if (SND_LIB_MAJOR < 1)
         err = snd_pcm_hw_params_set_period_time_near(handle, params, period_time, &dir);
#else
        err = snd_pcm_hw_params_set_period_time_near(handle, params, &period_time, &dir);
#endif
        if (err < 0) {
                ERRORLOG("Unable to set period time %i for playback: %s\n", period_time, snd_strerror(err));
                return err;
        }

#if (SND_LIB_MAJOR < 1)
        period_size = snd_pcm_hw_params_get_period_size(params, &dir);
#else
        err = snd_pcm_hw_params_get_period_size(params, &period_size, &dir);
        if (err < 0) {
                ERRORLOG("Unable to get hw period size: %s\n", snd_strerror(err));
        }
#endif

        DEBUGLOG("period_size %lu\n", period_size);

        /* write the parameters to device */
        err = snd_pcm_hw_params(handle, params);
        if (err < 0) {
                ERRORLOG("Unable to set hw params for playback: %s\n", snd_strerror(err));
                return err;
        }
        return 0;
}

static int set_swparams(snd_pcm_sw_params_t *swparams)
{
        int err;

        /* get the current swparams */
        err = snd_pcm_sw_params_current(handle, swparams);
        if (err < 0) {
                ERRORLOG("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
                return err;
        }
        /* start the transfer when the buffer is full */
        err = snd_pcm_sw_params_set_start_threshold(handle, swparams, buffer_size);
        if (err < 0) {
                ERRORLOG("Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
                return err;
        }
        /* allow the transfer when at least period_size samples can be processed */
        err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_size);
        if (err < 0) {
                ERRORLOG("Unable to set avail min for playback: %s\n", snd_strerror(err));
                return err;
        }
        /* align all transfers to 1 sample */
        err = snd_pcm_sw_params_set_xfer_align(handle, swparams, 1);
        if (err < 0) {
                ERRORLOG("Unable to set transfer align for playback: %s\n", snd_strerror(err));
                return err;
        }
        /* write the parameters to the playback device */
        err = snd_pcm_sw_params(handle, swparams);
        if (err < 0) {
                ERRORLOG("Unable to set sw params for playback: %s\n", snd_strerror(err));
                return err;
        }
        return 0;
}

int alsa_open( void )
{
  int err;

  snd_pcm_hw_params_t *hwparams;
  snd_pcm_sw_params_t *swparams;

  DEBUGLOG("alsa_open\n");

  snd_pcm_hw_params_alloca(&hwparams);
  snd_pcm_sw_params_alloca(&swparams);

  if((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0/*SND_PCM_NONBLOCK*/)) < 0 ) {
    ERRORLOG("open failed: %s\n", snd_strerror(err));
    return -1;
  }

  if((err = set_hwparams(hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    ERRORLOG("Setting of hwparams failed: %s\n", snd_strerror(err));
    return -1;
  }
  if((err = set_swparams(swparams)) < 0) {
    ERRORLOG("Setting of swparams failed: %s\n", snd_strerror(err));
    return -1;
  }

  return 0;
}

int alsa_close( void )
{
  int err;

  DEBUGLOG("alsa_close\n");

  err = alsa_stop();

#if (SND_LIB_MAJOR < 1)
   err = snd_pcm_close(handle);
#else
   err = snd_pcm_close(handle);
#endif

  free(device);

  return err;
}

/*
 * Play some audio and pass a status message upstream, if applicable.
 * Returns 0 on success.
 */
int
alsa_play(struct wm_cdda_block *blk)
{
  signed short *ptr;
  int err = 0, frames;

  ptr = (signed short *)blk->buf;
  frames = blk->buflen / (channels * 2);
  DEBUGLOG("play %i frames, %lu bytes\n", frames, blk->buflen);
  while (frames > 0) {
    err = snd_pcm_writei(handle, ptr, frames);

    if (err == -EAGAIN)
      continue;
    if(err == -EPIPE) {
      err = snd_pcm_prepare(handle);
      continue;
    } else if (err < 0)
      break;

    ptr += err * channels;
    frames -= err;
    DEBUGLOG("played %i, rest %i\n", err, frames);
  }

  if (err < 0) {
    ERRORLOG("alsa_write failed: %s\n", snd_strerror(err));
    err = snd_pcm_prepare(handle);

    if (err < 0) {
      ERRORLOG("Unable to snd_pcm_prepare pcm stream: %s\n", snd_strerror(err));
    }
    blk->status = WM_CDM_CDDAERROR;
    return err;
  }

  return 0;
}

/*
 * Stop the audio immediately.
 */
int
alsa_stop( void )
{
  int err;

  DEBUGLOG("alsa_stop\n");

  err = snd_pcm_drop(handle);
  if (err < 0) {
    ERRORLOG("Unable to drop pcm stream: %s\n", snd_strerror(err));
  }

  err = snd_pcm_prepare(handle);
  if (err < 0) {
    ERRORLOG("Unable to snd_pcm_prepare pcm stream: %s\n", snd_strerror(err));
  }

  return err;
}

static struct audio_oops alsa_oops = {
  .wmaudio_open    = alsa_open,
  .wmaudio_close   = alsa_close,
  .wmaudio_play    = alsa_play,
  .wmaudio_stop    = alsa_stop,
  .wmaudio_state   = NULL,
  .wmaudio_balvol  = NULL
};

struct audio_oops*
setup_alsa(const char *dev, const char *ctl)
{
  static int init_complete = 0;

  DEBUGLOG("setup_alsa\n");

  if(init_complete) {
    alsa_close();
    init_complete = 0;
  }

  if(dev && strlen(dev) > 0) {
    device = strdup(dev);
  } else {
    device = strdup("plughw:0,0"); /* playback device */
  }

  if(!alsa_open())
    init_complete = 1;
  else
    return NULL;

  return &alsa_oops;
}

#endif /* HAVE_ALSA */
