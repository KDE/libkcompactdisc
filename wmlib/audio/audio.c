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
*/

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
