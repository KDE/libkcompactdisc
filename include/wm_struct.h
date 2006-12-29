#ifndef WM_STRUCT_H
#define WM_STRUCT_H
/*
 * $Id: wm_struct.h 486075 2005-12-06 18:29:02Z thiago $
 *
 * This file is part of WorkMan, the civilized CD player library
 * Copyright (C) 1991-1997 by Steven Grimm (original author)
 * Copyright (C) by Dirk Försterling (current 'author' = maintainer)
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
 */



/*
 * Structure for a single track.  This is pretty much self-explanatory --
 * one of these exists for each track on the current CD.
 */
struct wm_trackinfo
{
  char	*songname;	/* Name of song, dynamically allocated */
  char	*otherdb;	/* Unrecognized info for this track */
  char	*otherrc;
  int	length;		/* Length of track in seconds or Kbytes */
  int	start;		/* Starting position (f+s*75+m*60*75) */
  int	volume;		/* Per-track volume (1-32, 0 to disable) */
  int	track;		/* Physical track number */
  int	section;	/* Section number (0 if track not split) */
  int	contd;		/* Flag: continuation of previous track */
  int	avoid;		/* Flag: don't play this track. */
  int	data;		/* Flag: data track */
};

/*
 * Structure for internal playlist management.  The internal playlist is
 * simply the list of track ranges that are being played currently.  This
 * is built whenever the CD starts playing; it's used in normal and shuffle
 * modes as well as playlist mode.
 *
 * The "starttime" element represents how much time has elapsed by the time
 * we get to this entry.  For instance, if the list begins with a 5-minute
 * track and a 3-minute track, the third entry would have a starttime of 8
 * minutes.  This is used so that the elapsed play time can be displayed
 * even in shuffle or playlist modes.
 *
 * The last member of the list has a start track of 0, and its starttime is
 * the total playing time of the playlist (which will usually be overestimated,
 * since we don't play leadouts in some cases.)
 */
struct wm_play
{
  int	start;		/* Start track, or 0 if end of list */
  int	end;		/* last track plus 1 */
  int	starttime;	/* Number of seconds elapsed previously */
};

/*
 * Structure for playlists (as seen by the user.)  This is simply a name
 * followed by a zero-terminated list of track numbers to play.  The list
 * is terminated by a NULL name.
 */
struct wm_playlist
{
  char	*name;		/* Name of this playlist */
  int	*list;		/* List of tracks */
};

struct wm_cdinfo
{
  char	artist[84];	/* Artist's name */
  char	cdname[84];	/* Disc's name */
  int	ntracks;	/* Number of tracks on the disc */
  int  curtrack;
  int  curtracklen;
  int	length;		/* Total running time in seconds */
  int	autoplay;	/* Start playing CD immediately */
  int	playmode;	/* How to play the CD */
  int	volume;		/* Default volume (1-32, 0 for none) */
  struct wm_trackinfo *trk;	/* struct wm_trackinfo[ntracks] */
  struct wm_playlist *lists;	/* User-specified playlists */
  char	*whichdb;	/* Which database is this entry from? */
  char	*otherdb;	/* Unrecognized lines from this entry */
  char	*otherrc;
  char	*user;		/* Name of originating user */
  unsigned int cddbid;  /* CDDB-ID of the current disc */
  struct cdinfo *next;	/* For browsers, etc. */
};

/* The global variable "cd" points to the struct for the CD that's playing. */
extern struct wm_cdinfo *cd;

extern struct wm_playlist *new_playlist();

#define WM_STR_GENVENDOR "Generic"
#define WM_STR_GENMODEL  "drive"
#define WM_STR_GENREV    "type"


/*
 * Drive descriptor structure.  Used for access to low-level routines.
 */
struct wm_drive_proto
{
  int (*gen_init)();
  int (*gen_close)();
  int (*gen_get_trackcount)();
  int (*gen_get_cdlen)();
  int (*gen_get_trackinfo)();
  int (*gen_get_drive_status)();
  int (*gen_get_volume)();
  int (*gen_set_volume)();
  int (*gen_pause)();
  int (*gen_resume)();
  int (*gen_stop)();
  int (*gen_play)();
  int (*gen_eject)();
  int (*gen_closetray)();
  int (*gen_get_cdtext)();
};

struct wm_drive
{
  int  cdda;         /* cdda 1, cdin 0 */
  const char *cd_device;
  char *soundsystem;
  char *sounddevice;
  char *ctldevice;
  int	fd;		/* File descriptor, if used by platform */
  int  cdda_slave;   /* File descriptor for CDDA */

  char *vendor;      /* Vendor name */
  char *model;       /* Drive model */
  char *revision;    /* Revision of the drive */
  void	*aux;		/* Pointer to optional platform-specific info */
  void	*daux;		/* Pointer to optional drive-specific info */

  struct wm_drive_proto *proto;
};

/*
 * Structure for information of the usage of cddb.
 */
struct wm_cddb {
        int     protocol;               /* 0-off, 1-cddbp, 2-http, 3-htproxy */
        char    cddb_server[84];        /* host.domain.name:port */
        char    mail_adress[84];        /* user@domain.name */
        char    path_to_cgi[84];        /* (/)path/to/cddb.cgi */
        char    proxy_server[84];       /* host.domain.name:port */
};
extern struct wm_cddb cddb;


/*
 * Each platform has to define generic functions, so may as well declare
 * them all here to save space.
 * These functions should never be seen outside libworkman. So I don't care
 * about the wm_ naming convention here.
 */
int gen_init( struct wm_drive *d );
int gen_close( struct wm_drive *d );
int gen_get_trackcount(struct wm_drive *d, int *tracks);
int gen_get_cdlen(struct wm_drive *d, int *frames);
int gen_get_trackinfo(struct wm_drive *d, int track, int *data, int *startframe);
int gen_get_drive_status( struct wm_drive *d, int oldmode,
  int *mode, int *pos, int *track, int *ind );
int gen_set_volume( struct wm_drive *d, int left, int right );
int gen_get_volume( struct wm_drive *d, int *left, int *right );
int gen_pause(struct wm_drive *d);
int gen_resume(struct wm_drive *d);
int gen_stop(struct wm_drive *d);
int gen_play(struct wm_drive *d, int start, int end, int realstart);
int gen_eject(struct wm_drive *d);
int gen_closetray(struct wm_drive *d);
int gen_get_cdtext(struct wm_drive *d, unsigned char **pp_buffer, int *p_buffer_lenght);

int find_drive_struct(const char *vendor, const char *model, const char *rev);


struct cdtext_info* get_glob_cdtext(struct wm_drive*, int);
void free_cdtext(void);
const char* gen_status(int);

#endif /* WM_STRUCT_H */
