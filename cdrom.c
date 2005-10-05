/*
 * $Id$
 *
 * This file is part of WorkMan, the civilized CD player library
 * (c) 1991-1997 by Steven Grimm (original author)
 * (c) by Dirk Försterling (current 'author' = maintainer)
 * The maintainer can be contacted by his e-mail address:
 * milliByte@DeathsDoor.com 
 *
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 * Interface between most of WorkMan and the low-level CD-ROM library
 * routines defined in plat_*.c and drv_*.c.  The goal is to have no
 * platform- or drive-dependent code here.
 */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
/* #include <sys/time.h> */

#include "config.h"

#include "include/wm_config.h"
#include "include/wm_struct.h"
#include "include/wm_cddb.h"
#include "include/wm_cdrom.h"
#include "include/wm_database.h"
#include "include/wm_platform.h"
#include "include/wm_helpers.h"
#include "include/wm_cdinfo.h"
#include "include/wm_cdtext.h"

#ifdef CAN_CLOSE
#include <fcntl.h>
#endif

/* local prototypes */
int read_toc(void);

#define WM_MSG_CLASS WM_MSG_CLASS_CDROM

/* extern struct wm_drive generic_proto, toshiba_proto, sony_proto; */
/*	toshiba33_proto; <=== Somehow, this got lost */

/*
 * The supported drive types are listed here.  NULL means match anything.
 * The first match in the list is used, and substring matches are done (so
 * put long names before their shorter prefixes.)
 */
struct drivelist {
  const char *ven;
  const char *mod;
  const char *rev;
  struct wm_drive_proto *proto;
} drives[] = {
{	"TOSHIBA",		"XM-3501",		NULL,		&toshiba_proto	},
{	"TOSHIBA",		"XM-3401",		NULL,		&toshiba_proto	},
{	"TOSHIBA",		"XM-3301",		NULL,		&toshiba_proto	},
{	"SONY",			"CDU-8012",		NULL,		&sony_proto	},
{	"SONY",			"CDU 561",		NULL,		&sony_proto	},
{       "SONY",         	"CDU-76S",      	NULL,   	&sony_proto	},
{	WM_STR_GENVENDOR,	WM_STR_GENMODEL,	WM_STR_GENREV,	&generic_proto  },
{	NULL,			NULL,			NULL,		&generic_proto	}
};

/*
 * Solaris 2.2 will remove the device out from under us.  Getting an ENOENT
 * is therefore sometimes not a problem.
 */
int intermittent_dev = 0;

static int wm_cd_cur_balance = 10;
static int wm_cur_cdmode = WM_CDM_UNKNOWN;
static char *wm_cd_device = NULL; /* do not use this extern */

static struct wm_drive drive = {
  0,
  NULL,
  NULL,
  NULL,
  NULL,
  -1,
  -1,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,

  NULL
};

extern struct wm_cdinfo thiscd;

/*
 * Macro magic
 *
 */
#define FREE(x); if(x) free(x); x = NULL;

#define STRDUP(old, neu); \
  if(old) free(old); old = NULL;\
  if(neu) old = strdup(neu);

#define CARRAY(id) ((id)-1)

#define DATATRACK 1
/*
 * init the workmanlib
 */
int wm_cd_init( int cdin, const char *cd_device, const char *soundsystem,
  const char *sounddevice, const char *ctldevice )
{
  drive.cdda = (WM_CDDA == cdin);
#if !defined(BUILD_CDDA)
  if(drive.cdda) {
    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "Libworkman library was compiled without cdda support\n");
    return -1;
  }
#endif
  wm_cd_destroy();

  STRDUP(wm_cd_device, cd_device);
  drive.cd_device = wm_cd_device;
  STRDUP(drive.soundsystem, soundsystem);
  STRDUP(drive.sounddevice, sounddevice);
  STRDUP(drive.ctldevice, ctldevice);

  return wm_cd_status();
}

int wm_cd_destroy( void )
{
  free_cdtext();

  if(drive.fd != -1) {
    /* first free one old */
    if(drive.proto && drive.proto->gen_close)
      drive.proto->gen_close(&drive);
    else
      close(drive.fd);
  }
  drive.fd = -1;
  FREE(wm_cd_device);
  FREE(drive.soundsystem);
  FREE(drive.sounddevice);
  FREE(drive.ctldevice);
  FREE(drive.vendor);
  FREE(drive.model);
  FREE(drive.revision);
  drive.proto = NULL;

  return 0;
}
/*
 * Give information about the drive we found during wmcd_open()
 */
