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

#ifndef KCOMPACTDISC_H
#define KCOMPACTDISC_H

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QTimer>

#include "kcompactdisc_export.h"

class KCompactDiscPrivate;

/**
 *  KCompactDisc - A CD drive interface for the KDE Project.
 *
 *  The disc interface is modelled by these slots:
 *
 * @see #playoutTrack(unsigned track): Play specified track.
 * @see #playoutPosition(unsigned position): seek to specified position.
 * @see #playNext(): Play next track in the playlist.
 * @see #playPrev(): Play previous track in the playlist.
 * @see #pauseResumePlayout(): Toggle between pause/resume.
 * @see #stopPlayout(): Stop playout:
 * @see #eject(): Stop playout and eject disc or Close
 *                        tray and try to read TOC from disc.
 *
 *  The progress of playout is modelled by these signals:
 *
 * @see #playoutPositionChanged(unsigned position): A position in a track.
 * @see #playoutTrackChanged(unsigned track): A playout of this track is started.
 *
 *
 *  The shape of playlist is controlled by these accessors.
 *
 * @see #setRandomPlaylist(bool): Shuffle the playlist.
 * @see #setLoopPlaylist(bool): Couple begin and end of playlist.
 *
 *
 *  The disc lifecycle is modelled by these signals:
 *
 * @see #discChanged(...): A new disc was inserted.
 * @see #discStatusChanged(KCompactDisc::Playing): A disc started playout.
 * @see #discStatusChanged(KCompactDisc::Paused): A disc was paused.
 * @see #discStatusChanged(KCompactDisc::Stopped): The disc stopped.
 * @see #discStatusChanged(KCompactDisc::NoDisc): The disc is removed. No disc in tray or data disc.
 * @see #discStatusChanged(KCompactDisc::NotReady): The disc is present. But playout is not possible.
 * @see #discInformation(QStringList info): A content for disc information is arrived.
 *
 *
 *  The volume control is modelled by these slots:
 *
 * @see #setVolume(unsigned): A new volume value.
 * @see #setBalance(unsigned): A new balance value.
 *
 *
 *  And these signals:
 *
 * @see #volumeChanged(unsigned): A current volume value.
 * @see #balanceChanged(unsigned): A current balance value.
 *
 *
 *  All times in this interface are in seconds. Valid track numbers are
 *  positive numbers; zero is not a valid track number.
 */
class KCOMPACTDISC_EXPORT KCompactDisc : public QObject
{
    Q_OBJECT
/*
    Q_CLASSINFO("D-Bus Interface", "org.kde.KSCD")

public Q_SLOTS:
    Q_SCRIPTABLE bool playing();
    Q_SCRIPTABLE void play() { play(); }
    Q_SCRIPTABLE void stop() { stop(); }
    Q_SCRIPTABLE void previous() { prev(); }
    Q_SCRIPTABLE void next() { next(); }
    Q_SCRIPTABLE void jumpTo(int seconds) { jumpToTime(seconds); }
    Q_SCRIPTABLE void eject() { eject(); }
    Q_SCRIPTABLE void toggleLoop() { loop(); }
    Q_SCRIPTABLE void toggleShuffle() { random(); }
    Q_SCRIPTABLE void toggleTimeDisplay() { cycleplaytimemode(); }
    Q_SCRIPTABLE void cddbDialog() { CDDialogSelected(); }
    Q_SCRIPTABLE void optionDialog() { showConfig(); }
    Q_SCRIPTABLE void setTrack(int t) { trackSelected(t > 0 ? t - 1 : 0); }
    Q_SCRIPTABLE void volumeDown() { decVolume(); }
    Q_SCRIPTABLE void volumeUp() { incVolume(); }
    Q_SCRIPTABLE void setVolume(int v);
    Q_SCRIPTABLE void setDevice(const QString& dev);
    Q_SCRIPTABLE int  getVolume() { return Prefs::volume(); }
    Q_SCRIPTABLE int currentTrack();
    Q_SCRIPTABLE int currentTrackLength();
    Q_SCRIPTABLE int currentPosition();
    Q_SCRIPTABLE int getStatus();
    Q_SCRIPTABLE QString currentTrackTitle();
    Q_SCRIPTABLE QString currentAlbum();
    Q_SCRIPTABLE QString currentArtist();
    Q_SCRIPTABLE QStringList trackList();
*/
public:
    enum InformationMode
    {
        Synchronous, // Return and emit signal when cdrom and cddb information arrives.
        Asynchronous // Block until cdrom and cddb infromation has been obtained
    };

