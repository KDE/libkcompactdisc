/*
 * This file is part of WorkMan, the civilized CD player library
 * Copyright (C) 1991-1997 by Steven Grimm (original author)
 * Copyright (C) by Dirk Försterling <milliByte@DeathsDoor.com>
 * Copyright (C) 2004-2006 Alexander Kern <alex.kern@gmx.de>
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

#include "include/wm_config.h"
#include "include/wm_struct.h"
#include "include/wm_cddb.h"
#include "include/wm_cdrom.h"
#include "include/wm_platform.h"
#include "include/wm_helpers.h"
#include "include/wm_cdtext.h"
#include "include/wm_scsi.h"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef CAN_CLOSE
#include <fcntl.h>
#endif

/* local prototypes */
static int fixup_drive_struct(struct wm_drive *);
static int read_toc(struct wm_drive *);
static const char* gen_status(int);

#define WM_MSG_CLASS WM_MSG_CLASS_CDROM

/* extern struct wm_drive generic_proto, toshiba_proto, sony_proto; */
/*	toshiba33_proto; <=== Somehow, this got lost */

/*
 * The supported drive types are listed here.  NULL means match anything.
 * The first match in the list is used, and substring matches are done (so
 * put long names before their shorter prefixes.)
 */
static struct drivelist {
  const char *vendor;
  const char *model;
  const char *revision;
  int (*fixup)(struct wm_drive *d);
} drives[] = {
{	"TOSHIBA",		"XM-3501",		NULL,		toshiba_fixup	},
{	"TOSHIBA",		"XM-3401",		NULL,		toshiba_fixup	},
{	"TOSHIBA",		"XM-3301",		NULL,		toshiba_fixup	},
{	"SONY",			"CDU-8012",		NULL,		sony_fixup	},
{	"SONY",			"CDU 561",		NULL,		sony_fixup	},
{       "SONY",         	"CDU-76S",      	NULL,   	sony_fixup	},
{	NULL,			NULL,			NULL,		NULL	}
};

/*
 * Solaris 2.2 will remove the device out from under us.  Getting an ENOENT
 * is therefore sometimes not a problem.
 */
int intermittent_dev = 0;


/*
 * Macro magic
 *
 */

#define CARRAY(id) ((id)-1)

#define DATATRACK 1

/*
 * get the current position
 */
int wm_get_cur_pos_rel(void *p)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	return pdrive->thiscd.cur_pos_rel;
}

/*
 * get the current position
 */
int wm_get_cur_pos_abs(void *p)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	return pdrive->thiscd.cur_pos_abs;
}

/*
 * init the workmanlib
 */
int wm_cd_init(const char *cd_device, const char *soundsystem,
  const char *sounddevice, const char *ctldevice, void **ppdrive)
{
	int err;
	struct wm_drive *pdrive;

	if(!ppdrive)
		return -1;

	pdrive = *ppdrive = (struct wm_drive *)malloc(sizeof(struct wm_drive));
	if(!pdrive)
		return -1;
	memset(pdrive, 0, sizeof(*pdrive));

	pdrive->cdda = (soundsystem && strcasecmp(soundsystem, "cdin"));

	pdrive->cd_device = cd_device ? strdup(cd_device) : strdup(DEFAULT_CD_DEVICE);
	pdrive->soundsystem = soundsystem ? strdup(soundsystem): NULL;
	pdrive->sounddevice = sounddevice ? strdup(sounddevice) : NULL;
	pdrive->ctldevice = ctldevice ? strdup(ctldevice) : NULL;
	if(!pdrive->cd_device) {
		err = -ENOMEM;
		goto init_failed;
	}
	pdrive->fd = -1;

	pdrive->proto.open = gen_open;
	pdrive->proto.close = gen_close;
	pdrive->proto.get_trackcount = gen_get_trackcount;
	pdrive->proto.get_cdlen = gen_get_cdlen;
	pdrive->proto.get_trackinfo = gen_get_trackinfo;
	pdrive->proto.get_drive_status = gen_get_drive_status;
	pdrive->proto.pause = gen_pause;
	pdrive->proto.resume = gen_resume;
	pdrive->proto.stop = gen_stop;
	pdrive->proto.play = gen_play;
	pdrive->proto.eject = gen_eject;
	pdrive->proto.closetray = gen_closetray;
	pdrive->proto.scsi = gen_scsi;
	pdrive->proto.set_volume = gen_set_volume;
	pdrive->proto.get_volume = gen_get_volume;
	pdrive->proto.scale_volume = gen_scale_volume;
	pdrive->proto.unscale_volume = gen_unscale_volume;
	pdrive->oldmode = WM_CDM_UNKNOWN;

	if((err = gen_init(pdrive)) < 0)
		goto init_failed;

	if ((err = pdrive->proto.open(pdrive)) < 0)
		goto open_failed;

	/* Can we figure out the drive type? */
	if (wm_scsi_get_drive_type(pdrive)) {
		wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "plat_open(): inquiry failed\n");
	}

	/* let it override some functions */
	fixup_drive_struct(pdrive);
