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

#include <kdebug.h>
#include <klocale.h>

#include "kcompactdisc_p.h"
#include "wmlib_interface.h"
#include "phonon_interface.h"

KCompactDiscPrivate::KCompactDiscPrivate(KCompactDisc *p, const QString& dev) :
    m_infoMode(KCompactDisc::Synchronous),
    m_deviceName(dev),

    m_status(KCompactDisc::NoDisc),
    m_statusExpected(KCompactDisc::NoDisc),
    m_discId(0),
	m_discLength(0),
    m_track(0),
    m_tracks(0),
    m_trackPosition(0),
    m_discPosition(0),
    m_trackExpectedPosition(0),
    m_seek(0),

    m_randSequence(0),
    m_loopPlaylist(false),
    m_randomPlaylist(false),
	m_autoMetadata(true),

	m_deviceVendor(QString()),
    m_deviceModel(QString()),
    m_deviceRevision(QString()),

	q_ptr(p)
{
	m_interface = QString("dummy");
    m_trackStartFrames.clear();
	m_trackArtists.clear();
    m_trackTitles.clear();
	m_playlist.clear();
}

bool KCompactDiscPrivate::moveInterface(const QString &deviceName,
	const QString &audioSystem, const QString &audioDevice)
{
	Q_Q(KCompactDisc);

	KCompactDiscPrivate *pOld, *pNew;

	kDebug() << "switch from " << q->d_ptr->m_interface << " on " << q->d_ptr->m_deviceName << endl;
	kDebug() << "         to " << audioSystem << " on " << deviceName << endl;

	/* switch temporary to dummy implementation */
	if(q->d_ptr != this) {
		pOld = q->d_ptr;
		q->d_ptr = this;
		delete pOld;
	}

	if(audioSystem == QString("phonon"))
		pNew = new KPhononCompactDiscPrivate(q, deviceName);
	else
		pNew = new KWMLibCompactDiscPrivate(q, deviceName,
			audioSystem, audioDevice);

	pNew->m_infoMode = m_infoMode;

	if(pNew->createInterface()) {
		q->d_ptr = pNew;
		return true;
	} else {
		delete pNew;
		return false;
    }
}

bool KCompactDiscPrivate::createInterface()
{
	return true;
}

void KCompactDiscPrivate::make_playlist()
{
    /* koz: 15/01/00. I want a random list that does not repeat tracks. Ie, */
    /* a list is created in which each track is listed only once. The tracks */
    /* are picked off one by one until the end of the list */

    unsigned selected = 0, size = m_tracks;
    bool rejected = false;

    kDebug(67000) << "Playlist has " << size << " entries\n" << endl;
    m_playlist.clear();
    for(unsigned i = 0; i < size; i++) {
        if(m_randomPlaylist) {
        	do {
            	selected = 1 + m_randSequence.getLong(size);
            	rejected = (m_playlist.indexOf(selected) != -1);
        	} while(rejected == true);
		} else {
			selected = 1 + i;	
		}
        m_playlist.append(selected);
    }

    kDebug(67000) << "debug: dump playlist" << endl;
    QList<unsigned>::iterator it;
    for(it = m_playlist.begin(); it != m_playlist.end(); it++) {
        kDebug(67000) << "debug: " << *it << endl;
    }
    kDebug(67000) << "debug: dump playlist end" << endl;
}

unsigned KCompactDiscPrivate::getNextTrackInPlaylist()
{
    int current_index, min_index, max_index;

	if(m_playlist.empty())
		return 0;

	min_index = 0;
    max_index = m_playlist.size() - 1;
	
	current_index = m_playlist.indexOf(m_track);
    if(current_index < 0)
		current_index = min_index;
	else if(current_index >= max_index) {
		if(m_loopPlaylist) {
			//wrap around
			if(m_randomPlaylist)
				make_playlist();

			current_index = min_index;
		} else {
			return 0;
		}
	} else {
		++current_index;
	}

	return m_playlist[current_index];
}

unsigned KCompactDiscPrivate::getPrevTrackInPlaylist()
{
    int current_index, min_index, max_index;

	if(m_playlist.empty())
		return 0;

	min_index = 0;
    max_index = m_playlist.size() - 1;

	current_index = m_playlist.indexOf(m_track);
    if(current_index < 0)
		current_index = min_index;
	else if(current_index <= min_index) {
		if(m_loopPlaylist) {
			//wrap around
			if(m_randomPlaylist)
				make_playlist();

			current_index = max_index;
		} else {
			return 0;
		}
	} else {
		--current_index;
	}

	return m_playlist[current_index];
}

bool KCompactDiscPrivate::skipStatusChange(KCompactDisc::DiscStatus status)
{
	Q_Q(KCompactDisc);

	if(m_status != status) {
		if(status == KCompactDisc::Stopped) {
			if(m_statusExpected == KCompactDisc::Ejected) {
				eject();
			} else if(m_statusExpected != KCompactDisc::Stopped) {
				unsigned track = getNextTrackInPlaylist();
				if(track) {
					playTrackPosition(track, 0);
					return true;
				}
			}
		}
	
		emit q->discStatusChanged(status);
	}

	return false;
}

const QString KCompactDiscPrivate::discStatusI18n(KCompactDisc::DiscStatus status)
{
    switch (status) {
    case KCompactDisc::Playing:
        return i18n("Playing");
    case KCompactDisc::Paused:
        return i18n("Paused");
    case KCompactDisc::Stopped:
        return i18n("Stopped");
	case KCompactDisc::Ejected:
		return i18n("Ejected");
    case KCompactDisc::NoDisc:
        return i18n("No Disc");
    case KCompactDisc::NotReady:
        return i18n("Not Ready");
    case KCompactDisc::Error:
    default:
        return i18n("Error");
    }
}

void KCompactDiscPrivate::clearDiscInfo()
{
	Q_Q(KCompactDisc);

	m_discId = 0;
	m_discLength = 0;
	m_seek = 0;
	m_track = 0;
	m_tracks = 0;
	m_trackArtists.clear();
	m_trackTitles.clear();
	m_trackStartFrames.clear();
	emit q->discChanged(m_tracks);
}

unsigned KCompactDiscPrivate::trackLength(unsigned)
{
	return 0;
}

bool KCompactDiscPrivate::isTrackAudio(unsigned)
{
	return false;
}

void KCompactDiscPrivate::playTrackPosition(unsigned, unsigned)
{
}

void KCompactDiscPrivate::pause()
{
}

void KCompactDiscPrivate::stop()
{
}

void KCompactDiscPrivate::eject()
{
}

void KCompactDiscPrivate::closetray()
{
}

void KCompactDiscPrivate::setVolume(unsigned)
{
}

void KCompactDiscPrivate::setBalance(unsigned)
{
}

unsigned KCompactDiscPrivate::volume()
{
	return 0;
}

unsigned KCompactDiscPrivate::balance()
{
	return 50;
}

void KCompactDiscPrivate::queryMetadata()
{
}

#include "kcompactdisc_p.moc"
