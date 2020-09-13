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

#include "wmlib_interface.h"

#include <QtGlobal>

#include <KLocalizedString>

extern "C"
{
	// We don't have libWorkMan installed already, so get everything
	// from within our own directory
	#include "wmlib/include/wm_cdrom.h"
	#include "wmlib/include/wm_cdtext.h"
	#include "wmlib/include/wm_helpers.h"
}

#define TRACK_VALID(track) ((track) && (track <= m_tracks))

KWMLibCompactDiscPrivate::KWMLibCompactDiscPrivate(KCompactDisc *p,
	const QString &dev, const QString &audioSystem, const QString &audioDevice) :
	KCompactDiscPrivate(p, dev),
	m_handle(nullptr),
	m_audioSystem(audioSystem),
	m_audioDevice(audioDevice)
{
	m_interface = m_audioSystem;
}

KWMLibCompactDiscPrivate::~KWMLibCompactDiscPrivate()
{
	if (m_handle) {
		wm_cd_destroy(m_handle);
	}
}

bool KWMLibCompactDiscPrivate::createInterface()
{
	const QString devicePath = KCompactDisc::cdromDeviceUrl(m_deviceName).path();

	// Debug.
	if (qEnvironmentVariableIsSet("KCOMPACTDISC_WMLIB_DEBUG")) {
		wm_cd_set_verbosity(WM_MSG_LEVEL_DEBUG | WM_MSG_CLASS_ALL);
	}

	int status = wm_cd_init(
        devicePath.toLatin1().data(),
        m_audioSystem.toLatin1().data(),
        m_audioDevice.toLatin1().data(),
		nullptr,
		&m_handle);

	if(!WM_CDS_ERROR(status)) {
		m_deviceVendor = QLatin1String(wm_drive_vendor(m_handle));
		m_deviceModel = QLatin1String(wm_drive_model(m_handle));
		m_deviceRevision = QLatin1String(wm_drive_revision(m_handle));

		Q_Q(KCompactDisc);
		Q_EMIT q->discChanged(0);

		if (m_infoMode == KCompactDisc::Asynchronous) {
			timerExpired();
		} else {
			QTimer::singleShot(1000, this, SLOT(timerExpired()));
		}

		return true;
	}
	m_handle = nullptr;
	return false;
}

unsigned KWMLibCompactDiscPrivate::trackLength(unsigned track)
{
	return (unsigned)wm_cd_gettracklen(m_handle, track);
}

bool KWMLibCompactDiscPrivate::isTrackAudio(unsigned track)
{
	return !wm_cd_gettrackdata(m_handle, track);
}

void KWMLibCompactDiscPrivate::playTrackPosition(unsigned track, unsigned position)
{
	unsigned firstTrack, lastTrack;

	firstTrack = TRACK_VALID(track) ? track : 1;
	lastTrack = firstTrack + 1;
	lastTrack = TRACK_VALID(lastTrack) ? lastTrack : WM_ENDTRACK;

    qDebug() << "play track " << firstTrack << " position "
                 << position;

    wm_cd_play(m_handle, firstTrack, position, lastTrack);
}

void KWMLibCompactDiscPrivate::pause()
{
	wm_cd_pause(m_handle);
}

void KWMLibCompactDiscPrivate::stop()
{
	wm_cd_stop(m_handle);
}

void KWMLibCompactDiscPrivate::eject()
{
	wm_cd_eject(m_handle);
}

void KWMLibCompactDiscPrivate::closetray()
{
	wm_cd_closetray(m_handle);
}

/* WM_VOLUME_MUTE ... WM_VOLUME_MAXIMAL */
/* WM_BALANCE_ALL_LEFTS .WM_BALANCE_SYMMETRED. WM_BALANCE_ALL_RIGHTS */
#define RANGE2PERCENT(x, min, max) (((x) - (min)) * 100)/ ((max) - (min))
#define PERCENT2RANGE(x, min, max) ((((x) * ((max) - (min))) / 100 ) + (min))
void KWMLibCompactDiscPrivate::setVolume(unsigned volume)
{
	int vol, bal;
	vol = PERCENT2RANGE(volume, WM_VOLUME_MUTE, WM_VOLUME_MAXIMAL);
	bal = wm_cd_getbalance(m_handle);
	wm_cd_volume(m_handle, vol, bal);
}

void KWMLibCompactDiscPrivate::setBalance(unsigned balance)
{
	int vol, bal;
	vol = wm_cd_getvolume(m_handle);
	bal = PERCENT2RANGE(balance, WM_BALANCE_ALL_LEFTS, WM_BALANCE_ALL_RIGHTS);
	wm_cd_volume(m_handle, vol, bal);
}

unsigned KWMLibCompactDiscPrivate::volume()
{
	int vol = wm_cd_getvolume(m_handle);
	unsigned volume = RANGE2PERCENT(vol, WM_VOLUME_MUTE, WM_VOLUME_MAXIMAL);
	return volume;
}

unsigned KWMLibCompactDiscPrivate::balance()
{
	int bal = wm_cd_getbalance(m_handle);
	unsigned balance = RANGE2PERCENT(bal, WM_BALANCE_ALL_LEFTS, WM_BALANCE_ALL_RIGHTS);

	return balance;
}

void KWMLibCompactDiscPrivate::queryMetadata()
{
	cdtext();
	//cddb();
}

