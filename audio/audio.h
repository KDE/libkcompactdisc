/*
 * Audio 'LIB' defines
 */
#include "../include/wm_cdda.h"

#ifndef NULL
#define NULL 0
#endif

struct audio_oops {
  int (*wmaudio_open)(void);
  int (*wmaudio_close)(void);
  int (*wmaudio_play)(struct cdda_block*);
  int (*wmaudio_stop)(void);
  int (*wmaudio_state)(struct cdda_block*);
  int (*wmaudio_balance)(int);
  int (*wmaudio_volume)(int);
};

#ifdef __cplusplus
    extern "C" {
#endif

struct audio_oops *setup_soundsystem(const char *, const char *, const char *);

#ifdef __cplusplus
    }
#endif