	enum DiscCommand
	{
		Play,
		Pause,
		Next,
		Prev,
		Stop,
		Eject,
		Loop,
		Random
	};
	
    enum DiscStatus
    {
        Playing,
        Paused,
        Stopped,
        Ejected,
        NoDisc,
        NotReady,
        Error
    };

    enum DiscInfo
    {
        Cdtext,
        Cddb,
        PhononMetadata
    };

    KCompactDisc(InformationMode = KCompactDisc::Synchronous);
    virtual ~KCompactDisc();

    /**
     * @param device Name of CD device, e.g. /dev/cdrom.
     * @param digitalPlayback Select digital or analog playback.
     * @param audioSystem For digital playback, system to use, e.g. "phonon".
     * @param audioDevice For digital playback, device to use.
     * @return true if the device seemed usable.
     */
    bool setDevice(
        const QString &device,
        unsigned volume = 50,
        bool digitalPlayback = true,
        const QString &audioSystem = QString(),
        const QString &audioDevice = QString());

    /**
     * If the url is a media:/ or system:/ URL returns
     * the device it represents, otherwise returns device
     */
    static QString urlToDevice(const QUrl& url);

    /**
     * All installed audio backends.
     */
    static const QStringList audioSystems();

    /**
     * All present CDROM devices.
     */
    static const QStringList cdromDeviceNames();

    /**
     * The default CDROM device for this system.
     */
    static const QString defaultCdromDeviceName();

    /**
     * The Url of default CDROM device for this system.
     */
    static const QUrl defaultCdromDeviceUrl();

    /**
     * The Url of named CDROM device for this system.
     */
    static const QUrl cdromDeviceUrl(const QString &);

    /**
     * The Udi of default CDROM device for this system.
     */
	static const QString defaultCdromDeviceUdi();

    /**
     * The Udi of named CDROM device for this system.
     */
	static const QString cdromDeviceUdi(const QString &);

    /**
     * SCSI parameter VENDOR of current CDROM device.
     *
     * @return Null string if no usable device set.
     */
    const QString &deviceVendor();

    /**
     * SCSI parameter MODEL of current CDROM device.
     *
     * @return Null string if no usable device set.
     */
    const QString &deviceModel();

    /**
     * SCSI parameter REVISION of current CDROM device.
     *
     * @return Null string if no usable device set.
     */
    const QString &deviceRevision();

    /**
     * Current CDROM device.
     *
     * @return Null string if no usable device set.
     */
    const QString &deviceName();

    /**
     * Current device as QUrl.
     */
    const QUrl deviceUrl();

    /**
     * Current disc, 0 if no disc or impossible to calculate id.
     */
    unsigned discId();

    /**
     * CDDB signature of disc, empty if no disc or not possible to deliever.
     */
    const QList<unsigned> &discSignature();

    /**
     * Artist for whole disc.
     *
     * @return Disc artist or null string.
     */
    const QString &discArtist();

    /**
     * Title of disc.
     *
     * @return Disc title or null string.
     */
    const QString &discTitle();

    /**
     * Known length of disc.
     *
     * @return Disc length in seconds.
     */
    unsigned discLength();

    /**
     * Current position on the disc.
     *
     * @return Position in seconds.
     */
    unsigned discPosition();

    /**
     * Current status.
     *
     * @return Current status.
     */
    KCompactDisc::DiscStatus discStatus();

    /**
     * Status as string.
     *
     * @return Status as QString.
     */
    QString discStatusString(KCompactDisc::DiscStatus status);