#ifdef WMLIB_CDDA_BUILD
	if(pdrive->cdda && (err = wm_cdda_init(pdrive)))
		goto open_failed;
#endif
	return wm_cd_status(pdrive);

open_failed:
	wm_cd_destroy(pdrive);

init_failed:
	free(pdrive->cd_device);
	free(pdrive->soundsystem);
	free(pdrive->sounddevice);
	free(pdrive->ctldevice);
	free(pdrive);

	return err;
}

int wm_cd_destroy(void *p)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	free_cdtext();

	if(pdrive->cdda)
		wm_cdda_destroy(pdrive);

	pdrive->proto.close(pdrive);

	return 0;
}
/*
 * Give information about the drive we found during wmcd_open()
 */
const char *wm_drive_vendor(void *p)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
  	return pdrive->vendor?pdrive->vendor:"";
}

const char *wm_drive_model(void *p)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	return pdrive->model?pdrive->model:"";
}

const char *wm_drive_revision(void *p)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	return pdrive->revision?pdrive->revision:"";
}

const char *wm_drive_default_device()
{
	return DEFAULT_CD_DEVICE;
}

unsigned long wm_cddb_discid(void *p)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	return cddb_discid(pdrive);
}

/*
 * Figure out which prototype drive structure we should be using based
 * on the vendor, model, and revision of the current pdrive->
 */
static int fixup_drive_struct(struct wm_drive *d)
{
	struct drivelist *driver;

	for (driver = drives; driver->vendor; driver++) {
		if((strncmp(driver->vendor, d->vendor, strlen(d->vendor))) ||
			((driver->model != NULL) && strncmp(driver->model, d->model, strlen(d->model))) ||
			((d->revision != NULL) && strncmp(driver->revision, d->revision, strlen(d->revision))))
			continue;

		if(!(driver->fixup))
			goto fail;

		driver->fixup(d);
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
static int read_toc(struct wm_drive *pdrive)
{
	int    i;
	int    pos;

	if(!pdrive->proto.get_trackcount ||
		pdrive->proto.get_trackcount(pdrive, &pdrive->thiscd.ntracks) < 0) {
		return -1 ;
	}

	pdrive->thiscd.length = 0;
	pdrive->thiscd.cur_cdmode = WM_CDM_UNKNOWN;
	pdrive->thiscd.cd_cur_balance = WM_BALANCE_SYMMETRED;

	if (pdrive->thiscd.trk != NULL)
		free(pdrive->thiscd.trk);

	pdrive->thiscd.trk = malloc((pdrive->thiscd.ntracks + 1) * sizeof(struct wm_trackinfo));
	if (pdrive->thiscd.trk == NULL) {
		perror("malloc");
		return -1;
	}

	for (i = 0; i < pdrive->thiscd.ntracks; i++) {
		if(!pdrive->proto.get_trackinfo ||
			pdrive->proto.get_trackinfo(pdrive, i + 1, &pdrive->thiscd.trk[i].data,
			&pdrive->thiscd.trk[i].start) < 0) {
			return -1;
		}

		pdrive->thiscd.trk[i].length = pdrive->thiscd.trk[i].start / 75;

		pdrive->thiscd.trk[i].track = i + 1;
		wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "track %i, start frame %i\n",
			pdrive->thiscd.trk[i].track, pdrive->thiscd.trk[i].start);
	}

	if(!pdrive->proto.get_cdlen ||
		pdrive->proto.get_cdlen(pdrive, &pdrive->thiscd.trk[i].start) < 0) {
		return -1;
	}
	pdrive->thiscd.trk[i].length = pdrive->thiscd.trk[i].start / 75;

	/* Now compute actual track lengths. */
	pos = pdrive->thiscd.trk[0].length;
	for (i = 0; i < pdrive->thiscd.ntracks; i++) {
		pdrive->thiscd.trk[i].length = pdrive->thiscd.trk[i+1].length - pos;
		pos = pdrive->thiscd.trk[i+1].length;
		if (pdrive->thiscd.trk[i].data)
			pdrive->thiscd.trk[i].length = (pdrive->thiscd.trk[i + 1].start - pdrive->thiscd.trk[i].start) * 2;
	}

	pdrive->thiscd.length = pdrive->thiscd.trk[pdrive->thiscd.ntracks].length;

	wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "read_toc() successful\n");
	return 0;
} /* read_toc() */