const char *wm_drive_vendor( void )
{
  return drive.vendor?drive.vendor:"";
}

const char *wm_drive_model( void )
{
  return drive.model?drive.model:"";
}

const char *wm_drive_revision( void )
{
  return drive.revision?drive.revision:"";
}

const char *wm_drive_device( void )
{
  return drive.cd_device ? drive.cd_device : "";
}

/*
 * Figure out which prototype drive structure we should be using based
 * on the vendor, model, and revision of the current drive.
 */
int
find_drive_struct(const char *vendor, const char *model, const char *rev)
{
  struct drivelist *d;

  for (d = drives; d; d++) {
    if(((d->ven != NULL) && strncmp(d->ven, vendor, strlen(d->ven))) ||
       ((d->mod != NULL) && strncmp(d->mod, model, strlen(d->mod))) ||
       ((d->rev != NULL) && strncmp(d->rev, rev, strlen(d->rev))))
      continue;

    if(!(d->proto))
      goto fail;

    STRDUP(drive.vendor, vendor);
    STRDUP(drive.model, model);
    STRDUP(drive.revision, rev);

    drive.proto = d->proto;
    return 0;
  }

fail:
  return -1;
} /* find_drive_struct() */

/*
 * read_toc()
 *
 * Read the table of contents from the CD.  Return a pointer to a wm_cdinfo
 * struct containing the relevant information (minus artist/cdname/etc.)
 * This is a static struct.  Returns NULL if there was an error.
 *
 * XXX allocates one trackinfo too many.
 */
int
read_toc( void )
{
  struct wm_playlist *l;
  int    i;
  int    pos;

  if(!drive.proto)
    return -1;
    
  if(drive.proto && drive.proto->gen_get_trackcount &&
    (drive.proto->gen_get_trackcount)(&drive, &thiscd.ntracks) < 0) {
    return -1 ;
  }

  thiscd.artist[0] = thiscd.cdname[0] = '\0';
  thiscd.whichdb = thiscd.otherrc = thiscd.otherdb = thiscd.user = NULL;
  thiscd.length = 0;
  thiscd.autoplay = thiscd.playmode = thiscd.volume = 0;

  /* Free up any left-over playlists. */
  if (thiscd.lists != NULL) {
    for (l = thiscd.lists; l->name != NULL; l++) {
      free(l->name);
      free(l->list);
    }
    free(thiscd.lists);
    thiscd.lists = NULL;
  }

  if (thiscd.trk != NULL)
    free(thiscd.trk);

  thiscd.trk = malloc((thiscd.ntracks + 1) * sizeof(struct wm_trackinfo));
  if (thiscd.trk == NULL) {
    perror("malloc");
    return -1;
  }

  for (i = 0; i < thiscd.ntracks; i++) {
    if(drive.proto && drive.proto->gen_get_trackinfo &&
      (drive.proto->gen_get_trackinfo)(&drive, i + 1, &thiscd.trk[i].data,
      &thiscd.trk[i].start) < 0) {
      return -1;
    }

    thiscd.trk[i].avoid = thiscd.trk[i].data;
    thiscd.trk[i].length = thiscd.trk[i].start / 75;

    thiscd.trk[i].songname = thiscd.trk[i].otherrc =
    thiscd.trk[i].otherdb = NULL;
    thiscd.trk[i].contd = 0;
    thiscd.trk[i].volume = 0;
    thiscd.trk[i].track = i + 1;
    thiscd.trk[i].section = 0;
    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "track %i, start frame %i\n",
      thiscd.trk[i].track, thiscd.trk[i].start);
  }

  if(drive.proto && drive.proto->gen_get_cdlen &&
     (drive.proto->gen_get_cdlen)(&drive, &thiscd.trk[i].start) < 0) {
    return -1;
  }
  thiscd.trk[i].length = thiscd.trk[i].start / 75;

/* Now compute actual track lengths. */
  pos = thiscd.trk[0].length;
  for (i = 0; i < thiscd.ntracks; i++) {
    thiscd.trk[i].length = thiscd.trk[i+1].length - pos;
    pos = thiscd.trk[i+1].length;
    if (thiscd.trk[i].data)
      thiscd.trk[i].length = (thiscd.trk[i + 1].start - thiscd.trk[i].start) * 2;
      if (thiscd.trk[i].avoid)
        wm_strmcpy(&thiscd.trk[i].songname, "DATA TRACK");
  }

  thiscd.length = thiscd.trk[thiscd.ntracks].length;
  thiscd.cddbid = cddb_discid();
        
  wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "read_toc() successful\n");
  return 0;
} /* read_toc() */

