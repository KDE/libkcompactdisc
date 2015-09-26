#ifndef KCOMPACTDISCPRIVATE_H
#define KCOMPACTDISCPRIVATE_H

#include <QObject>
#include <QString>
#include <QList>

#include <cdio/cdio.h>
#include <cdio/audio.h>

#include "kcompactdisc.h"

class KCompactDiscPrivate : public QObject
{
    Q_OBJECT

    public:

    explicit KCompactDiscPrivate(const QString &deviceNode, QObject *parent = 0);
    virtual ~KCompactDiscPrivate();

    // methods, starting with the static ones

    static const QString discStatusI18n(KCompactDisc::DiscStatus);

    // device control

    bool setDevice(const QString &deviceNode);

    // track information

    void queryMetadata();
    quint64 trackLength(quint8 trackNo) const;
    bool isTrackAudio(quint8 trackNo) const;

    // playback control

    void playTrackPosition(quint8 trackNo, quint64 trackOffset);
    void pause();
    void stop();
    void eject();
    void closeTray();
    void setVolume(quint8 volume);
    void setBalance(quint8 balance);

    // playlist control

    void makePlaylist();
    quint8 getPrevTrackInPlaylist();
    quint8 getNextTrackInPlaylist();

    // variables, starting with enums

    KCompactDisc::DiscStatus m_status;
    KCompactDisc::DiscStatus m_statusExpected;

    // drive information

    QString m_deviceVendor;
    QString m_deviceModel;
    QString m_deviceRevision;
    QString m_deviceName;

    // disc information

    QList<quint64> m_trackStartFrames;
    QString m_albumTitle;
    QString m_albumArtist;
    QStringList m_trackArtists;
    QStringList m_trackTitles;

    quint32 m_discId;
    quint64 m_discLength;
    quint8  m_track;
    quint8  m_tracks;
    quint64 m_trackPosition;
    quint64 m_trackExpectedPosition;
    quint64 m_discPosition;
    qint64  m_seek;

    // playlist control

    QList<quint16> m_playlist;
    bool m_loopPlaylist;
    bool m_randomPlaylist;

    // metadata control

    bool m_autoMetadata;

    private:

    static quint64 sumDigits(quint64 num);

    void resetMetadata();
    void resetDevice();

    QString mDeviceNode;
    CdIo_t *mCdioDev;
};

#endif // KCOMPACTDISCPRIVATE_H