/*
 * wm_cd_status(pdrive)
 *
 * Return values:
 *     see wm_cdrom.h
 *
 * Updates variables.
 */
int wm_cd_status(void *p)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	int mode = -1, tmp;

	if(!pdrive->proto.get_drive_status ||
		(tmp = pdrive->proto.get_drive_status(pdrive, pdrive->oldmode, &mode,
		&pdrive->thiscd.cur_frame,
		&pdrive->thiscd.curtrack,
		&pdrive->thiscd.cur_index)) < 0) {
		perror("WM get_drive_status");
		return -1;
	}

	wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS,
		"get_drive_status returns status %s, track %i, frame %i\n",
		gen_status(mode), pdrive->thiscd.curtrack, pdrive->thiscd.cur_frame);

	if(WM_CDS_NO_DISC(pdrive->oldmode) && WM_CDS_DISC_READY(mode)) {
		/* device changed */
		pdrive->thiscd.ntracks = 0;

		if(read_toc(pdrive) || 0 == pdrive->thiscd.ntracks) {

			mode = WM_CDM_NO_DISC;
		} else /* refresh cdtext info */
			get_glob_cdtext(pdrive, 1);

		wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS,
			"device status changed() from %s to %s\n",
			gen_status(pdrive->oldmode), gen_status(mode));
	}
	pdrive->oldmode = mode;

	/*
	* it seems all driver have'nt state for stop
	*/
	if(WM_CDM_PAUSED == mode && 0 == pdrive->thiscd.cur_frame) {
		mode = WM_CDM_STOPPED;
		pdrive->thiscd.curtrack = 0;
	}

	switch (mode) {
	case WM_CDM_PLAYING:
	case WM_CDM_PAUSED:
		pdrive->thiscd.cur_pos_abs = pdrive->thiscd.cur_frame / 75;
		/* search for right track */
		for(tmp = pdrive->thiscd.ntracks;
			tmp > 1 && pdrive->thiscd.cur_frame < pdrive->thiscd.trk[CARRAY(tmp)].start;
			tmp--)
			;
		pdrive->thiscd.curtrack = tmp;
		/* Fall through */


	case WM_CDM_UNKNOWN:
		if (mode == WM_CDM_UNKNOWN)
		{
			mode = WM_CDM_NO_DISC;
		}
		/* Fall through */

	case WM_CDM_STOPPED:
/*assert(thiscd.trk != NULL);*/
		if(pdrive->thiscd.curtrack >= 1 && pdrive->thiscd.curtrack <= pdrive->thiscd.ntracks) {
			pdrive->thiscd.cur_pos_rel = (pdrive->thiscd.cur_frame -
				pdrive->thiscd.trk[CARRAY(pdrive->thiscd.curtrack)].start) / 75;
			if (pdrive->thiscd.cur_pos_rel < 0)
				pdrive->thiscd.cur_pos_rel = -pdrive->thiscd.cur_pos_rel;
		}

		if (pdrive->thiscd.cur_pos_abs < 0)
			pdrive->thiscd.cur_pos_abs = pdrive->thiscd.cur_frame = 0;

		if (pdrive->thiscd.curtrack < 1)
			pdrive->thiscd.curtracklen = pdrive->thiscd.length;
		else
			pdrive->thiscd.curtracklen = pdrive->thiscd.trk[CARRAY(pdrive->thiscd.curtrack)].length;
		/* Fall through */

	case WM_CDM_TRACK_DONE:
		pdrive->thiscd.cur_cdmode = mode;
		break;
	case WM_CDM_FORWARD:
	case WM_CDM_EJECTED:
		pdrive->thiscd.cur_cdmode = mode;
		break;
	}

	wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS,
		"wm_cd_status returns %s\n", gen_status(pdrive->thiscd.cur_cdmode));

	return pdrive->thiscd.cur_cdmode;
}

