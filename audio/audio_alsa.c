#include "audio.h"

#ifdef USE_ALSA
/*
 *  Driver for Advanced Linux Sound Architecture, http://alsa.jcu.cz
 *
 *  mpg123 comments:
 *  Code by Anders Semb Hermansen <ahermans@vf.telia.no>
 *  Cleanups by Jaroslav Kysela <perex@jcu.cz>
 *              Ville Syrjala <syrjala@sci.fi>
 *
 *  adopted for libworkman cdda audio backend from Alexander Kern alex.kern@gmx.de  
 *
 * This file comes under GPL license.
 */

#include <stdio.h>

#include <alsa/asoundlib.h>

char* device = NULL;
snd_pcm_t *handle;

snd_pcm_format_t format = SND_PCM_FORMAT_S16;    /* sample format */
int rate = 44100;                                /* stream rate */
int channels = 2;                                /* count of channels */
int buffer_time = 2000000;                       /* ring buffer length in us */
int period_time = 100000;                        /* period time in us */

snd_pcm_sframes_t buffer_size;
snd_pcm_sframes_t period_size;

static int set_hwparams(snd_pcm_hw_params_t *params,
                        snd_pcm_access_t access)
{
       int err, dir;

        /* choose all parameters */
        err = snd_pcm_hw_params_any(handle, params);
        if (err < 0) {
                ERRORLOG("Broken configuration for playback: no configurations available: %s\n", snd_strerror(err));
                return err;
        }
        /* set the interleaved read/write format */
        err = snd_pcm_hw_params_set_access(handle, params, access);
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
        err = snd_pcm_hw_params_set_rate_near(handle, params, rate, 0);
        if (err < 0) {
                ERRORLOG("Rate %iHz not available for playback: %s\n", rate, snd_strerror(err));
                return err;
        }
        if (err != rate) {
                ERRORLOG("Rate doesn't match (requested %iHz, get %iHz)\n", rate, err);
                return -EINVAL;
        }
        /* set the buffer time */
        err = snd_pcm_hw_params_set_buffer_time_near(handle, params, buffer_time, &dir);
        if (err < 0) {
                ERRORLOG("Unable to set buffer time %i for playback: %s\n", buffer_time, snd_strerror(err));
                return err;
        }
        buffer_size = snd_pcm_hw_params_get_buffer_size(params);

        DEBUGLOG("buffersize %i\n", buffer_size);

        /* set the period time */
        err = snd_pcm_hw_params_set_period_time_near(handle, params, period_time, &dir);
        if (err < 0) {
                ERRORLOG("Unable to set period time %i for playback: %s\n", period_time, snd_strerror(err));
                return err;
        }
        period_size = snd_pcm_hw_params_get_period_size(params, &dir);
        
        DEBUGLOG("period_size %i\n", period_size);

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
  int err, chn;

  DEBUGLOG("alsa_open\n");

  snd_pcm_hw_params_t *hwparams;
  snd_pcm_sw_params_t *swparams;

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

  err = snd_pcm_close(&handle);

  free(device);

  return err;
}

/*
 * Play some audio and pass a status message upstream, if applicable.
 * Returns 0 on success.
 */
int
alsa_play(char *rawbuf, long buflen, struct cdda_block *blk)
{
  signed short *ptr;
  int err, frames;

  ptr = rawbuf;
  frames = buflen / (channels * 2);
  DEBUGLOG("play %i frames, %i bytes\n", frames, buflen);
  while (frames > 0) {
    err = snd_pcm_writei(handle, ptr, frames);
    if (err == -EAGAIN)
      continue;
    if (err < 0) break;

    ptr += err * channels;
    frames -= err;
    DEBUGLOG("played %i, rest %i\n", err / channels, frames);
  }

  if (err < 0) {
    ERRORLOG("alsa_write failed: %s\n", snd_strerror(err));
    err = snd_pcm_prepare(handle);
    if (err < 0) {
      ERRORLOG("Unable to snd_pcm_prepare pcm stream: %s\n", snd_strerror(err));
    }
    blk->status = WMCDDA_ERROR;
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

/*
 * Get the current audio state.
 */
int
alsa_state(struct cdda_block *blk)
{
  return -1; /* not implemented yet for ALSA */
}

static struct audio_oops alsa_oops = {
  .wmaudio_open    = alsa_open,
  .wmaudio_close   = alsa_close,
  .wmaudio_play    = alsa_play,
  .wmaudio_stop    = alsa_stop,
  .wmaudio_state   = alsa_state,
  .wmaudio_balance = NULL,
  .wmaudio_volume  = NULL
};

struct audio_oops*
setup_alsa(const char *dev, const char *ctl)
{
  static int init_complete = 0;

  if(dev) {
    device = strdup(dev);
  } else {
    device = strdup("plughw:0,0"); /* playback device */
  }

  if(init_complete) {
    ERRORLOG("already initialized\n");
    return NULL;
  }
  if(!alsa_open())
    init_complete = 1;
  else
    return NULL;

  return &alsa_oops;
}

#endif /* USE_ALSA */
