/*
 * $id$
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
 ******************************************************************
 *
 * Digital audio manipulator for WorkMan.
 *
 * The CDDA architecture looks like this:
 *
 *                     WorkMan  (or another UI!)
 *                       ^^^
 *                       |||    (separate processes connected by pipe)
 *                       vvv
 *     +------------- cddaslave -------------+
 *     |                  |                  |
 * command module    CDDA reader       audio output
 * (portable)        (per platform)    (per platform)
 *
 * This source file has the command module and some of the scaffolding
 * to hold cddaslave together, plus some non-system-dependent audio
 * processing code.  Look in plat_*_cdda.c for system-specific stuff.
 *
 */

#include "include/wm_cdda.h"

#ifdef BUILD_CDDA
 
static char cddaslave_id[] = "$Id$";

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "include/wm_struct.h"
#include "include/wm_cdda.h"
#include "include/wm_platform.h"
#include "audio/audio.h"

void send_status(struct cdda_block *);

#define SEND_ACK(b); { (b)->status |= WMCDDA_ACK; send_status(b); }
#define SEND_STATUS(b); { (b)->status &= ~WMCDDA_ACK; send_status(b); }
#define SEND_STATUS_ACK(b, s); { (b)->status = ((s) | WMCDDA_ACK); send_status(b); }

int recieve_command(struct cdda_device *, struct cdda_block *);

int	playing = 0;		/* Should the CD be playing now? */
int pausing = 0;

/*
 * Loudness setting, plus the floating volume multiplier and decaying-average
 * volume level.
 */
int		loudness = 0;
unsigned int	volume = 32768;
unsigned int	level;

/*
 * Playback speed (0 = slow)
 */
int		speed = 128;

/*
 * This is non-null if we're saving audio to a file.
 */
FILE		*output = NULL;

/*
 * These are driverdependent oops
 *
 */
struct audio_oops *oops = NULL;

/*
 * Audio file header format.
 */
typedef unsigned long	u_32;
struct auheader {
	u_32	magic;
	u_32	hdr_size;
	u_32	data_size;
	u_32	encoding;
	u_32	sample_rate;
	u_32	channels;
};

/* had to change #ifdef to #if   -> see wm_cdda.h */
#if WM_BIG_ENDIAN
# ifndef htonl
#  define htonl(x) (x)
# endif
#else
extern unsigned long htonl(x);
#endif

void *malloc();
long cdda_transform();

/*
 * Send status information upstream.
 */
void
send_status(struct cdda_block *blk)
{
  DEBUGLOG("send_status, send %i(%s | %s)\n", blk->status,
    gen_status(blk->status & WMCDDA_ACK), gen_status(blk->status & ~WMCDDA_ACK));
	write(1, blk, sizeof(*blk));
}

/*
 * Accept a command from our master.
 *
 * The protocol is byte-oriented:
 *   PmsfMSFxyz		Play from msf to MSF (MSF can be 0,0,0 to play to end)
 *			xyz is the msf of the start of this chunk, i.e., the
 *			ending boundary for reverse play.
 *   S                  Stop.
 *   E                  Eject.  This means we just close the CD device and
 *                      open it again later.
 *   Q			Quit.
 *   Vn 		Set volume level (0-255).
 *   Bn			Set balance level (0-255).
 *   EnL		Set an equalizer level (n = 0 for bass, 255 for treble)
 *   G			Get current status.
 *   sn                 Set speed multiplier to n.
 *   dn			Set direction to forward (n = 0) or reverse.
 *   Fllllx...          Start saving to a file (length = l, followed by name)
 *   F0000              Stop saving.
 *   Ln			Set loudness level (0-255).
 *   A                  Pause/Resume
 *   I                  Get status, current frame
 */