int wm_cd_getcurtrack(void *p)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	if(WM_CDS_NO_DISC(pdrive->thiscd.cur_cdmode))
		return 0;
	return pdrive->thiscd.curtrack;
}

int wm_cd_getcurtracklen(void *p)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	if(WM_CDS_NO_DISC(pdrive->thiscd.cur_cdmode))
		return 0;

	return pdrive->thiscd.curtracklen;
}

int wm_cd_getcountoftracks(void *p)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	if(WM_CDS_NO_DISC(pdrive->thiscd.cur_cdmode))
		return 0;

	return pdrive->thiscd.ntracks;
}

int wm_cd_gettracklen(void *p, int track)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	if (track < 1 ||
		track > pdrive->thiscd.ntracks ||
		pdrive->thiscd.trk == NULL)
		return 0;

  	return pdrive->thiscd.trk[CARRAY(track)].length;
}

int wm_cd_gettrackstart(void *p, int track)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	if (track < 1 ||
		track > (pdrive->thiscd.ntracks+1) ||
		pdrive->thiscd.trk == NULL)
		return 0;

  return pdrive->thiscd.trk[CARRAY(track)].start;
}

int wm_cd_gettrackdata(void *p, int track)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	if (track < 1 ||
		track > pdrive->thiscd.ntracks ||
		pdrive->thiscd.trk == NULL)
		return 0;

  return pdrive->thiscd.trk[CARRAY(track)].data;
}

/*
 * wm_cd_play(starttrack, pos, endtrack)
 *
 * Start playing the CD or jump to a new position.  "pos" is in seconds,
 * relative to start of track.
 */
int wm_cd_play(void *p, int start, int pos, int end)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	int real_start, real_end, status;
	int play_start, play_end;

	status = wm_cd_status(pdrive);
	if(WM_CDS_NO_DISC(status) || pdrive->thiscd.ntracks < 1)
		return -1;

	/*
	* check ranges
	*/
	for(real_end = pdrive->thiscd.ntracks;
		(pdrive->thiscd.trk[CARRAY(real_end)].data == DATATRACK);
		real_end--)
		;
	for(real_start = 1;
		(pdrive->thiscd.trk[CARRAY(real_start)].data == DATATRACK);
		real_start++)
		;

	if(end == WM_ENDTRACK)
		end = real_end;
	else if(end > real_end)
		end = real_end;

	/*
	* handle as overrun
	*/
	if(start < real_start)
		start = real_start;
	if(start > real_end)
		start = real_end;

	/*
	* Try to avoid mixed mode and CD-EXTRA data tracks
	*/
	if(start > end || pdrive->thiscd.trk[CARRAY(start)].data == DATATRACK) {
		wm_cd_stop(pdrive);
		return -1;
	}

	play_start = pdrive->thiscd.trk[CARRAY(start)].start + pos * 75;
	play_end = (end == pdrive->thiscd.ntracks) ? pdrive->thiscd.length * 75 :
		pdrive->thiscd.trk[CARRAY(end)].start - 1;

	--play_end;

	if (play_start >= play_end)
		play_start = play_end-1;

	if(pdrive->proto.play)
		pdrive->proto.play(pdrive, play_start, play_end);
	else
		return -1;

		/* So we don't update the display with the old frame number */
	wm_cd_status(pdrive);

	return pdrive->thiscd.curtrack;
}