    /**
     * Artist of current track.
     *
     * @return Track artist or null string.
     */
    QString trackArtist();

    /**
     * Artist of given track.
     *
     * @return Track artist or null string.
     */
    QString trackArtist(unsigned track);

    /**
     * Title of current track.
     *
     * @return Track title or null string.
     */
    QString trackTitle();

    /**
     * Title of given track.
     *
     * @return Track title or null string.
     */
    QString trackTitle(unsigned track);

    /**
     * Length of current track.
     *
     * @return Track length in seconds.
     */
    unsigned trackLength();

    /**
     * Length of given track.
     *
     * @param track Track number.
     * @return Track length in seconds.
     */
    unsigned trackLength(unsigned track);

    /**
     * Current track.
     *
     * @return Track number.
     */
    unsigned track();

    /**
     * Current track position.
     *
     * @return Track position in seconds.
     */
    unsigned trackPosition();

    /**
     * Number of tracks.
     */
    unsigned tracks();

    /**
     * Is status playing.
     */
    bool isPlaying();

    /**
     * Is status pausing.
     */
    bool isPaused();

    /**
     * Is status no disc.
     */
    bool isNoDisc();

    /**
     * @return if the track is actually an audio track.
     */
    bool isAudio(unsigned track);


public Q_SLOTS:

    /**
     * Start playout of track.
     */
    void playTrack(unsigned int track);

    /**
     * Start playout or seek to given position of track.
     */
    void playPosition(unsigned int position);

    /* GUI bindings */
    /**
     * Start playout.
     */
	void play();

    /**
     * Start playout of next track.
     */
    void next();

    /**
     * Start playout of previous track.
     */
    void prev();

    /**
     * Pause/resume playout.
     */
    void pause();

    /**
     * Stop playout.
     */
    void stop();

    /**
     * Open/close tray.
     */
    void eject();

    /**
     * Switch endless playout on/off.
     */
	void loop();

    /**
     * Switch random playout on/off.
     */
    void random();

    /**
     * Pipe GUI command.
     */
	void doCommand(KCompactDisc::DiscCommand);


	void metadataLookup();


Q_SIGNALS:
    /**
     * A new position in a track. This signal is delivered at
     * approximately 1 second intervals while a track is playing. At first sight,
     * this might seem overzealous, but it is likely that any CD player UI will use
     * this to track the second-by-second position, so we may as well do it for
     * them.
     *
     * @param position Position within track in seconds.
     */
    void playoutPositionChanged(unsigned int position);

    /**
     * A new track is started.
     *
     * @param track Track number.
     */
    void playoutTrackChanged(unsigned int track);


public Q_SLOTS:

    void setRandomPlaylist(bool);
    void setLoopPlaylist(bool);
	void setAutoMetadataLookup(bool);


Q_SIGNALS:

	void randomPlaylistChanged(bool);
    void loopPlaylistChanged(bool);


Q_SIGNALS:

    /**
     * A new Disc is inserted
     *
     */
    void discChanged(unsigned int tracks);

    /**
     * A new Disc information is arrived
     *
     */
    void discInformation(KCompactDisc::DiscInfo info);

    /**
     * A Disc status changed
     *
     */
    void discStatusChanged(KCompactDisc::DiscStatus status);


public Q_SLOTS:

    /**
     * Set volume
     */
    void setVolume(unsigned int volume);

    /**
     * Set balance
     */
    void setBalance(unsigned int balance);

Q_SIGNALS:

    /**
     * New volume
     */
    void volumeChanged(unsigned int volume);

    /**
     * New balance
     */
    void balanceChanged(unsigned int balance);


protected:
    KCompactDiscPrivate * d_ptr;
    KCompactDisc(KCompactDiscPrivate &dd, QObject *parent);

private:
    Q_DECLARE_PRIVATE(KCompactDisc)
#ifdef USE_WMLIB
	friend class KWMLibCompactDiscPrivate;
#endif
	friend class KPhononCompactDiscPrivate;
};

#endif
