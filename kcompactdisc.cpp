/*
 *  KCompactDisc - A CD drive interface for the KDE Project.
 *
 *  Copyright (C) 2005 Shaheedur R. Haque <srhaque@iee.org>
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

#include <QFile>
#include <kdebug.h>
#include <klocale.h>
#include <kprotocolmanager.h>
#include <krun.h>
#include "kcompactdisc.h"
#include <netwm.h>
#include <QtDBus>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

/* this is for glibc 2.x which the ust structure in ustat.h not stat.h */
#ifdef __GLIBC__
#include <sys/ustat.h>
#endif

#ifdef __FreeBSD__
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#endif

#ifdef __linux__
#include <mntent.h>
#define KSCDMAGIC 0
#endif

#include <k3process.h>
#include <config.h>

extern "C"
{
// We don't have libWorkMan installed already, so get everything
// from within our own directory
#include "include/wm_cdrom.h"
#include "include/wm_cdtext.h"

// Sun, Ultrix etc. have a canonical CD device specified in the
// respective plat_xxx.c file. On those platforms you need not
// specify the CD device and DEFAULT_CD_DEVICE is not defined
// in config.h.
#ifndef DEFAULT_CD_DEVICE
#define DEFAULT_CD_DEVICE "/dev/cdrom"
#endif
}

#include <qtextcodec.h>
#include <fixx11h.h>

// Our internal definition of when we have no disc. Used to guard some
// internal arrays.
#define NO_DISC ((m_discId == missingDisc) && (m_previousDiscId == 0))

#define FRAMES_TO_MS(frames) \
((frames) * 1000 / 75)

#define TRACK_VALID(track) \
((track) && (track <= m_tracks))

const QString KCompactDisc::defaultDevice = DEFAULT_CD_DEVICE;
const unsigned KCompactDisc::missingDisc = (unsigned)-1;

KCompactDisc::KCompactDisc(InformationMode infoMode) :
    m_device(QString::null),
    m_status(0),
    m_previousStatus(123456),
    m_discId(missingDisc),
    m_previousDiscId(0),
    m_artist(QString::null),
    m_title(QString::null),
    m_track(0),
    m_previousTrack(99999999),
    m_infoMode(infoMode)
{
    // Debug.
    // wm_cd_set_verbosity(WM_MSG_LEVEL_DEBUG | WM_MSG_CLASS_ALL );
    m_trackArtists.clear();
    m_trackTitles.clear();
    m_trackStartFrames.clear();
    connect(&timer, SIGNAL(timeout()), SLOT(timerExpired()));
}

KCompactDisc::~KCompactDisc()
{
    // Ensure nothing else starts happening.
    timer.stop();
    wm_cd_stop();
    wm_cd_set_verbosity(0x0);
    wm_cd_destroy();
}

const QString &KCompactDisc::device() const
{
    return m_device;
}

unsigned KCompactDisc::discLength() const
{
    if (NO_DISC || !m_tracks)
        return 0;
    return FRAMES_TO_MS(m_trackStartFrames[m_tracks] - m_trackStartFrames[0]);
}

unsigned KCompactDisc::discPosition() const
{
    return wm_get_cur_pos_abs() * 1000 - FRAMES_TO_MS(m_trackStartFrames[0]);
}

QString KCompactDisc::discStatus(int status)
{
    QString message;

    switch (status)
    {
    case WM_CDM_TRACK_DONE: // == WM_CDM_BACK
        message = i18n("Back/Track Done");
        break;
    case WM_CDM_PLAYING:
        message = i18n("Playing");
        break;
    case WM_CDM_FORWARD:
        message = i18n("Forward");
        break;
    case WM_CDM_PAUSED:
        message = i18n("Paused");
        break;
    case WM_CDM_STOPPED:
        message = i18n("Stopped");
        break;
    case WM_CDM_EJECTED:
        message = i18n("Ejected");
        break;
    case WM_CDM_NO_DISC:
        message = i18n("No Disc");
        break;
    case WM_CDM_UNKNOWN:
        message = i18n("Unknown");
        break;
    case WM_CDM_CDDAERROR:
        message = i18n("CDDA Error");
        break;
    case WM_CDM_CDDAACK:
        message = i18n("CDDA Ack");
        break;
    default:
        if (status <= 0)
            message = strerror(-status);
        else
            message = QString::number(status);
        break;
    }
    return message;
}

