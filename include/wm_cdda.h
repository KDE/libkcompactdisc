#ifndef WM_CDDA_H
#define WM_CDDA_H
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * Information about a particular block of CDDA data.
 */
struct cdda_block {
	unsigned char	status;		/* see below */
	unsigned char	track;
	unsigned char	index;
	unsigned char	minute;
	unsigned char	second;
	unsigned char	frame;

	/* Average volume levels, for level meters */
	unsigned char	lev_chan0;
	unsigned char	lev_chan1;

	/* Current volume setting (0-255) */
	unsigned char	volume;

	/* Current balance setting (0-255, 128 = balanced) */
	unsigned char	balance;
};

struct cdda_device {
  int        fd;
  const char *devname;
  char       *buf;
  long       buflen;
};

#include "wm_cdrom.h"
#include "wm_config.h"
#include "wm_struct.h"
/*
 * cdda_block status codes.
 */

#define WMCDDA_DONE     WM_CDM_TRACK_DONE /* Chunk of data is done playing */
#define WMCDDA_PLAYING  WM_CDM_PLAYING /* Just played the block in question */
#define WMCDDA_PAUSED   WM_CDM_PAUSED
#define WMCDDA_STOPPED  WM_CDM_STOPPED /* Block data invalid; we've just stopped */
#define WMCDDA_EJECTED  WM_CDM_EJECTED /* Disc ejected or offline */

#define WMCDDA_ACK      WM_CDM_CDDAACK /* Acknowledgement of command from parent */
#define WMCDDA_ERROR    WM_CDM_CDDAERROR /* Couldn't read CDDA from disc */

/*
 * Enable or disable CDDA building depending on platform capabilities, and
 * determine endianness based on architecture.  (Gross!)
 *
 * For header-comfort, the macros LITTLE_ENDIAN and BIG_ENDIAN had to be
 * renamed. At least Linux does have bytesex.h and endian.h for easy
 * byte-order examination.
 */

#ifdef HAVE_MACHINE_ENDIAN_H
	#include <machine/endian.h>
	#if BYTE_ORDER == LITTLE_ENDIAN
		#define WM_LITTLE_ENDIAN 1
		#define WM_BIG_ENDIAN 0
	#else
		#define WM_LITTLE_ENDIAN 0
		#define WM_BIG_ENDIAN 1
	#endif
#elif defined(__sun) || defined(sun) 
# ifdef SYSV
#  include <sys/types.h>
#  include <sys/cdio.h>
#  ifndef CDROMCDDA
#   undef BUILD_CDDA
#  endif
#  ifdef i386
#   define WM_LITTLE_ENDIAN 1
#   define WM_BIG_ENDIAN 0
#  else
#   define WM_BIG_ENDIAN 1
#   define WM_LITTLE_ENDIAN 0
#  endif
# else
#  undef BUILD_CDDA
# endif

/* Linux only allows definition of endianness, because there's no
 * standard interface for CDROM CDDA functions that aren't available
 * if there is no support.
 */
#elif defined(__linux__)
/*# include <bytesex.h>*/
# include <endian.h>
/*
 * XXX could this be a problem? The results are only 0 and 1 because
 * of the ! operator. How about other linux compilers than gcc ?
 */
# define WM_LITTLE_ENDIAN !(__BYTE_ORDER - __LITTLE_ENDIAN)
# define WM_BIG_ENDIAN !(__BYTE_ORDER - __BIG_ENDIAN)
#elif defined WORDS_BIGENDIAN
	#define WM_LITTLE_ENDIAN 0
	#define WM_BIG_ENDIAN 1
#else
	#define WM_LITTLE_ENDIAN 1
	#define WM_BIG_ENDIAN 0
#endif

/*
 * The following code shouldn't take effect now. 
 * In 1998, the WorkMan platforms don't support __PDP_ENDIAN
 * architectures.
 *
 */

#if !defined(WM_LITTLE_ENDIAN)
#  if !defined(WM_BIG_ENDIAN)
#    error yet unsupported architecture
	foo bar this is to stop the compiler. 
#  endif
#endif

#if defined(BUILD_CDDA)
/*
 * The following code support us by optimize cdda operations
 */
#define CDDARETURN(x) if(x && x->cdda == 1) return
#define IFCDDA(x) if(x && x->cdda == 1)
int  cdda_get_drive_status( struct wm_drive *d, int oldmode,
  int *mode, int *pos, int *track, int *ind );
int  cdda_play( struct wm_drive *d, int start, int end, int realstart );
int  cdda_pause( struct wm_drive *d );
int  cdda_stop( struct wm_drive *d );
int  cdda_eject( struct wm_drive *d );
int  cdda_set_volume( struct wm_drive *d, int left, int right );
int  cdda_get_volume( struct wm_drive *d, int *left, int *right );
void cdda_kill( struct wm_drive *d );
void cdda_save( struct wm_drive *d, char *filename );
int  cdda_get_ack( int );
int  gen_cdda_init( struct wm_drive *d );

void cdda_set_direction( struct wm_drive *d, int newdir );
void cdda_set_speed( struct wm_drive *d, int speed );
void cdda_set_loudness( struct wm_drive *d, int loud );


int  wmcdda_init(struct cdda_device*, struct cdda_block *block);
int  wmcdda_open(const char*);
int  wmcdda_close(struct cdda_device*);
int  wmcdda_setup(int start, int end, int realstart);
long wmcdda_read(struct cdda_device*, struct cdda_block *block);
void wmcdda_speed(int speed);
void wmcdda_direction(int newdir);
long wmcdda_normalize(struct cdda_device*, struct cdda_block *block);

#else
 #define CDDARETURN(x)
 #define IFCDDA(x)
 #define cdda_get_drive_status
 #define cdda_play
 #define cdda_pause
 #define cdda_resume
 #define cdda_stop
 #define cdda_eject
 #define cdda_set_volume
 #define cdda_get_volume
 #define cdda_kill
 #define cdda_save
 #define cdda_get_ack
#endif /* defined(BUILD_CDDA) */

#include <stdio.h>

#ifdef DEBUG
 #define DEBUGLOG(fmt, args...) fprintf(stderr, fmt, ##args)
#else
 #define DEBUGLOG(fmt, args...)
#endif
#define ERRORLOG(fmt, args...) fprintf(stderr, fmt, ##args)

#endif /* WM_CDDA_H */
