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

#ifndef KCOMPACTDISC_H
#define KCOMPACTDISC_H

#include <QObject>
#include <QTimer>
#include <QtCore/QStringList>
//#include <q3valuelist.h>


#include <kdemacros.h>

#if defined Q_OS_WIN

#ifndef KCOMPACTDISC_EXPORT
# ifdef MAKE_KCOMPACTDISC_LIB
#  define KCOMPACTDISC_EXPORT KDE_EXPORT
# else
#  define KCOMPACTDISC_EXPORT KDE_IMPORT
# endif
#endif

#else /* UNIX */

/* export statements for unix */
#define KCOMPACTDISC_EXPORT KDE_EXPORT
#endif


/**
 *  KCompactDisc - A CD drive interface for the KDE Project.
 *
 *  The disc lifecycle is modelled by these signals:
 *
 * @see #trayClosing(): A disc is being inserted.
 * @see #discChanged(): A disc was inserted or removed.
 * @see #trayOpening(): A disc is being removed.
 *
 *  The progress of playout is modelled by these signals:
 *
 * @see #trackPlaying(): A track started playing, or is still playing.
 * @see #trackPaused(): A track was paused.
 * @see #discStopped(): The disc stopped.
 *
 *  All times in this interface are in milliseconds. Valid track numbers are
 *  positive numbers; zero is not a valid track number.
 */
