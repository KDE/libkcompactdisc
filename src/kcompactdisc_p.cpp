#include "kcompactdisc_p.h"

#include <KLocalizedString>
#include <KRandomSequence>

KCompactDiscPrivate::KCompactDiscPrivate(const QString &deviceNode, QObject *parent) :
    QObject(parent),
    mCdioDev(nullptr)
{
    // reset everything
    resetDevice();

    // init the device
    setDevice(deviceNode);
}

KCompactDiscPrivate::~KCompactDiscPrivate()
{
    if (mCdioDev) {
        cdio_destroy(mCdioDev);
    }
}

// static functions

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

quint64 KCompactDiscPrivate::sumDigits(quint64 num)
{
    quint64 ret = 0;

    while (true) {
        ret += num % 10;
        num /= 10;
        if (!num) {
            return ret;
        }
    }
}

// device control

bool KCompactDiscPrivate::setDevice(const QString &deviceNode)
{
    // start by initing the cdio device
    mCdioDev = cdio_open(deviceNode.toAscii().constData(), DRIVER_UNKNOWN);
    if (!mCdioDev) {
        mCdioDev = nullptr;
        return false;
    }
    mDeviceNode = deviceNode;

    // fill up the device information
    cdio_hwinfo_t hwinfo;
    if (cdio_get_hwinfo(mCdioDev, &hwinfo)) {
        m_deviceModel = QString(hwinfo.psz_model).trimmed();
        m_deviceVendor = QString(hwinfo.psz_vendor).trimmed();
        m_deviceRevision = QString(hwinfo.psz_revision).trimmed();

        m_deviceName = '[' + m_deviceVendor + " - " + m_deviceModel + " - " + m_deviceRevision + ']';
    }

    // if we have a valid audio cd in, set metadata
    if (cdio_get_discmode(mCdioDev) == CDIO_DISC_MODE_CD_DA) {
        m_status = m_statusExpected = KCompactDisc::Stopped;
        queryMetadata();
    }

    return true;
}

// track information

void KCompactDiscPrivate::queryMetadata()
{
    // start with the basics - the number of tracks
    m_tracks = cdio_get_num_tracks(mCdioDev);

    // and the cdtext artist and title
    cdtext_t *cdTextData = cdio_get_cdtext(mCdioDev);
    if (cdTextData == NULL) {
        m_albumArtist = i18n("Unknown Artist");
        m_albumTitle = i18n("Unknown Album");

        for (int i = 0; i < m_tracks; ++i) {
            m_trackArtists.append(i18n("Unknown Artist"));
            m_trackTitles.append(i18n("Unknown Title"));
        }
        return;
    }

    // cdtext for track 0 contains the album title
    // and artist
    m_albumArtist = QString(cdtext_get_const(cdTextData, CDTEXT_FIELD_PERFORMER, 0)).trimmed();
    m_albumTitle = QString(cdtext_get_const(cdTextData, CDTEXT_FIELD_TITLE, 0)).trimmed();

    for (int i = 1; i <= m_tracks; ++i) {
        m_trackTitles.append(QString(cdtext_get_const(cdTextData, CDTEXT_FIELD_TITLE, i)).trimmed());
        m_trackArtists.append(QString(cdtext_get_const(cdTextData, CDTEXT_FIELD_PERFORMER, i)).trimmed());
    }

    // we're done with cdtext
    cdtext_destroy(cdTextData);

    // we have one thing left to do now, and that is
    // calculate the cddb discid for the disc.

    quint64 msfSum = 0;
    msf_t msfStart;
    msf_t msfEnd;

    for (int i = 1; i <= m_tracks; ++i) {
        cdio_get_track_msf(mCdioDev, i, &msfEnd);
        msfSum += sumDigits(cdio_audio_get_msf_seconds(&msfEnd));
    }

    cdio_get_track_msf(mCdioDev, 1, &msfStart);
    cdio_get_track_msf(mCdioDev, CDIO_CDROM_LEADOUT_TRACK, &msfEnd);
    quint64 msfDiff = cdio_audio_get_msf_seconds(&msfEnd) - cdio_audio_get_msf_seconds(&msfStart);

    m_discId = ((msfSum % 0xff) << 24 | msfDiff << 8 | m_tracks);

    // done
}