/**
 * Do everything needed if the user requested to eject the disc.
 */
void KCompactDisc::eject()
{
    if (m_status == WM_CDM_EJECTED)
    {
        emit trayClosing();
        wm_cd_closetray();
    }
    else
    {
        wm_cd_stop();
        wm_cd_eject();
    }
}

unsigned KCompactDisc::track() const
{
    return m_track;
}

bool KCompactDisc::isPaused() const
{
    return (m_status == WM_CDM_PAUSED);
}

bool KCompactDisc::isPlaying() const
{
    return WM_CDS_DISC_PLAYING(m_status) && (m_status != WM_CDM_PAUSED) && (m_status != WM_CDM_TRACK_DONE);
}

void KCompactDisc::pause()
{
    // wm_cd_pause "does the right thing" by flipping between pause and resume.
    wm_cd_pause();
}

void KCompactDisc::play(unsigned startTrack, unsigned startTrackPosition, unsigned endTrack)
{
    kDebug() << " startTrack " << startTrack << " startTrackPosition " << startTrackPosition << " endTrack " << endTrack << endl;
    wm_cd_play(TRACK_VALID(startTrack) ? startTrack : 1, startTrackPosition / 1000, TRACK_VALID(endTrack) ? endTrack : WM_ENDTRACK );
}

QString KCompactDisc::urlToDevice(const QString& device)
{
    KUrl deviceUrl = KUrl::fromPathOrUrl(device);
    if (deviceUrl.protocol() == "media" || deviceUrl.protocol() == "system")
    {
        kDebug() << "Asking mediamanager for " << deviceUrl.fileName() << endl;

        QDBusInterface mediamanager( "org.kde.kded", "/modules/mediamanager", "org.kde.MediaManager" );
        QDBusReply<QStringList> reply = mediamanager.call("properties",deviceUrl.fileName());

        QStringList properties = reply;
        if (!reply.isValid() || properties.count() < 6)
        {
            kError() << "Invalid reply from mediamanager" << endl;
            return defaultDevice;
        }
        else
        {
            kDebug() << "Reply from mediamanager " << properties[5] << endl;
            return properties[5];
        }
    }

    return device;
}

bool KCompactDisc::setDevice(
    const QString &device_,
    unsigned volume,
    bool digitalPlayback,
    const QString &audioSystem,
    const QString &audioDevice)
{
    timer.stop();

    QString device = urlToDevice(device_);

    wm_cd_set_verbosity(9);
    int status = wm_cd_init(
                    digitalPlayback ? WM_CDDA : WM_CDIN,
                    QFile::encodeName(device),
                    digitalPlayback ? audioSystem.toAscii().data() : 0,
                    digitalPlayback ? audioDevice.toAscii().data() : 0,
                    0);
    m_device = wm_drive_device();
    kDebug() << "Device change: "
        << (digitalPlayback ? "WM_CDDA, " : "WM_CDIN, ")
        << m_device << ", "
        << (digitalPlayback ? audioSystem : QString::null) << ", "
        << (digitalPlayback ? audioDevice : QString::null) << ", status: "
        << discStatus(status) << endl;

    if (status < 0)
    {
        // Severe (OS-level) error.
        m_device.clear();
    }
    else
    {
        // Init CD-ROM and display.
        setVolume(volume);
    }

    m_previousStatus = m_status = wm_cd_status();

    if (m_infoMode == Asynchronous)
        timerExpired();
    else
    {
        timer.setSingleShot(true);
        timer.start(1000);
    }
    return !m_device.isNull();
}

void KCompactDisc::setVolume(unsigned volume)
{
    int status = wm_cd_volume(volume, WM_BALANCE_SYMMETRED);
    kDebug() << "Volume change: " << volume << ", status: " << discStatus(status) << endl;
}

void KCompactDisc::stop()
{
    wm_cd_stop();
}

QString KCompactDisc::trackArtist() const
{
    return trackArtist(m_track);
}

QString KCompactDisc::trackArtist(unsigned track) const
{
    if (NO_DISC || !TRACK_VALID(track))
        return QString();
    return m_trackArtists[track - 1];
}

unsigned KCompactDisc::trackLength() const
{
    return trackLength(m_track);
}