/*
 * wm_cd_status()
 *
 * Return values:
 *     see wm_cdrom.h
 * 
 * Updates variables.
 */
int
wm_cd_status( void )
{
  static int oldmode = WM_CDM_UNKNOWN;
  int mode, err, tmp;

  if(!drive.proto) {
    oldmode = WM_CDM_UNKNOWN;
    err = wmcd_open( &drive );
    if (err < 0) {
      wm_cur_cdmode = WM_CDM_UNKNOWN;
      return err;
    }
  }

  if(drive.proto && drive.proto->gen_get_drive_status &&
    (drive.proto->gen_get_drive_status)(&drive, oldmode, &mode, &cur_frame,
    &(thiscd.curtrack), &cur_index) < 0) {
    perror("WM gen_get_drive_status");
    return -1;
  } else {
    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS,
      "gen_det_drive_status returns status %s, track %i, frame %i\n",
      gen_status(mode), thiscd.curtrack, cur_frame);
  }

  if(WM_CDS_NO_DISC(oldmode) && WM_CDS_DISC_READY(mode)) {
    /* device changed */
    thiscd.ntracks = 0;
    if(read_toc() || 0 == thiscd.ntracks)
    {
      close(drive.fd);
      drive.fd = -1;
      mode = WM_CDM_NO_DISC;
    }
    else /* refresh cdtext info */
      get_glob_cdtext(&drive, 1);

    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "device status changed() from %s to %s\n",
      gen_status(oldmode), gen_status(mode));
  }
  oldmode = mode;

  /*
   * it seems all driver have'nt state for stop
   */
  if(WM_CDM_PAUSED == mode && 0 == cur_frame) {
    mode = WM_CDM_STOPPED;
    thiscd.curtrack = 0;
  }

  switch (mode) {
  case WM_CDM_PLAYING:
  case WM_CDM_PAUSED:
    cur_pos_abs = cur_frame / 75;
    /* search for right track */
    for(tmp = thiscd.ntracks;
        tmp > 1 && cur_frame < thiscd.trk[CARRAY(tmp)].start;
        tmp--)
      ;
    thiscd.curtrack = tmp;
    /* Fall through */


  case WM_CDM_UNKNOWN:
    if (mode == WM_CDM_UNKNOWN)
    {
      mode = WM_CDM_NO_DISC;
      cur_lasttrack = cur_firsttrack = -1;
    }
    /* Fall through */

  case WM_CDM_STOPPED:
    if(thiscd.curtrack >= 1 && thiscd.curtrack <= thiscd.ntracks && thiscd.trk != NULL) {
      cur_trackname = thiscd.trk[CARRAY(thiscd.curtrack)].songname;
      cur_avoid = thiscd.trk[CARRAY(thiscd.curtrack)].avoid;
      cur_contd = thiscd.trk[CARRAY(thiscd.curtrack)].contd;
      cur_pos_rel = (cur_frame - thiscd.trk[CARRAY(thiscd.curtrack)].start) / 75;
      if (cur_pos_rel < 0)
        cur_pos_rel = -cur_pos_rel;
    }
    if((playlist != NULL) && playlist[0].start & (cur_listno > 0)) {
      cur_pos_abs -= thiscd.trk[playlist[CARRAY(cur_listno)].start - 1].start / 75;
      cur_pos_abs += playlist[CARRAY(cur_listno)].starttime;
    }
    if (cur_pos_abs < 0)
      cur_pos_abs = cur_frame = 0;

    if (thiscd.curtrack < 1)
      thiscd.curtracklen = thiscd.length;
    else
      thiscd.curtracklen = thiscd.trk[CARRAY(thiscd.curtrack)].length;
    /* Fall through */

  case WM_CDM_TRACK_DONE:
    wm_cur_cdmode = mode;
    break;
  case WM_CDM_FORWARD:
  case WM_CDM_EJECTED:
    wm_cur_cdmode = mode;
    break;	
  }

  wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS,
    "wm_cd_status returns %s\n", gen_status(wm_cur_cdmode));
  return wm_cur_cdmode;
}

int
wm_cd_getcurtrack( void )
{
  if(WM_CDS_NO_DISC(wm_cur_cdmode))
    return 0;
  return thiscd.curtrack;
}
  
