#include "audio.h"

#include <string.h>

struct audio_oops* setup_soundsystem(const char* ss, const char* dev, const char* ctl)
{
  if(!strcmp(ss, "arts"))
    return setup_arts(dev, ctl);
#if defined(HAVE_ARTS_LIBASOUND2)
  if(!strcmp(ss, "alsa"))
    return setup_alsa(dev, ctl);
#endif
#ifdef USE_SUN_AUDIO
  if(!strcmp(ss, "sun"))
    return setup_sun_audio(dev, ctl);
#endif
  ERRORLOG("audio: unknown soundsystem '%s'\n", ss);
  return NULL;
}