unsigned KCompactDisc::trackLength(unsigned track) const
{
    if (NO_DISC || !TRACK_VALID(track))
        return 0;
    return wm_cd_getref()->trk[track - 1].length * 1000;
}

unsigned KCompactDisc::trackPosition() const
{
    return wm_get_cur_pos_rel() * 1000;
}

unsigned KCompactDisc::tracks() const
{
    return m_tracks;
}

QString KCompactDisc::trackTitle() const
{
    return trackTitle(m_track);
}

QString KCompactDisc::trackTitle(unsigned track) const
{
    if (NO_DISC || !TRACK_VALID(track))
        return QString();
    return m_trackTitles[track - 1];
}

bool KCompactDisc::isAudio(unsigned track) const
{
    if (NO_DISC || !TRACK_VALID(track))
        return 0;
    return !(wm_cd_getref()->trk[track - 1].data);
}

/*
 * timerExpired
 *
 * - Data discs not recognized as data discs.
 *
 */
void KCompactDisc::timerExpired()
{
    m_status = wm_cd_status();

    if (WM_CDS_NO_DISC(m_status) || m_device.isNull())
    {
        if (m_previousStatus != m_status)
        {
            m_previousStatus = m_status;
            m_discId = missingDisc;
            m_previousDiscId = 0;
            m_trackArtists.clear();
            m_trackTitles.clear();
            m_trackStartFrames.clear();
            m_tracks = 0;
            m_track = 0;
            emit discChanged(m_discId);
        }
    }
    else
    {
        m_discId = cddb_discid();
        if (m_previousDiscId != m_discId)
        {
            m_previousDiscId = m_discId;
            kDebug() << "New discId=" << m_discId << endl;
            // Initialise the album and its signature from the CD.
            struct cdtext_info *info = wm_cd_get_cdtext();
            if (info && info->valid)
            {
                m_artist = reinterpret_cast<char*>(info->blocks[0]->performer[0]);
                m_title = reinterpret_cast<char*>(info->blocks[0]->name[0]);
            }
            else
            {
                m_artist = i18n("Unknown Artist");
                m_title = i18n("Unknown Title");
            }

            // Read or default CD data.
            m_trackArtists.clear();
            m_trackTitles.clear();
            m_trackStartFrames.clear();
            m_tracks = wm_cd_getcountoftracks();
            for (unsigned i = 1; i <= m_tracks; i++)
            {
                if (info && info->valid)
                {
                    m_trackArtists.append(reinterpret_cast<char*>(info->blocks[0]->performer[i]));
                    m_trackTitles.append(reinterpret_cast<char*>(info->blocks[0]->name[i]));
                }
                else
                {
                    m_trackArtists.append(i18n("Unknown Artist"));
                    m_trackTitles.append(ki18n("Track %1").subs(i, 2).toString());
                }
                // FIXME: KDE4
                // track.length = wm_cd_getref()->trk[i - 1].length;
                m_trackStartFrames.append(wm_cd_getref()->trk[i - 1].start);
            }
            m_trackStartFrames.append(wm_cd_getref()->trk[m_tracks].start);
            emit discChanged(m_discId);
        }

        // Per-event processing.
        m_track = wm_cd_getcurtrack();
        if (m_previousTrack != m_track)
        {
            m_previousTrack = m_track;

            // Update the current track and its length.
            emit trackChanged(m_track, trackLength());
        }
        if (isPlaying())
        {
            m_previousStatus = m_status;
            // Update the current playing position.
            emit trackPlaying(m_track, trackPosition());
        }
        else
        if (m_previousStatus != m_status)
        {
            // If we are not playing, then we are either paused, or stopped.
            switch (m_status)
            {
            case WM_CDM_PAUSED:
                emit trackPaused(m_track, trackPosition());
                break;
            case WM_CDM_EJECTED:
                emit trayOpening();
                break;
            default:
                if ((m_previousStatus == WM_CDM_PLAYING || m_previousStatus == WM_CDM_PAUSED)
                      && m_status == WM_CDM_STOPPED)
                {
                emit discStopped();
                }
                break;
            }

            m_previousStatus = m_status;
        }
    }

    // Now that we have incurred any delays caused by the signals, we'll start the timer.
    timer.setSingleShot(true);
    timer.start(1000);
}

#include "kcompactdisc.moc"