quint64 KCompactDiscPrivate::trackLength(quint8 trackNo) const
{
    if (trackNo < 1 || trackNo > m_tracks) {
        return 0;
    }

    msf_t trackMsf;
    cdio_get_track_msf(mCdioDev, trackNo, &trackMsf);
    return cdio_audio_get_msf_seconds(&trackMsf);
}

bool KCompactDiscPrivate::isTrackAudio(quint8 trackNo) const
{
    if (trackNo < 1 || trackNo > m_tracks) {
        return false;
    }
    return (cdio_get_track_format(mCdioDev, trackNo) == TRACK_FORMAT_AUDIO);
}

// playback control

void KCompactDiscPrivate::playTrackPosition(quint8 trackNo, quint64 trackOffset) {}
void KCompactDiscPrivate::pause() {}
void KCompactDiscPrivate::stop() {}

void KCompactDiscPrivate::eject()
{
    if (m_status == KCompactDisc::Playing || m_status == KCompactDisc::Paused) {
        stop();
    }

    resetMetadata();
    driver_return_code_t ret = cdio_eject_media(&mCdioDev);
    if (ret == DRIVER_OP_SUCCESS) {
        resetDevice();
    }
}

void KCompactDiscPrivate::closeTray()
{
    driver_id_t driver = DRIVER_UNKNOWN;
    driver_return_code_t ret = cdio_close_tray(mDeviceNode.toAscii().constData(), &driver);
    if (ret == DRIVER_OP_SUCCESS) {
        queryMetadata();
    }
}

void KCompactDiscPrivate::setVolume(quint8 volume) {}
void KCompactDiscPrivate::setBalance(quint8 balance) {}

// playlist control

void KCompactDiscPrivate::makePlaylist()
{
    m_playlist.clear();

    for (quint16 i = 1; i <= m_tracks; ++i) {
        m_playlist.append(i);
    }

    if (m_randomPlaylist) {
        KRandomSequence sequence;
        sequence.randomize(m_playlist);
    }
}

quint8 KCompactDiscPrivate::getPrevTrackInPlaylist()
{
    if (m_playlist.empty()) {
        return 0;
    }

    int currentIndex = m_playlist.indexOf(m_track);
    if (currentIndex < 0) {
        return 0;
    } else if (currentIndex > 0) {
        return m_playlist[currentIndex - 1];
    } else {
        if (m_loopPlaylist) {
            return m_playlist.last();
        }
        return 0;
    }
}

quint8 KCompactDiscPrivate::getNextTrackInPlaylist()
{
    if (m_playlist.empty()) {
        return 0;
    }

    int currentIndex = m_playlist.indexOf(m_track);
    if (currentIndex < 0) {
        return 0;
    } else if (currentIndex < (m_playlist.size() - 1)) {
        return m_playlist[currentIndex + 1];
    } else {
        if (m_loopPlaylist) {
            return m_playlist.first();
        }
        return 0;
    }
}

// private

void KCompactDiscPrivate::resetMetadata()
{
    m_status = KCompactDisc::NoDisc;
    m_statusExpected = KCompactDisc::NoDisc;

    m_discId = 0;
    m_discLength = 0;
    m_track = 0;
    m_tracks = 0;
    m_trackPosition = 0;
    m_trackExpectedPosition = 0;
    m_discPosition = 0;
    m_seek = 0;

    m_trackStartFrames.clear();
    m_albumTitle = QString();
    m_albumArtist = QString();
    m_trackArtists.clear();
    m_trackTitles.clear();

    m_playlist.clear();
    m_loopPlaylist = false;
    m_randomPlaylist = false;
    m_autoMetadata = true;
}

void KCompactDiscPrivate::resetDevice()
{
    if (mCdioDev) {
        cdio_destroy(mCdioDev);
        mCdioDev = nullptr;
    }
    mDeviceNode = QString();

    m_deviceVendor = QString();
    m_deviceModel = QString();
    m_deviceRevision = QString();
    m_deviceName = QString();

    resetMetadata();
}