/*
 * wm_cd_pause()
 *
 * Pause the CD, if it's in play mode.  If it's already paused, go back to
 * play mode.
 */
int wm_cd_pause(void *p)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	static int paused_pos;
	int status;

	status = wm_cd_status(pdrive);
	if(WM_CDS_NO_DISC(status))
		return -1;

	if(WM_CDM_PLAYING == pdrive->thiscd.cur_cdmode) {
		paused_pos = pdrive->thiscd.cur_pos_rel;
		if(pdrive->proto.pause)
			return pdrive->proto.pause(pdrive);
	} else if(WM_CDM_PAUSED == status) {
		if(pdrive->proto.resume)
			return pdrive->proto.resume(pdrive);
		else if(pdrive->proto.play)
			return pdrive->proto.play(pdrive, pdrive->thiscd.cur_pos_rel, -1);
	}

	return -1;
} /* wm_cd_pause() */

/*
 * wm_cd_stop()
 *
 * Stop the CD if it's not already stopped.
 */
int wm_cd_stop(void *p)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	int status;

	status = wm_cd_status(pdrive);
	if(WM_CDS_NO_DISC(status))
		return -1;

	if (status != WM_CDM_STOPPED) {

		if(pdrive->proto.stop)
			pdrive->proto.stop(pdrive);

		status = wm_cd_status(pdrive);
	}

	return (status != WM_CDM_STOPPED);
} /* wm_cd_stop() */

/*
 * Eject the current CD, if there is one, and set the mode to 5.
 *
 * Returns 0 on success, 1 if the CD couldn't be ejected, or 2 if the
 * CD contains a mounted filesystem.
 */
int wm_cd_eject(void *p)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	int err = -1;

	if(pdrive->proto.eject)
		err = pdrive->proto.eject(pdrive);

	if (err < 0) {
		if (err == -3) {
			return 2;
		} else {
			return 1;
		}
	}

	return (WM_CDM_EJECTED == wm_cd_status(pdrive)) ? 0 : -1;
}

int wm_cd_closetray(void *p)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	int status, err = -1;

	status = wm_cd_status(pdrive);
	if (status == WM_CDM_UNKNOWN || status == WM_CDM_NO_DISC)
		return -1;

#ifdef CAN_CLOSE
	err = pdrive->proto.closetray(pdrive);

	if(err) {
		/* do close/open */
		if(!pdrive->proto.close(pdrive)) {
			wm_susleep( 1000 );
      		err = pdrive->proto.open(pdrive);
			wm_susleep( 1000 );
		}
	}

#else
	err = 0;
#endif

	return (err ? 0 : ((wm_cd_status(pdrive) == 2) ? 1 : 0));
} /* wm_cd_closetray() */

struct cdtext_info* wm_cd_get_cdtext(void *p)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	int status;

	status = wm_cd_status(pdrive);

	if(WM_CDS_NO_DISC(status))
		return NULL;

	return get_glob_cdtext(pdrive, 0);
}

int wm_cd_set_verbosity(int level)
{
	wm_lib_set_verbosity(level);
	return wm_lib_get_verbosity();
}

/*
 * volume is valid WM_VOLUME_MUTE <= vol <= WM_VOLUME_MAXIMAL,
 * balance is valid WM_BALANCE_ALL_LEFTS <= balance <= WM_BALANCE_ALL_RIGHTS
 */