int
wm_cd_getcurtracklen( void )
{
  if(WM_CDS_NO_DISC(wm_cur_cdmode))
    return 0;
    
  return thiscd.curtracklen;
}

int
wm_cd_getcountoftracks( void )
{
  if(WM_CDS_NO_DISC(wm_cur_cdmode))
    return 0;
    
  return thiscd.ntracks;
}

int wm_cd_gettracklen( int track )
{
  if (track < 1 ||
      track > thiscd.ntracks ||
      thiscd.trk == NULL)
    return 0;

  return thiscd.trk[CARRAY(track)].length;
}

/*
 * wm_cd_play(starttrack, pos, endtrack)
 *
 * Start playing the CD or jump to a new position.  "pos" is in seconds,
 * relative to start of track.
 */
int
wm_cd_play( int start, int pos, int end )
{
  int real_start, real_end, status;
  
  status = wm_cd_status();
  if(WM_CDS_NO_DISC(status) || thiscd.ntracks < 1)
    return -1;

  /*
   * check ranges
   */
  for(real_end = thiscd.ntracks; (thiscd.trk[CARRAY(real_end)].data == DATATRACK); real_end--)
    ;
  for(real_start = 1; (thiscd.trk[CARRAY(real_start)].data == DATATRACK); real_start++)
    ;
  
  if(end == WM_ENDTRACK) end = real_end;
  if(end > real_end) end = real_end;
  
  /*
   * handle as overrun
   */
  if(start < real_start) start = real_start;
  if(start > real_end) start = real_end;
  
  /*
   * Try to avoid mixed mode and CD-EXTRA data tracks
   */
  if(start > end || thiscd.trk[CARRAY(start)].data == DATATRACK) {
    wm_cd_stop();
    return -1;
  }
  
  cur_firsttrack = start;
  cur_lasttrack = end;
    
  wm_cd_play_chunk(thiscd.trk[CARRAY(start)].start + pos * 75, end = thiscd.ntracks ?
    thiscd.length * 75 : thiscd.trk[CARRAY(end)].start - 1, thiscd.trk[CARRAY(start)].start);
  /* So we don't update the display with the old frame number */
  wm_cd_status();

  return thiscd.curtrack;
}
  
/*
 * wm_cd_play_chunk(start, end)
 *
 * Play the CD from one position to another (both in frames.)
 */
int
wm_cd_play_chunk( int start, int end, int realstart )
{
  int status;
  
  status = wm_cd_status();
  if(WM_CDS_NO_DISC(status))
    return -1;
  
  end--;
  if (start >= end)
    start = end-1;
  
  if(!(drive.proto) || !(drive.proto->gen_play)) {
    perror("WM gen_play:  function pointer NULL");
    return -1;
  }

  return (drive.proto->gen_play)(&drive, start, end, realstart);
}
  
/*
 * Set the offset into the current track and play.  -1 means end of track
 * (i.e., go to next track.)
 */
int
wm_cd_play_from_pos( int pos )
{
  int status;
  
  status = wm_cd_status();
  if(WM_CDS_NO_DISC(status))
    return -1;
  
  if (pos == -1)
    pos = thiscd.trk[thiscd.curtrack - 1].length - 1;

  if (wm_cur_cdmode == WM_CDM_PLAYING)
    return wm_cd_play(thiscd.curtrack, pos, playlist[cur_listno-1].end);
  else
    return -1;
} /* wm_cd_play_from_pos() */

/*
 * wm_cd_pause()
 *
 * Pause the CD, if it's in play mode.  If it's already paused, go back to
 * play mode.
 */
int
wm_cd_pause( void )
{
  static int paused_pos;
  int status;
   
  status = wm_cd_status();
  if(WM_CDS_NO_DISC(status))
    return -1;

  if(WM_CDM_PLAYING == wm_cur_cdmode) {
    if(drive.proto && drive.proto->gen_pause)
      (drive.proto->gen_pause)(&drive);

    paused_pos = cur_pos_rel;
  } else if(WM_CDM_PAUSED == status) {
    if(!(drive.proto->gen_resume) || (drive.proto->gen_resume(&drive) > 0)) {
      wm_cd_play(thiscd.curtrack, paused_pos, playlist[cur_listno-1].end);
    }
  } else {
    return -1;
  }
  wm_cd_status();
  return 0;
} /* wm_cd_pause() */

/*
 * wm_cd_stop()
 *
 * Stop the CD if it's not already stopped.
 */