int
recieve_command(struct cdda_device *dev, struct cdda_block* blk)
{
	unsigned char		inbuf[10];
	char			*filename;
	int			namelen;
	struct auheader		hdr;

  if (read(0, inbuf, 1) <= 0) {
    wmcdda_close(dev);
    oops->wmaudio_close();
/*    ERRORLOG("cddaslave: parent died, exit\n");*/
		exit(0);
	}

  DEBUGLOG("cddaslave: CMD %c\n", inbuf[0]);

	switch (inbuf[0]) {
  case 'I':
    if(dev->fd < 0) wmcdda_init(dev, blk);
    SEND_ACK(blk);
    break;
  case 'A': /* pause/resume */
    if(WMCDDA_PLAYING & blk->status) {
      oops->wmaudio_stop();
      SEND_STATUS_ACK(blk, WMCDDA_PAUSED);
    } else if (WMCDDA_PAUSED & blk->status) {
      SEND_STATUS_ACK(blk, WMCDDA_PLAYING);
    } else {
      SEND_ACK(blk);
    }
    break;
	case 'E':
    oops->wmaudio_stop();
    wmcdda_close(dev);
    SEND_ACK(blk);
		break;
	case 'P':
		read(0, inbuf, 9);

    oops->wmaudio_stop();

		wmcdda_setup(inbuf[0] * 60 * 75 + inbuf[1] * 75 + inbuf[2],
			inbuf[3] * 60 * 75 + inbuf[4] * 75 + inbuf[5],
			inbuf[6] * 60 * 75 + inbuf[7] * 75 + inbuf[8]);

		level = 2500;
		volume = 1 << 15;

    blk->track =  -1;
    blk->index =  0;
    blk->minute = inbuf[6];
    blk->second = inbuf[7];
    blk->frame  = inbuf[8];
    SEND_STATUS_ACK(blk, WMCDDA_PLAYING);
		break;

	case 'S':
    oops->wmaudio_stop();
    SEND_STATUS_ACK(blk, WMCDDA_STOPPED);
		break;

	case 'B':
		read(0, inbuf, 1);
    if(oops->wmaudio_balance)
      oops->wmaudio_balance(inbuf[0]);
		break;

	case 'V':
		read(0, inbuf, 1);
    if(oops->wmaudio_balance)
      oops->wmaudio_volume(inbuf[0]);
		break;
  case 'G':
    {
      SEND_ACK(blk);
      if(!oops->wmaudio_state || oops->wmaudio_state(blk) == -1) {
        blk->volume = -1;
        blk->balance = 128;
      }
      send_status(blk);
    }
    break;
  case 'Q':
    SEND_ACK(blk);
    wmcdda_close(dev);
    oops->wmaudio_close();
		exit(0);
/*
	case 's':
		read(0, inbuf, 1);
		speed = inbuf[0];
		wmcdda_speed(speed);
    SEND_STATUS(blk, WMCDDA_ACK);
		break;

	case 'd':
		read(0, inbuf, 1);
		wmcdda_direction(inbuf[0]);
    SEND_STATUS(blk, WMCDDA_ACK);
		break;
*/
	case 'L':
		read(0, inbuf, 1);
		loudness = inbuf[0];
    SEND_ACK(blk);
		break;

	case 'F':
		read(0, &namelen, sizeof(namelen));
		if (output != NULL)
		{
			fclose(output);
			output = NULL;
		}
		if (namelen)
		{
			filename = malloc(namelen + 1);
			if (filename == NULL)
			{
				perror("cddaslave");
        wmcdda_close(dev);
        oops->wmaudio_close();
				exit(1);
			}

			read(0, filename, namelen);
			filename[namelen] = '\0';
			output = fopen(filename, "w");
			if (output == NULL)
				perror(filename);
			else
			{
				/* Write an .au file header. */
				hdr.magic = htonl(0x2e736e64);
				hdr.hdr_size = htonl(sizeof(hdr) + 28);
				hdr.data_size = htonl(~0);
				hdr.encoding = htonl(3);	/* linear-16 */
				hdr.sample_rate = htonl(44100);
				hdr.channels = htonl(2);

				fwrite(&hdr, sizeof(hdr), 1, output);
				fwrite("Recorded from CD by WorkMan", 28, 1,
					output);
			}

			free(filename);
		}
    SEND_ACK(blk);
	}

  return(dev->fd);
}