KCompactDisc::DiscStatus KWMLibCompactDiscPrivate::discStatusTranslate(int status)
{
	switch (status) {
	case WM_CDM_TRACK_DONE:
	case WM_CDM_PLAYING:
	case WM_CDM_FORWARD:
		return KCompactDisc::Playing;
	case WM_CDM_PAUSED:
		return KCompactDisc::Paused;
	case WM_CDM_STOPPED:
		return KCompactDisc::Stopped;
	case WM_CDM_EJECTED:
		return KCompactDisc::Ejected;
	case WM_CDM_NO_DISC:
	case WM_CDM_UNKNOWN:
		return KCompactDisc::NoDisc;
	case WM_CDM_CDDAERROR:
	case WM_CDM_LOADING:
	case WM_CDM_BUFFERING:
		return KCompactDisc::NotReady;
	default:
		return KCompactDisc::Error;
	}
}

void KWMLibCompactDiscPrivate::timerExpired()
{
	KCompactDisc::DiscStatus status;
	unsigned track, i;
	Q_Q(KCompactDisc);

	status = discStatusTranslate(wm_cd_status(m_handle));

	if(m_status != status) {
		if(skipStatusChange(status))
			goto timerExpiredExit;

		m_status = status;

		switch(m_status) {
		case KCompactDisc::Ejected:
		case KCompactDisc::NoDisc:
			clearDiscInfo();
			break;
		default:
			if(m_tracks == 0) {
				m_tracks = wm_cd_getcountoftracks(m_handle);
				if(m_tracks > 0) {
                    qDebug() << "New disc with " << m_tracks << " tracks";
					m_discId = wm_cddb_discid(m_handle);

					for(i = 1; i <= m_tracks; ++i) {
						m_trackStartFrames.append(wm_cd_gettrackstart(m_handle, i));
					}
					m_trackStartFrames.append(wm_cd_gettrackstart(m_handle, i));

					m_discLength = FRAMES2SEC(m_trackStartFrames[m_tracks] -
						m_trackStartFrames[0]);

					make_playlist();

					m_trackArtists.append(i18n("Unknown Artist"));
					m_trackTitles.append(i18n("Unknown Title"));
					for(i = 1; i <= m_tracks; ++i) {
						m_trackArtists.append(i18n("Unknown Artist"));
						m_trackTitles.append(ki18n("Track %1").subs(i, 2).toString());
					}

qDebug() << "m_tracks " << m_tracks;
qDebug() << "m_trackStartFrames " << m_trackStartFrames;
qDebug() << "m_trackArtists " << m_trackArtists;
qDebug() << "m_trackTitles " << m_trackTitles;

					Q_EMIT q->discChanged(m_tracks);

					if(m_autoMetadata)
						queryMetadata();
				}
			}
			break;
		}
	}

	switch(m_status) {
	case KCompactDisc::Playing:
		m_trackPosition = wm_get_cur_pos_rel(m_handle);
		m_discPosition = wm_get_cur_pos_abs(m_handle) - FRAMES2SEC(m_trackStartFrames[0]);
		// Update the current playing position.
		if(m_seek) {
            qDebug() << "seek: " << m_seek << " trackPosition " << m_trackPosition;
			if(abs((long)(m_trackExpectedPosition - m_trackPosition)) > m_seek)
				m_seek = 0;
			else
				m_seek = abs((long)(m_trackExpectedPosition - m_trackPosition));
		}

		if(!m_seek) {
			Q_EMIT q->playoutPositionChanged(m_trackPosition);
			//Q_EMIT q->playoutDiscPositionChanged(m_discPosition);
		}

		// Per-event processing.
		track = wm_cd_getcurtrack(m_handle);

		if(m_track != track) {
			m_track = track;
			Q_EMIT q->playoutTrackChanged(m_track);
		}
		break;

	case KCompactDisc::Stopped:
		m_seek = 0;
		m_track = 0;
		break;

	default:
		break;
	}

timerExpiredExit:
	// Now that we have incurred any delays caused by the signals, we'll start the timer.
	QTimer::singleShot(1000, this, SLOT(timerExpired()));
}

void KWMLibCompactDiscPrivate::cdtext()
{
	struct cdtext_info *info;
	unsigned i;
	Q_Q(KCompactDisc);

	info = wm_cd_get_cdtext(m_handle);

	if(!info || !info->valid || (unsigned)info->count_of_entries != (m_tracks + 1)) {
        qDebug() << "no or invalid CDTEXT";
		return;
	}

	m_trackArtists[0] = QLatin1String( reinterpret_cast<char*>(info->blocks[0]->performer[0]) );
	m_trackTitles[0] = QLatin1String( reinterpret_cast<char*>(info->blocks[0]->name[0]) );

	for(i = 1; i <= m_tracks; ++i) {
            m_trackArtists[i] = QLatin1String( reinterpret_cast<char*>(info->blocks[0]->performer[i]) );
            m_trackTitles[i] =QLatin1String( reinterpret_cast<char*>(info->blocks[0]->name[i]) );
	}

    qDebug() << "CDTEXT";
    qDebug() << "m_trackArtists " << m_trackArtists;
    qDebug() << "m_trackTitles " << m_trackTitles;

	Q_EMIT q->discInformation(KCompactDisc::Cdtext);
}