int wm_cd_volume(void *p, int vol, int bal)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	int left, right;
	const int bal1 = (vol - WM_VOLUME_MUTE)/(WM_BALANCE_ALL_RIGHTS - WM_BALANCE_SYMMETRED);

/*
 * Set "left" and "right" to volume-slider values accounting for the
 * balance setting.
 *
 */
	if(vol < WM_VOLUME_MUTE)
		vol = WM_VOLUME_MUTE;
	if(vol > WM_VOLUME_MAXIMAL)
		vol = WM_VOLUME_MAXIMAL;
	if(bal < WM_BALANCE_ALL_LEFTS)
		bal = WM_BALANCE_ALL_LEFTS;
	if(bal > WM_BALANCE_ALL_RIGHTS)
		bal = WM_BALANCE_ALL_RIGHTS;

	left = vol - (bal * bal1);
	right = vol + (bal * bal1);

	wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "calculate volume left %i, right %i\n", left, right);

	if (left > WM_VOLUME_MAXIMAL)
		left = WM_VOLUME_MAXIMAL;
	if (right > WM_VOLUME_MAXIMAL)
		right = WM_VOLUME_MAXIMAL;

	if(pdrive->proto.scale_volume)
		pdrive->proto.scale_volume(&left, &right);

	if(pdrive->proto.set_volume)
		return pdrive->proto.set_volume(pdrive, left, right);

	return -1;
} /* cd_volume() */

int wm_cd_getvolume(void *p)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	int left, right;

	if(!pdrive->proto.get_volume ||
		pdrive->proto.get_volume(pdrive, &left, &right) < 0 || left == -1)
		return -1;

	if(pdrive->proto.unscale_volume)
		pdrive->proto.unscale_volume(&left, &right);

	if (left < right) {
		pdrive->thiscd.cd_cur_balance = (right - left) / 2;
		if (pdrive->thiscd.cd_cur_balance > WM_BALANCE_ALL_RIGHTS)
			pdrive->thiscd.cd_cur_balance = WM_BALANCE_ALL_RIGHTS;
		return right;
	} else if (left == right) {
		pdrive->thiscd.cd_cur_balance = WM_BALANCE_SYMMETRED;
		return left;
	} else {
		pdrive->thiscd.cd_cur_balance = (right - left) / 2;
		if (pdrive->thiscd.cd_cur_balance < WM_BALANCE_ALL_LEFTS)
			pdrive->thiscd.cd_cur_balance = WM_BALANCE_ALL_LEFTS;
		return left;
	}
}

int wm_cd_getbalance(void *p)
{
	struct wm_drive *pdrive = (struct wm_drive *)p;
	int left, right;

	if(!pdrive->proto.get_volume ||
		pdrive->proto.get_volume(pdrive, &left, &right) < 0 || left == -1)
		return WM_BALANCE_SYMMETRED;

	if(pdrive->proto.unscale_volume)
		pdrive->proto.unscale_volume(&left, &right);

	if (left < right) {
		pdrive->thiscd.cd_cur_balance = (right - left) / 2;
		if (pdrive->thiscd.cd_cur_balance > WM_BALANCE_ALL_RIGHTS)
			pdrive->thiscd.cd_cur_balance = WM_BALANCE_ALL_RIGHTS;
	} else if (left == right) {
		pdrive->thiscd.cd_cur_balance = WM_BALANCE_SYMMETRED;
	} else {
		pdrive->thiscd.cd_cur_balance = (right - left) / 2;
		if (pdrive->thiscd.cd_cur_balance < WM_BALANCE_ALL_LEFTS)
			pdrive->thiscd.cd_cur_balance = WM_BALANCE_ALL_LEFTS;
	}
	return pdrive->thiscd.cd_cur_balance;
}

static const char *gen_status(int status)
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
	case WM_CDM_LOADING:
		return "WM_CDM_LOADING";
	case WM_CDM_BUFFERING:
		return "WM_CDM_BUFFERING";
	default:
		sprintf(tmp, "unexpected status %i", status);
		return tmp;
	}
}