/*
 * Transform some CDDA data.
 */
long
wmcdda_transform(unsigned char *rawbuf, long buflen, struct cdda_block *block)
{
	long		i;
	long		*buf32 = (long *)rawbuf;
	register short	*buf16 = (short *)rawbuf;
	register int	aval;

	/*
	 * Loudness transformation.  Basically this is a self-adjusting
	 * volume control; our goal is to keep the average output level
	 * around a certain value (2500 seems to be pleasing.)  We do this
	 * by maintaining a decaying average of the recent output levels
	 * (where "recent" is some fraction of a second.)  All output levels
	 * are multiplied by the inverse of the decaying average; this has
	 * the volume-leveling effect we desire, and isn't too CPU-intensive.
	 *
	 * This is done by modifying the digital data, rather than adjusting
	 * the system volume control, because (at least on some systems)
	 * tweaking the system volume can generate little pops and clicks.
	 *
	 * There's probably a more elegant way to achieve this effect, but
	 * what the heck, I never took a DSP class and am making this up as
	 * I go along, with a little help from some friends.
	 *
	 * This is all done with fixed-point math, oriented around powers
	 * of two, which with luck will keep the CPU usage to a minimum.
	 * More could probably be done, for example using lookup tables to
	 * replace multiplies and divides; whether the memory hit (128K
	 * for each table) is worthwhile is unclear.
	 */
	if (loudness)
	{
		/* We aren't really going backwards, but i > 0 is fast */
		for (i = buflen / 2; i > 0; i--)
		{
			/*
			 * Adjust this sample to the current level.
			 */
			aval = (*buf16 = (((long)*buf16) * volume) >> 15);
			buf16++;

			/*
			 * Don't adjust the decaying average for each sample;
			 * that just spends CPU time for very little benefit.
			 */
			if (i & 127)
				continue;

			/*
			 * We want to use absolute values to compute the
			 * decaying average; otherwise it'd sit around 0.
			 */
			if (aval < 0)
				aval = -aval;

			/*
			 * Adjust more quickly when we start hitting peaks,
			 * or we'll get clipping when there's a sudden loud
			 * section after lots of quiet.
			 */
			if (aval & ~8191)
				aval <<= 3;

			/*
			 * Adjust the decaying average.
			 */
			level = ((level << 11) - level + aval) >> 11;

			/*
			 * Let *really* quiet sounds play softly, or we'll
			 * amplify background hiss to full volume and blast
			 * the user's speakers when real sound starts up.
			 */
			if (! (level & ~511))
				level = 512;

			/*
			 * And adjust the volume setting using the inverse
			 * of the decaying average.
			 */
			volume = (2500 << 15) / level;
		}
	}

	if (speed == 128)
		return (buflen);

	/*
	 * Half-speed play.  Stretch things out.
	 */
	if (speed == 0)
	{
		buflen /= 2;	/* Since we're moving 32 bits at a time */

		for (i = buflen - 1; i > 0; i--)
		{
			buf32[i] = buf32[i / 2];
		}

		buflen *= 4;	/* 2 for doubling the buffer, 2 from above */
	}

	/*
	 * Slow play; can't optimize it as well as half-speed.
	 */
	if (speed && speed < 128)
	{
		int	multiplier = ((speed + 128) * 128) / 256;
		int	newlen;
		int	tally = 0, pos;

		buflen /= 4;	/* Get the number of 32-bit values */

		/*
		 * Buffer length doubles when speed is 0, stays the same
		 * when speed is 128.
		 */
		newlen = (buflen * 128) / multiplier;

		pos = buflen - 1;
		for (i = newlen - 1; i > 0; i--)
		{
			buf32[i] = buf32[pos];
			tally += multiplier;
			if (tally & 128)
			{
				pos--;
				tally ^= 128;
			}
		}

		buflen = newlen * 4;
	}

	return (buflen);
}


