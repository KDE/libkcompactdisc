#include "audio.h"

#include <string.h>

struct audio_oops* setup_soundsystem(const char* ss, const char* dev, const char* ctl)
{
#ifdef USE_ALSA
  if(!strcmp(ss, "alsa"))
    return setup_alsa(dev, ctl);
#endif
#ifdef USE_ARTS
  if(!strcmp(ss, "arts"))
    return setup_arts(dev, ctl);
#endif
  return NULL;
}
