#include "audio.h"

#include <config.h>
#include <config-alsa.h>
#include <string.h>

struct audio_oops *setup_phonon(const char *dev, const char *ctl);
struct audio_oops *setup_arts(const char *dev, const char *ctl);
struct audio_oops *setup_alsa(const char *dev, const char *ctl);

struct audio_oops *setup_soundsystem(const char *ss, const char *dev, const char *ctl)
{
  if(!ss) {
    ERRORLOG("audio: Internal error, trying to setup a NULL soundsystem.\n");
    return NULL;
  }

  if(!strcmp(ss, "phonon"))
    return setup_phonon(dev, ctl);
#ifdef USE_ARTS
  if(!strcmp(ss, "arts"))
    return setup_arts(dev, ctl);
#endif
#if defined(HAVE_LIBASOUND2)
  if(!strcmp(ss, "alsa"))
    return setup_alsa(dev, ctl);
#endif
#if defined(sun) || defined(__sun__)
  if(!strcmp(ss, "sun"))
    return setup_sun_audio(dev, ctl);
#endif
  ERRORLOG("audio: unknown soundsystem '%s'\n", ss);
  return NULL;
}