int
wm_cd_stop( void )
{
  int status;
   
  status = wm_cd_status();
  if(WM_CDS_NO_DISC(status))
    return -1;

  if (status != WM_CDM_STOPPED) {
    
    if(drive.proto && drive.proto->gen_stop)
      (drive.proto->gen_stop)(&drive);

    status = wm_cd_status();
  }
  
  return (status != WM_CDM_STOPPED);
} /* wm_cd_stop() */

/*
 * Eject the current CD, if there is one, and set the mode to 5.
 *
 * Returns 0 on success, 1 if the CD couldn't be ejected, or 2 if the
 * CD contains a mounted filesystem.
 */
int
wm_cd_eject( void )
{
  int err = -1;

  wm_cd_stop();
  
  if(drive.proto && drive.proto->gen_eject)
    err = (drive.proto->gen_eject)(&drive);

  if (err < 0) {
    if (err == -3) {
      return 2;
    } else {
      return 1;
    }
  }

  wm_cd_status();

  return 0;
}

int wm_cd_closetray(void)
{
  int status, err = -1;

  status = wm_cd_status();
  if (status == WM_CDM_UNKNOWN || status == WM_CDM_NO_DISC)
    return -1;

  if(drive.proto->gen_closetray)
    err = (drive.proto->gen_closetray)(&drive);

  return(err ? 0 : wm_cd_status()==2 ? 1 : 0);
} /* wm_cd_closetray() */

struct cdtext_info*
wm_cd_get_cdtext( void )
{
  int status;

  status = wm_cd_status();
  
  if(WM_CDS_NO_DISC(status))
    return NULL;
  
  return get_glob_cdtext(&drive, 0);
}

/*
 * find_trkind(track, index, start)
 *
 * Start playing at a particular track and index, optionally using a particular
 * frame as a starting position.  Returns a frame number near the start of the
 * index mark if successful, 0 if the track/index didn't exist.
 *
 * This is made significantly more tedious (though probably easier to port)
 * by the fact that CDROMPLAYTRKIND doesn't work as advertised.  The routine
 * does a binary search of the track, terminating when the interval gets to
 * around 10 frames or when the next track is encountered, at which point
 * it's a fair bet the index in question doesn't exist.
 */
int
wm_find_trkind( int track, int ind, int start )
{
  int top = 0, bottom, current, interval, ret = 0, i, status;

  status = wm_cd_status();
  if(WM_CDS_NO_DISC(status))
    return 0;

  for (i = 0; i < thiscd.ntracks; i++) {
    if (thiscd.trk[i].track == track)
      break;
  }

  bottom = thiscd.trk[i].start;

  for (; i < thiscd.ntracks; i++) {
    if (thiscd.trk[i].track > track)
      break;
  }

  top = i == thiscd.ntracks ? (thiscd.length - 1) * 75 : thiscd.trk[i].start;
  if (start > bottom && start < top)
    bottom = start;

  current = (top + bottom) / 2;
  interval = (top - bottom) / 4;

  do {
    wm_cd_play_chunk(current, current + 75, current);

    if (wm_cd_status() != 1)
      return 0;
    while (cur_frame < current) {
      if(wm_cd_status() != 1 || wm_cur_cdmode != WM_CDM_PLAYING)
        return 0;
      else
       wm_susleep(1);
    }

    if (thiscd.trk[thiscd.curtrack - 1].track > track)
      break;

    if(cur_index >= ind) {
      ret = current;
      current -= interval;
    } else {
      current += interval;
    }

    interval /= 2;

  } while (interval > 2);

  return ret;
} /* find_trkind() */

int
wm_cd_set_verbosity( int level )
{
  wm_lib_set_verbosity(level);
  return wm_lib_get_verbosity();
}

/*
 * volume is valid WM_VOLUME_MUTE <= vol <= WM_VOLUME_MAXIMAL,
 * balance is valid WM_BALANCE_ALL_LEFTS <= balance <= WM_BALANCE_ALL_RIGHTS
 */

