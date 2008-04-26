/*
 *  KCompactDisc - A CD drive interface for the KDE Project.
 *
 *  Copyright (C) 2005 Shaheedur R. Haque <srhaque@iee.org>
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

#include "kcompactdisc.h"
#include "kcompactdisc_p.h"

#include <QtDBus>

#include <kdebug.h>
#include <kurl.h>
#include <klocale.h>

#include <solid/device.h>
#include <solid/block.h>
#include <solid/opticaldrive.h>

static QMap<QString, KUrl> cdromsNameToDeviceUrl;
static QMap<QString, QString> cdromsNameToUdi;
static QString ___null = QString();

static void refreshListOfCdromDevices()
{
    cdromsNameToDeviceUrl.clear();
    cdromsNameToUdi.clear();
    QString name, type;
    KUrl url;

    //get a list of all devices that are Cdrom
    foreach(const Solid::Device &device, Solid::Device::listFromType(Solid::DeviceInterface::OpticalDrive)) {
        kDebug() << device.udi().toLatin1().constData();
        const Solid::Block *b = device.as<Solid::Block>();
        const Solid::OpticalDrive *o = device.as<Solid::OpticalDrive>();
        Solid::OpticalDrive::MediumTypes mediumType = o->supportedMedia();

        url = KUrl::fromPath(b->device().toLatin1());

        if(mediumType < Solid::OpticalDrive::Cdrw) {
            type = "CD-ROM";
        } else if(mediumType < Solid::OpticalDrive::Dvd) {
            type = "CDRW";
        } else if(mediumType < Solid::OpticalDrive::Dvdr) {
            type = "DVD-ROM";
        } else if(mediumType < Solid::OpticalDrive::Bd) {
            type = "DVDRW";
        } else if(mediumType < Solid::OpticalDrive::HdDvd) {
            type = "Blu-ray";
        } else {
            type = "High Density DVD";
        }

        if(!device.vendor().isEmpty())
            name = (QString('[') + type + " - " + device.vendor() + " - " + device.product() + "]");
        else
            name = (QString('[') + type + " - unknown vendor - " + device.product() + "]");

        cdromsNameToDeviceUrl.insert(name, url);
        cdromsNameToUdi.insert(name, device.udi());
    }
#if 0
    if(cdromsNameToDeviceUrl.empty()) {
        cdromsNameToDeviceUrl.insert(QString("Generic CDROM []"), KUrl::fromPath(wm_drive_default_device()));
    }
#endif
}

static QMap<QString, KUrl> &getListOfCdromDevicesNamesAndUrl()
{
    if(cdromsNameToDeviceUrl.empty())
        refreshListOfCdromDevices();

    return cdromsNameToDeviceUrl;
}

static QMap<QString, QString> &getListOfCdromDevicesNamesAndUdi()
{
    if(cdromsNameToUdi.empty())
        refreshListOfCdromDevices();

    return cdromsNameToUdi;
}

QString KCompactDisc::urlToDevice(const KUrl& deviceUrl)
{
    if(deviceUrl.protocol() == "media" || deviceUrl.protocol() == "system") {
        kDebug() << "Asking mediamanager for " << deviceUrl.fileName();

        QDBusInterface mediamanager( "org.kde.kded", "/modules/mediamanager", "org.kde.MediaManager" );
        QDBusReply<QStringList> reply = mediamanager.call("properties", deviceUrl.fileName());

        QStringList properties = reply;
        if(!reply.isValid() || properties.count() < 6) {
            kError() << "Invalid reply from mediamanager" << endl;
            return deviceUrl.path();
        } else {
            kDebug() << "Reply from mediamanager " << properties[5];
            return properties[5];
        }
    } else if(deviceUrl.protocol() == "file") {
        return deviceUrl.path();
    } else {
        return QString();
    }
}

const QStringList KCompactDisc::audioSystems()
{
    QStringList list;

    list << "phonon"
#ifdef USE_ARTS
        << "arts"
#endif
#if defined(HAVE_LIBASOUND2)
        << "alsa"
#endif
#if defined(sun) || defined(__sun__)
        << "sun"
#endif
    ;
    return list;
}

const QStringList KCompactDisc::cdromDeviceNames()
{
    return getListOfCdromDevicesNamesAndUrl().keys();
}

const QString KCompactDisc::defaultCdromDeviceName()
{
    return getListOfCdromDevicesNamesAndUrl().keys().at(0);
}

const KUrl KCompactDisc::defaultCdromDeviceUrl()
{
    return getListOfCdromDevicesNamesAndUrl().values().at(0);
}

const KUrl KCompactDisc::cdromDeviceUrl(const QString &cdromDeviceName)
{
    return getListOfCdromDevicesNamesAndUrl().value(cdromDeviceName, KCompactDisc::defaultCdromDeviceUrl());
}

const QString KCompactDisc::defaultCdromDeviceUdi()
{
    return getListOfCdromDevicesNamesAndUdi().values().at(0);
}

const QString KCompactDisc::cdromDeviceUdi(const QString &cdromDeviceName)
{
    return getListOfCdromDevicesNamesAndUdi().value(cdromDeviceName, KCompactDisc::defaultCdromDeviceUdi());
}

KCompactDisc::KCompactDisc(InformationMode infoMode) :
    d_ptr(new KCompactDiscPrivate(this, KCompactDisc::defaultCdromDeviceName()))
{
    Q_D(KCompactDisc);
    dummy_ptr = d;
    d->m_infoMode = infoMode;
}

KCompactDisc::~KCompactDisc()
{
    stop();
}

const QString &KCompactDisc::deviceVendor()
{
    Q_D(KCompactDisc);
    return d->m_deviceVendor;
}

const QString &KCompactDisc::deviceModel()
{
    Q_D(KCompactDisc);
    return d->m_deviceModel;
}

const QString &KCompactDisc::deviceRevision()
{
    Q_D(KCompactDisc);
    return d->m_deviceRevision;
}

const QString &KCompactDisc::deviceName()
{
    Q_D(KCompactDisc);
    return d->m_deviceName;
}

const KUrl KCompactDisc::deviceUrl()
{
    Q_D(KCompactDisc);
    return KCompactDisc::cdromDeviceUrl(d->m_deviceName);
}

unsigned KCompactDisc::discId()
{
    Q_D(KCompactDisc);
    return d->m_discId;
}

const QList<unsigned> &KCompactDisc::discSignature()
{
    Q_D(KCompactDisc);
    return d->m_trackStartFrames;
}

const QString &KCompactDisc::discArtist()
{
    Q_D(KCompactDisc);
    if (!d->m_tracks)
        return ___null;
    return d->m_trackArtists[0];
}

const QString &KCompactDisc::discTitle()
{
    Q_D(KCompactDisc);
    if (!d->m_tracks)
        return ___null;
    return d->m_trackTitles[0];
}

unsigned KCompactDisc::discLength()
{
    Q_D(KCompactDisc);
    if (!d->m_tracks)
        return 0;
    return d->m_discLength;
}

unsigned KCompactDisc::discPosition()
{
    Q_D(KCompactDisc);
    return d->m_discPosition;
}

KCompactDisc::DiscStatus KCompactDisc::discStatus()
{
    Q_D(KCompactDisc);
    return d->m_status;
}

QString KCompactDisc::discStatusString(KCompactDisc::DiscStatus status)
{
    return KCompactDiscPrivate::discStatusI18n(status);
}

QString KCompactDisc::trackArtist()
{
	Q_D(KCompactDisc);
	return trackArtist(d->m_track);
}

QString KCompactDisc::trackArtist(unsigned track)
{
	Q_D(KCompactDisc);
    if (!track)
        return QString();
    return d->m_trackArtists[track];
}

QString KCompactDisc::trackTitle()
{
	Q_D(KCompactDisc);
    return trackTitle(d->m_track);
}

QString KCompactDisc::trackTitle(unsigned track)
{
	Q_D(KCompactDisc);
    if (!track)
        return QString();
    return d->m_trackTitles[track];
}

unsigned KCompactDisc::trackLength()
{
	Q_D(KCompactDisc);
    return trackLength(d->m_track);
}

unsigned KCompactDisc::trackLength(unsigned track)
{
	Q_D(KCompactDisc);
    if (!track)
        return 0;
    return d->trackLength(track);
}

unsigned KCompactDisc::track()
{
	Q_D(KCompactDisc);
	return d->m_track;
}

unsigned KCompactDisc::trackPosition()
{
	Q_D(KCompactDisc);
    return d->m_trackPosition;
}

unsigned KCompactDisc::tracks()
{
	Q_D(KCompactDisc);
    return d->m_tracks;
}

bool KCompactDisc::isPlaying()
{
	Q_D(KCompactDisc);
	return (d->m_status == KCompactDisc::Playing);
}

bool KCompactDisc::isPaused()
{
	Q_D(KCompactDisc);
	return (d->m_status == KCompactDisc::Paused);
}

bool KCompactDisc::isNoDisc()
{
	Q_D(KCompactDisc);
	return (d->m_status == KCompactDisc::NoDisc);
}

bool KCompactDisc::isAudio(unsigned track)
{
	Q_D(KCompactDisc);
    if (!track)
        return 0;
    return d->isTrackAudio(track);
}

void KCompactDisc::playTrack(unsigned track)
{
	Q_D(KCompactDisc);

	d->m_statusExpected = KCompactDisc::Playing;
    d->m_trackExpectedPosition = 0;
    d->m_seek = abs(d->m_trackExpectedPosition - trackPosition());

	d->playTrackPosition(track, 0);
}

void KCompactDisc::playPosition(unsigned position)
{
	Q_D(KCompactDisc);

	d->m_statusExpected = Playing;
    d->m_trackExpectedPosition = position;
    d->m_seek = abs(d->m_trackExpectedPosition - trackPosition());

	d->playTrackPosition(d->m_track, position);
}

void KCompactDisc::play()
{
	doCommand(KCompactDisc::Play);
}

void KCompactDisc::next()
{
	doCommand(KCompactDisc::Next);
}

void KCompactDisc::prev()
{
	doCommand(KCompactDisc::Prev);
}

void KCompactDisc::pause()
{
	doCommand(KCompactDisc::Pause);
}

void KCompactDisc::stop()
{
	doCommand(KCompactDisc::Stop);
}

void KCompactDisc::eject()
{
	doCommand(KCompactDisc::Eject);
}

void KCompactDisc::loop()
{
	doCommand(KCompactDisc::Loop);
}

void KCompactDisc::random()
{
	doCommand(KCompactDisc::Random);
}

void KCompactDisc::doCommand(KCompactDisc::DiscCommand cmd)
{
	Q_D(KCompactDisc);
	unsigned track;

	switch(cmd) {
	case Play:
		if(d->m_status == KCompactDisc::Playing)
			return;
		next();
		break;

	case Next:
		track = d->getNextTrackInPlaylist();
    	if(track)
			playTrack(track);
		break;

	case Prev:
		track = d->getPrevTrackInPlaylist();
    	if(track)
			playTrack(track);
		break;

	case Pause:
		if(d->m_status == KCompactDisc::Paused)
			d->m_statusExpected = KCompactDisc::Playing;
		else
			d->m_statusExpected = KCompactDisc::Paused;

		d->pause();
		break;

	case Stop:
		d->m_statusExpected = KCompactDisc::Stopped;
    	d->stop();
		break;

	case Eject:
		if(d->m_status != KCompactDisc::Ejected) {
			if(d->m_status != KCompactDisc::Stopped) {
				d->m_statusExpected = KCompactDisc::Ejected;
				d->stop();
			} else {
				d->eject();
			}
		} else {
			d->m_statusExpected = KCompactDisc::Stopped;
			d->closetray();
		}
		break;

	case Loop:
		setLoopPlaylist(!d->m_loopPlaylist);
		break;

	case Random:
		setRandomPlaylist(!d->m_randomPlaylist);
		break;
	}
}

void KCompactDisc::metadataLookup()
{
	Q_D(KCompactDisc);
	d->queryMetadata();
}

void KCompactDisc::setRandomPlaylist(bool random)
{
	Q_D(KCompactDisc);
	d->m_randomPlaylist = random;
	d->make_playlist();
	emit randomPlaylistChanged(d->m_randomPlaylist);
}

void KCompactDisc::setLoopPlaylist(bool loop)
{
	Q_D(KCompactDisc);
	d->m_loopPlaylist = loop;
	emit loopPlaylistChanged(d->m_loopPlaylist);
}

void KCompactDisc::setAutoMetadataLookup(bool autoMetadata)
{
	Q_D(KCompactDisc);
	d->m_autoMetadata = autoMetadata;
	if(d->m_autoMetadata)
		metadataLookup();
}

bool KCompactDisc::setDevice(const QString &deviceName, unsigned volume,
    bool digitalPlayback, const QString &audioSystem, const QString &audioDevice)
{
	QString as = digitalPlayback ? audioSystem : QString("cdin");
	QString ad = digitalPlayback ? audioDevice : QString();
    kDebug() << "Device init: " << deviceName << ", " << as << ", " << ad;

	if(dummy_ptr->moveInterface(deviceName, as, ad)) {
		setVolume(volume);
		return 1;
	} else {
        // Severe (OS-level) error.
		return 0;
    }
}

void KCompactDisc::setVolume(unsigned volume)
{
	Q_D(KCompactDisc);
    kDebug() << "change volume: " << volume;
	d->setVolume(volume);
}

void KCompactDisc::setBalance(unsigned balance)
{
	Q_D(KCompactDisc);
    kDebug() << "change balance: " << balance;
	d->setBalance(balance);
}

#include "kcompactdisc.moc"
