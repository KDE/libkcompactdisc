#ifndef WM_STRUCT_H
#define WM_STRUCT_H
/*
 * This file is part of WorkMan, the civilized CD player library
 * Copyright (C) 1991-1997 by Steven Grimm <koreth@midwinter.com>
 * Copyright (C) by Dirk FÃ¶rsterling <milliByte@DeathsDoor.com>
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
 */

#include "wm_platform.h"

#define WM_STR_GENVENDOR "Generic"
#define WM_STR_GENMODEL  "drive"
#define WM_STR_GENREV    "type"

struct wm_drive;

/*
 * Structure for a single track.  This is pretty much self-explanatory --
 * one of these exists for each track on the current CD.
 */
struct wm_trackinfo
{
	int	length;		/* Length of track in seconds or Kbytes */
	int	start;		/* Starting position (f+s*75+m*60*75) */
	int	track;		/* Physical track number */
	int	data;		/* Flag: data track */
};

struct wm_cdinfo
{
	int	ntracks;	/* Number of tracks on the disc */
	int curtrack;
	int curtracklen;
	int cur_cdmode;
	int cur_index;	 /* Current index mark */
	int cur_pos_rel; /* Current track-relative position in seconds */
	int cur_pos_abs; /* Current absolute position in seconds */
	int cur_frame;   /* Current frame number */
	int	length;		/* Total running time in seconds */
	int cd_cur_balance;
	struct wm_trackinfo *trk;	/* struct wm_trackinfo[ntracks] */
};

/*
 * Each platform has to define generic functions, so may as well declare
 * them all here to save space.
 * These functions should never be seen outside libworkman. So I don't care
 * about the wm_ naming convention here.
 */
struct wm_drive_proto
{
	int (*open)(struct wm_drive *d);
	int (*close)(struct wm_drive *d);
	int (*get_trackcount)(struct wm_drive *d, int *tracks);
	int (*get_cdlen)(struct wm_drive *d, int *frames);
	int (*get_trackinfo)(struct wm_drive *d, int track, int *data, int *startframe);
	int (*get_drive_status)(struct wm_drive *d, int oldmode, int *mode, int *pos, int *track, int *ind);
	int (*pause)(struct wm_drive *d);
	int (*resume)(struct wm_drive *d);
	int (*stop)(struct wm_drive *d);
	int (*play)(struct wm_drive *d, int start, int end);
	int (*eject)(struct wm_drive *d);
	int (*closetray)(struct wm_drive *d);
	int (*scsi)(struct wm_drive *d, unsigned char *cdb, int cdb_len, void *ret_buf, int ret_buflen, int get_reply);
	int (*set_volume)(struct wm_drive *d, int left, int right);
	int (*get_volume)(struct wm_drive *d, int *left, int *right);
	int (*scale_volume)(int *left, int *right);
	int (*unscale_volume)(int *left, int *right);
};

/* forward declaration */
int gen_init(struct wm_drive *d);
int gen_open(struct wm_drive *d);
int gen_close(struct wm_drive *d);
int gen_get_trackcount(struct wm_drive *d, int *tracks);
int gen_get_cdlen(struct wm_drive *d, int *frames);
int gen_get_trackinfo(struct wm_drive *d, int track, int *data, int *startframe);
int gen_get_drive_status(struct wm_drive *d, int oldmode, int *mode, int *pos, int *track, int *ind);
int gen_pause(struct wm_drive *d);
int gen_resume(struct wm_drive *d);
int gen_stop(struct wm_drive *d);
int gen_play(struct wm_drive *d, int start, int end);
int gen_eject(struct wm_drive *d);
int gen_closetray(struct wm_drive *d);
int gen_scsi(struct wm_drive *d, unsigned char *cdb, int cdb_len, void *ret_buf, int ret_buflen, int get_reply);
int gen_set_volume(struct wm_drive *d, int left, int right);
int gen_get_volume(struct wm_drive *d, int *left, int *right);
int gen_scale_volume(int *left, int *right);
int gen_unscale_volume(int *left, int *right);

/*
 * Information about a particular block of CDDA data.
 */
struct wm_cdda_block
{
    unsigned char status;
    unsigned char track;
    unsigned char index;
    unsigned char reserved;

    int   frame;
    char *buf;
    long  buflen;
};

#ifdef WMLIB_CDDA_BUILD
int gen_cdda_init(struct wm_drive *d);
int gen_cdda_open(struct wm_drive* d);
int gen_cdda_read(struct wm_drive* d, struct wm_cdda_block *block);
int gen_cdda_close(struct wm_drive* d);
#else
	#define gen_cdda_init(x) (-1)
	#define gen_cdda_open(x) (-1)
	#define gen_cdda_read(x, y) (-1)
	#define gen_cdda_close(x) (-1)
#endif


/*
 * Drive descriptor structure.  Used for access to low-level routines.
 */
struct wm_drive
{
	int  cdda;            /* cdda 1, cdin 0 */

	/* commpn section */
	char *cd_device;
	char *soundsystem;
	char *sounddevice;
	char *ctldevice;

	char  vendor[9];      /* Vendor name */
	char  model[17];      /* Drive model */
	char  revision[5];    /* Revision of the drive */
	void  *aux;           /* Pointer to optional platform-specific info */

	struct wm_cdinfo thiscd;
    int cur_cdmode;

	/* cdin section */
	int    fd;            /* file descriptor */
	void  *daux;          /* Pointer to optional drive-specific info etc. */
	struct wm_drive_proto proto;

	/* cdda section */
    unsigned char status;
    unsigned char track;
    unsigned char index;
    unsigned char command;
	
	int current_position;
	int ending_position;

    int frame;
    int frames_at_once;

    struct wm_cdda_block *blocks;
    int numblocks;
  	void  *cddax;         /* Pointer to optional drive-specific info  etc. */
};

int toshiba_fixup(struct wm_drive *d);
int sony_fixup(struct wm_drive *d);

struct cdtext_info* get_glob_cdtext(struct wm_drive*, int);
void free_cdtext(void);

int wm_cdda_init(struct wm_drive *d);
int wm_cdda_destroy(struct wm_drive *d);

#endif /* WM_STRUCT_H */