int
wm_cd_volume( int vol, int bal )
{
  int left, right;
  const int bal1 = (vol - WM_VOLUME_MUTE)/(WM_BALANCE_ALL_RIGHTS - WM_BALANCE_SYMMETRED);

/*
 * Set "left" and "right" to volume-slider values accounting for the
 * balance setting.
 *
 */
  if(vol < WM_VOLUME_MUTE) vol = WM_VOLUME_MUTE;
  if(vol > WM_VOLUME_MAXIMAL) vol = WM_VOLUME_MAXIMAL;
  if(bal < WM_BALANCE_ALL_LEFTS) bal = WM_BALANCE_ALL_LEFTS;
  if(bal > WM_BALANCE_ALL_RIGHTS) bal = WM_BALANCE_ALL_RIGHTS;
  
  left = vol - (bal * bal1);
  right = vol + (bal * bal1);
  
  wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "calculate volume left %i, right %i\n", left, right);
  
  if (left > WM_VOLUME_MAXIMAL)
    left = WM_VOLUME_MAXIMAL;
  if (right > WM_VOLUME_MAXIMAL)
    right = WM_VOLUME_MAXIMAL;

  if(!(drive.proto) || !(drive.proto->gen_set_volume))
    return -1;
  else
    return (drive.proto->gen_set_volume)(&drive, left, right);
} /* cd_volume() */

int
wm_cd_getvolume( void )
{
  int left, right;

  if(!(drive.proto) || !(drive.proto->gen_get_volume) ||
    (drive.proto->gen_get_volume)(&drive, &left, &right) < 0 || left == -1)
    return -1;

  if (left < right) {
    wm_cd_cur_balance = (right - left) / 2;
    if (wm_cd_cur_balance > WM_BALANCE_ALL_RIGHTS)
      wm_cd_cur_balance = WM_BALANCE_ALL_RIGHTS;
    return right;
  } else if (left == right) {
    wm_cd_cur_balance = WM_BALANCE_SYMMETRED;
    return left;
  } else {
    wm_cd_cur_balance = (right - left) / 2;
    if (wm_cd_cur_balance < WM_BALANCE_ALL_LEFTS)
      wm_cd_cur_balance = WM_BALANCE_ALL_LEFTS;
    return left;
  }
}

int
wm_cd_getbalance( void )
{
  int left, right;

  if(!(drive.proto) || !(drive.proto->gen_get_volume) ||
    (drive.proto->gen_get_volume)(&drive, &left, &right) < 0 || left == -1)
    return WM_BALANCE_SYMMETRED;

  if (left < right) {
    wm_cd_cur_balance = (right - left) / 2;
    if (wm_cd_cur_balance > WM_BALANCE_ALL_RIGHTS)
      wm_cd_cur_balance = WM_BALANCE_ALL_RIGHTS;
  } else if (left == right) {
    wm_cd_cur_balance = WM_BALANCE_SYMMETRED;
  } else {
    wm_cd_cur_balance = (right - left) / 2;
    if (wm_cd_cur_balance < WM_BALANCE_ALL_LEFTS)
      wm_cd_cur_balance = WM_BALANCE_ALL_LEFTS;
  }
  return wm_cd_cur_balance;
}

/*
 * Prototype wm_drive structure, with generic functions.  The generic functions
 * will be replaced with drive-specific functions as appropriate once the drive
 * type has been sensed.
 */
struct wm_drive_proto generic_proto = {
	gen_init, /* functions... */
	gen_close,
	gen_get_trackcount,
	gen_get_cdlen,
	gen_get_trackinfo,
	gen_get_drive_status,
	gen_get_volume,
	gen_set_volume,
	gen_pause,
	gen_resume,
	gen_stop,
	gen_play,
	gen_eject,
	gen_closetray,
	gen_get_cdtext
};

const char*
gen_status(int status)
{
  static char tmp[250];
  switch(status) {
  case WM_CDM_TRACK_DONE:
    return "WM_CDM_TRACK_DONE";
  case WM_CDM_PLAYING:
    return "WM_CDM_PLAYING";
  case WM_CDM_FORWARD:
    return "WM_CDM_FORWARD";
  case WM_CDM_PAUSED:
    return "WM_CDM_PAUSED";
  case WM_CDM_STOPPED:
    return "WM_CDM_STOPPED";
  case WM_CDM_EJECTED:
    return "WM_CDM_EJECTED";
  case WM_CDM_DEVICECHANGED:
    return "WM_CDM_DEVICECHANGED";
  case WM_CDM_NO_DISC:
    return "WM_CDM_NO_DISC";
  case WM_CDM_UNKNOWN:
    return "WM_CDM_UNKNOWN";
  case WM_CDM_CDDAERROR:
    return "WM_CDM_CDDAERROR";
  case WM_CDM_CDDAACK:
    return "WM_CDM_CDDAACK";
  default:
    {
      sprintf(tmp, "unexpected status %i", status);
      return tmp;
    }
  }
}
