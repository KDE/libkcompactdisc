/*
 *
 * Copyright (C) 2004-2007 Matthias Kretz <kretz@kde.org>
 * Copyright (C) by Alexander Kern <alex.kern@gmx.de>
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
 * CDDA version taken from guitest in phonon test directory
 */

#include <kdebug.h>
#include <klocale.h>

#include <Phonon/Global>
#include <Phonon/MediaObject>
#include <Phonon/AudioOutput>
#include <Phonon/Path>
#include <Phonon/MediaController>

#include <solid/device.h>
#include <solid/opticaldrive.h>
#include <solid/opticaldisc.h>

#include "phonon_interface.h"

#define WM_MSG_CLASS WM_MSG_CLASS_PLATFORM

using namespace Phonon;

class ProducerWidget : public QObject
{
    public:
        ProducerWidget(KPhononCompactDiscPrivate *, const QString &);
        ~ProducerWidget();

    public:
        MediaObject *m_media;
        AudioOutput *m_output;
        MediaController *m_mediaController;
};

ProducerWidget::ProducerWidget(KPhononCompactDiscPrivate *p, const QString &Udi) :
    m_media(0),
    m_output(0),
    m_mediaController(0)
{
    m_media = new MediaObject(this);
    connect(m_media, SIGNAL(metaDataChanged()), SLOT(updateMetaData()));
    m_media->setTickInterval(1000);

    m_output = new AudioOutput(Phonon::MusicCategory, this);
    Phonon::createPath(m_media, m_output);

    connect(m_media, SIGNAL(stateChanged(Phonon::State, Phonon::State)),
            p, SLOT(stateChanged(Phonon::State, Phonon::State)));

    connect(m_media, SIGNAL(tick(qint64)), p, SLOT(tick(qint64)));

    MediaSource *m_mediaSource = new MediaSource(Phonon::Cd, Udi);
    m_media->setCurrentSource(*m_mediaSource);

    m_mediaController = new MediaController(m_media);
}

ProducerWidget::~ProducerWidget()
{
	delete(m_mediaController);
    delete(m_output);
	delete(m_media);
}

KPhononCompactDiscPrivate::KPhononCompactDiscPrivate(KCompactDisc *p,
	const QString &dev) :
	KCompactDiscPrivate(p, dev),
	m_producerWidget(NULL),
	m_udi(KCompactDisc::cdromDeviceUdi(dev))
{
	m_interface = QString("phonon");
}

KPhononCompactDiscPrivate::~KPhononCompactDiscPrivate()
{
	delete m_producerWidget;
	m_producerWidget = NULL;
}

bool KPhononCompactDiscPrivate::createInterface()
{
	Solid::Device opticalDevice(m_udi);
	Solid::OpticalDrive *opticalDrive = opticalDevice.as<Solid::OpticalDrive>();

	if(opticalDrive) {
		Q_Q(KCompactDisc);

		m_deviceVendor = opticalDevice.vendor();
		m_deviceModel = opticalDevice.product();

		emit q->discChanged(0);

		producer();

		return true;
	}

	return false; 
}

ProducerWidget *KPhononCompactDiscPrivate::producer()
{
	//try to create
	if(!m_producerWidget) {
		Solid::Device opticalDevice(m_udi);
		Solid::OpticalDrive *opticalDrive = opticalDevice.as<Solid::OpticalDrive>();	

		if(opticalDrive) {
			Solid::OpticalDisc *opticalDisc = opticalDevice.as<Solid::OpticalDisc>();
			kDebug() << "opticalDisc " << opticalDisc;
			//if(opticalDisc && (opticalDisc->availableContent() == Solid::OpticalDisc::Audio)) {
				m_producerWidget = new ProducerWidget(this, m_udi);
			//}
		}
	}

	return m_producerWidget;
}

unsigned KPhononCompactDiscPrivate::trackLength(unsigned track)
{
    if(!producer() || m_producerWidget->m_mediaController->currentTitle() != track)
		return 0;

	return MS2SEC(m_producerWidget->m_media->totalTime());
}

bool KPhononCompactDiscPrivate::isTrackAudio(unsigned)
{
	return true;
}

void KPhononCompactDiscPrivate::playTrackPosition(unsigned track, unsigned position)
{
    if(!producer())
		return;

	kDebug() << "play track " << track << " position " << position;

    m_producerWidget->m_mediaController->setCurrentTitle(track);
    m_producerWidget->m_media->seek(SEC2MS(position));
	emit m_producerWidget->m_media->play();
}

void KPhononCompactDiscPrivate::pause()
{
    if(!producer())
		return;

	emit m_producerWidget->m_media->pause();
}

void KPhononCompactDiscPrivate::stop()
{
    if(!producer())
		return;

	emit m_producerWidget->m_media->stop();
}