int main(int argc, char **argv)
{
  fd_set			readfd, dummyfd;
  struct timeval		timeout;
  struct cdda_block	blockinfo;
  long			result;
  int			nfds;
  struct cdda_device      dev;
  const char      *sondsystem;
  const char     *sounddevice;
  const char  *sounddevicectl;

  memset(&blockinfo, 0, sizeof(struct cdda_block));
  
  /*
   * Device name should be the command-line argument.
   */
  if (argc < 2)
    dev.devname = NULL;
  else
    dev.devname = argv[1];

  if (argc < 3)
    sondsystem = "arts";
  else
    sondsystem = argv[2];

  if (argc < 4)
    sounddevice = NULL;
  else
    sounddevice = argv[3];

  if (argc < 5)
    sounddevicectl = NULL;
  else
    sounddevicectl = argv[3];

  DEBUGLOG("cddaslave: called with %s %s %s %s\n",
    dev.devname?dev.devname:"",
    sondsystem?sondsystem:"",
    sounddevice?sounddevice:"",
    sounddevicectl?sounddevicectl:"");

	/*
	 * If we're running setuid root, bump up our priority then lose
	 * superuser access.
	 */
	nice(-14);
	setuid(getuid());

	FD_ZERO(&dummyfd);
	FD_ZERO(&readfd);

	timerclear(&timeout);

  dev.fd = -1;
  wmcdda_init(&dev, &blockinfo);

  oops = setup_soundsystem(sondsystem, sounddevice, sounddevicectl);
  if (!oops) {
    ERRORLOG("cddaslave: setup_soundsystem failed\n");
    exit(1);
  }

  DEBUGLOG("cddaslave: sent first ACK\n");
  SEND_ACK(&blockinfo);

	/*
	 * Accept commands as they come in, and play some sound if we're
	 * supposed to be doing that.
	 */
  while (1) {
		FD_SET(0, &readfd);

		/*
		 * If we're playing, we don't want select to block.  Otherwise,
		 * wait a little while for the next command.
		 */
		if (playing)
			timeout.tv_usec = 0;
		else
			timeout.tv_usec = 500000;

		nfds = select(1, &readfd, &dummyfd, &dummyfd, &timeout);

    if (nfds < 0) { /* Broken pipe; our GUI exited. */
      wmcdda_close(&dev);
      oops->wmaudio_close();
			exit(0);
		}

    if (FD_ISSET(0, &readfd)) {
      recieve_command(&dev, &blockinfo);
			/*
			 * Process all commands in rapid succession, rather
			 * than possibly waiting for a CDDA read.
			 */
			continue;
		}
		
    if ((blockinfo.status & ~WMCDDA_ACK) == WMCDDA_PLAYING) {
      result = wmcdda_read(&dev, &blockinfo);
      if (result <= 0 && blockinfo.status != WMCDDA_DONE) {
        ERRORLOG("cddaslave: wmcdda_read failed\n");
        blockinfo.status = WMCDDA_STOPPED;
					send_status(&blockinfo);
      } else {
        result = wmcdda_normalize(&dev, &blockinfo);
        if (output)
          fwrite(dev.buf, result, 1, output);

        if (oops->wmaudio_play(dev.buf, dev.buflen, &blockinfo)) {
          oops->wmaudio_stop();
          ERRORLOG("cddaslave: wmaudio_play failed\n");
          blockinfo.status = WMCDDA_STOPPED;
          send_status(&blockinfo);
        }
      }
/*    } else {
      SEND_STATUS(&blockinfo);*/
    }
      else
        send_status(&blockinfo);
  }

  return 0;
}

#endif /* BUILD_CDDA */