class KCOMPACTDISC_EXPORT KCompactDisc :
    public QObject
{
    Q_OBJECT
public:
    enum InformationMode
    {
        Synchronous, // Return and emit signal when cdrom and cddb information arrives.
        Asynchronous // Block until cdrom and cddb infromation has been obtained
    };

    KCompactDisc(InformationMode=Synchronous);
    virtual ~KCompactDisc();

    /**
     * Open/close tray.
     */
    void eject();

    /**
     * Start playout at given position of track.
     */
    void play(unsigned startTrack = 0, unsigned startTrackPosition = 0, unsigned endTrack = 0);

    /**
     * Pause/resume playout.
     */
    void pause();

    /**
     * @param device Name of CD device, e.g. /dev/cdrom.
     * @param digitalPlayback Select digial or analog playback.
     * @param audioSystem For analog playback, system to use, e.g. "arts".
     * @param audioDevice For analog playback, device to use.
     * @return true if the device seemed usable.
     */
    bool setDevice(
        const QString &device,
        unsigned volume = 50,
        bool digitalPlayback = true,
        const QString &audioSystem = QString::null,
        const QString &audioDevice = QString::null);

    void setVolume(unsigned volume);

    /**
     * Stop playout.
     */
    void stop();

    /**
     * If the url is a media:/ or system:/ URL returns
     * the device it represents, otherwise returns device
     */
    static QString urlToDevice(const KUrl& url);

    /**
     * All installed audio backends.
     */
    static const QStringList audioSystems();

    /**
     * All present CDROM devices.
     */
    static const QStringList deviceNames();

    /**
     * The default CDROM device for this system.
     */
    static const QString defaultDevice();

    /**
     * The Url of default CDROM device for this system.
     */
    static const KUrl defaultDeviceUrl();

    /**
     * SCSI parameter VENDOR of current CDROM device.
     *
     * @return Null string if no usable device set.
     */
    const QString deviceVendor() const;

    /**
     * SCSI parameter MODEL of current CDROM device.
     *
     * @return Null string if no usable device set.
     */
    const QString deviceModel() const;

    /**
     * SCSI parameter REVISION of current CDROM device.
     *
     * @return Null string if no usable device set.
     */
    const QString deviceRevision() const;

    /**
     * Current CDROM device.
     *
     * @return Null string if no usable device set.
     */
    const QString &deviceName() const;

    /**
     * Current CDROM device as URL.
     *
     * @return Null string if no usable device set.
     */
    const KUrl &deviceUrl() const;

    /**
     * Current device as path.
     *
     * @return Null string if no usable device set.
     */
    const QString &devicePath() const;

    /**
     * The discId for a missing disc.
     */
    static const unsigned missingDisc;

    /**
     * Current disc, missingDisc if no disc.
     */
    unsigned discId() const { return m_discId; }

    /**
     * CDDB signature of disc.
     */
    const QList<unsigned> &discSignature() const { return m_trackStartFrames; }

    /**
     * Artist for whole disc.
     *
     * @return Disc artist or null string.
     */
    const QString &discArtist() const { return m_artist; }

    /**
     * Title of disc.
     *
     * @return Disc title or null string.
     */
    const QString &discTitle() const { return m_title; }

    /**
     * Length of disc.
     *
     * @return Disc length in milliseconds.
     */
    unsigned discLength() const;

    /**
     * Position in disc.
     *
     * @return Position in milliseconds.
     */
    unsigned discPosition() const;
    /**
     * Artist of current track.
     *
     * @return Track artist or null string.
     */
    QString trackArtist() const;

    /**
     * Artist of given track.
     *
     * @return Track artist or null string.
     */
    QString trackArtist(unsigned track) const;

    /**
     * Title of current track.
     *
     * @return Track title or null string.
     */
    QString trackTitle() const;

    /**
     * Title of given track.
     *
     * @return Track title or null string.
     */
    QString trackTitle(unsigned track) const;

    /**
     * Current track.
     *
     * @return Track number.
     */
    unsigned track() const;

    /**
     * Number of tracks.
     */
    unsigned tracks() const;

    /**
     * @return if the track is actually an audio track.
     */
    bool isAudio(unsigned track) const;

    /**
     * Length of current track.
     *
     * @return Track length in milliseconds.
     */
    unsigned trackLength() const;

    /**
     * Length of given track.
     *
     * @param track Track number.
     * @return Track length in milliseconds.
     */
    unsigned trackLength(unsigned track) const;

    /**
     * Position in current track.
     *
     * @return Position in milliseconds.
     */
    unsigned trackPosition() const;

    bool isPaused() const;

    bool isPlaying() const;

Q_SIGNALS:

    /**
     * A disc is being inserted.
     */
    void trayClosing();

    /**
     * A disc is being removed.
     */
    void trayOpening();

    /**
     * A disc was inserted or removed.
     *
     * @param discId Current disc, missingDisc if no disc.
     */
    void discChanged(unsigned discId);

    /**
     * Disc stopped. See @see #trackPaused.
     */
    void discStopped();

    /**
     * The current track changed.
     *
     * @param track Track number.
     * @param trackLength Length within track in milliseconds.
     */
    void trackChanged(unsigned track, unsigned trackLength);

    /**
     * A track started playing, or is still playing. This signal is delivered at
     * approximately 1 second intervals while a track is playing. At first sight,
     * this might seem overzealous, but its likely that any CD player UI will use
     * this to track the second-by-second position, so we may as well do it for
     * them.
     *
     * @param track Track number.
     * @param trackPosition Position within track in milliseconds.
     */
    void trackPlaying(unsigned track, unsigned trackPosition);

    /**
     * A track paused playing.
     *
     * @param track Track number.
     * @param trackPosition Position within track in milliseconds.
     */
    void trackPaused(unsigned track, unsigned trackPosition);

private:
    QTimer timer;
    unsigned m_tracks;
    QList<unsigned> m_trackStartFrames;
    QStringList m_trackArtists;
    QStringList m_trackTitles;

    QString m_deviceName;
    KUrl m_deviceUrl;
    QString m_devicePath;
    int m_status;
    int m_previousStatus;
    unsigned m_discId;
    unsigned m_previousDiscId;
    QString m_artist;
    QString m_title;
    unsigned m_track;
    unsigned m_previousTrack;
    InformationMode m_infoMode;

    void checkDeviceStatus();
    QString discStatus(int status);

    class KCompactDiscPrivate *d;

private Q_SLOTS:
    void timerExpired();
};

#endif
