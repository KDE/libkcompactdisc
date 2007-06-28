/*
 *  KCompactDisc - A CD drive interface for the KDE Project.
 *
 *  Copyright (C) 2007 Alexander Kern <alex.kern@gmx.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KCOMPACTDISC_P_H
#define KCOMPACTDISC_P_H

#include <QString>
#include <QList>

#include <krandomsequence.h>
#include <kurl.h>
#include "kcompactdisc.h"

class KCompactDiscPrivate : public QObject
{
	Q_OBJECT

	public:
		KCompactDiscPrivate(KCompactDisc *, const QString&);
		virtual ~KCompactDiscPrivate() { };
	
		bool moveInterface(const QString &, const QString &, const QString &);
		virtual bool createInterface();

		QString m_interface;
		KCompactDisc::InformationMode m_infoMode;
		QString m_deviceName;
	
		KCompactDisc::DiscStatus m_status;
		KCompactDisc::DiscStatus m_statusExpected;
		unsigned m_discId;
        unsigned m_discLength;
		unsigned m_track;
		unsigned m_tracks;
		unsigned m_trackPosition;
		unsigned m_discPosition;
		unsigned m_trackExpectedPosition;
		int m_seek;
	
		QList<unsigned> m_trackStartFrames;
		QStringList m_trackArtists;
		QStringList m_trackTitles;
	
		KRandomSequence m_randSequence;
		QList<unsigned> m_playlist;
		bool m_loopPlaylist;
		bool m_randomPlaylist;
		bool m_autoMetadata;
	
		void make_playlist();
		unsigned getNextTrackInPlaylist();
		unsigned getPrevTrackInPlaylist();
		bool skipStatusChange(KCompactDisc::DiscStatus);
		static QString discStatusI18n(KCompactDisc::DiscStatus);

		void clearDiscInfo();

		virtual unsigned trackLength(unsigned);
		virtual bool isTrackAudio(unsigned);
		virtual void playTrackPosition(unsigned, unsigned);
		virtual void pause();
		virtual void stop();
		virtual void eject();
		virtual void closetray();
	
		virtual void setVolume(unsigned);
		virtual void setBalance(unsigned);
		virtual unsigned volume();
		virtual unsigned balance();

		virtual void queryMetadata();
	
		QString m_deviceVendor;
		QString m_deviceModel;
		QString m_deviceRevision;

	public:
		Q_DECLARE_PUBLIC(KCompactDisc)
		KCompactDisc * const q_ptr;
};


#define SEC2FRAMES(sec) ((sec) * 75)
#define FRAMES2SEC(frames) ((frames) / 75)
#define MS2SEC(ms) ((ms) / 1000)
#define SEC2MS(sec) ((sec) * 1000)
#define MS2FRAMES(ms) (((ms) * 75) / 1000)
#define FRAMES2MS(frames) (((frames) * 1000) / 75)

#endif /* KCOMPACTDISC_P_H */