void KPhononCompactDiscPrivate::eject()
{
	Solid::Device opticalDevice(m_udi);
	Solid::OpticalDrive *opticalDrive = opticalDevice.as<Solid::OpticalDrive>();	
	Solid::OpticalDisc *opticalDisc = opticalDevice.as<Solid::OpticalDisc>();

	if(!opticalDrive || !opticalDisc)
		return;
	
	opticalDrive->eject();
}

void KPhononCompactDiscPrivate::closetray()
{
	Solid::Device opticalDevice(m_udi);
	Solid::OpticalDrive *opticalDrive = opticalDevice.as<Solid::OpticalDrive>();	
	Solid::OpticalDisc *opticalDisc = opticalDevice.as<Solid::OpticalDisc>();

	if(!opticalDrive || opticalDisc)
		return;

	opticalDrive->eject();
}

void KPhononCompactDiscPrivate::setVolume(unsigned volume)
{
	if(!producer())
		return;

	/* 1.0 = 100% */
	m_producerWidget->m_output->setVolume(volume * 0.01);
}

void KPhononCompactDiscPrivate::setBalance(unsigned)
{
}

unsigned KPhononCompactDiscPrivate::volume()
{
	if(!producer())
		return 0;

	return (unsigned)(m_producerWidget->m_output->volume() * 100.0);
}

unsigned KPhononCompactDiscPrivate::balance()
{
	return 50;
}

void KPhononCompactDiscPrivate::queryMetadata()
{
	Q_Q(KCompactDisc);

	if(!producer())
		return;

	QMultiMap<QString, QString> data = m_producerWidget->m_media->metaData();
	kDebug() << "METADATA";
	//kDebug() << data;

	m_trackArtists[0] = data.take("ARTIST");
	m_trackTitles[0] = data.take("ALBUM");

	m_trackArtists[m_track] = data.take("ARTIST");
	m_trackTitles[m_track] = data.take("TITLE");

	emit q->discInformation(KCompactDisc::PhononMetadata);
}

KCompactDisc::DiscStatus KPhononCompactDiscPrivate::discStatusTranslate(Phonon::State state)
{
    switch (state) {
    case Phonon::PlayingState:
        return KCompactDisc::Playing;
    case Phonon::PausedState:
        return KCompactDisc::Paused;
    case Phonon::StoppedState:
        return KCompactDisc::Stopped;
    case Phonon::ErrorState:
        return KCompactDisc::NoDisc;
    case Phonon::LoadingState:
    case Phonon::BufferingState:
        return KCompactDisc::NotReady;
    default:
        return KCompactDisc::Error;
    }
}

void KPhononCompactDiscPrivate::tick(qint64 t)
{
    unsigned track;
	Q_Q(KCompactDisc);

	track = m_producerWidget->m_mediaController->currentTitle();
	if(track != m_track) {
		m_track = track;
		m_discLength = trackLength(m_track);
		emit q->playoutTrackChanged(m_track);

		/* phonon gives us Matadata only per Track */
		if(m_autoMetadata)
			queryMetadata();
	}

	m_trackPosition = MS2SEC(t);
	m_discPosition = m_trackPosition;
	// Update the current playing position.
	if(m_seek) {
		kDebug() << "seek: " << m_seek << " trackPosition " << m_trackPosition;
		if(abs(m_trackExpectedPosition - m_trackPosition) > m_seek)
			m_seek = 0;
		else
			m_seek = abs(m_trackExpectedPosition - m_trackPosition);
	}

	if(!m_seek) {
		emit q->playoutPositionChanged(m_trackPosition);
		//emit q->playoutDiscPositionChanged(m_discPosition);
	}
}

void KPhononCompactDiscPrivate::stateChanged(Phonon::State newstate, Phonon::State)
{
    KCompactDisc::DiscStatus status;
	Q_Q(KCompactDisc);

    status = discStatusTranslate(newstate);

    if(m_status != status) {
		if(skipStatusChange(status))
			return;

		m_status = status;

     	switch(m_status) {
		case KCompactDisc::Ejected:
		case KCompactDisc::NoDisc:
			clearDiscInfo();
			break;
		default:
            if(m_tracks == 0) {
				m_tracks = m_producerWidget->m_mediaController->availableTitles();
				if(m_tracks > 0) {
					kDebug() << "New disc with " << m_tracks << " tracks";

					make_playlist();

					m_trackArtists.append(i18n("Unknown Artist"));
					m_trackTitles.append(i18n("Unknown Title"));
					for(unsigned i = 1; i <= m_tracks; i++) {
						m_trackArtists.append(i18n("Unknown Artist"));
						m_trackTitles.append(ki18n("Track %1").subs(i, 2).toString());
					}

					emit q->discChanged(m_tracks);

					if(m_autoMetadata)
						queryMetadata();
				}
            }

			break;
		}
	}
}

#include "phonon_interface.moc"

