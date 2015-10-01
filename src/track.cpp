/*
 *  Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "track.h"
#include <KLocalizedString>

using namespace KCompactDisc;

class Track::TrackPrivate {
    public:

    CdIo_t   *mCdioDevice;
    cdtext_t *mCdTextData;
    quint8    mTrackNo;
};

Track::Track(CdIo_t *cdioDevice, cdtext_t *cdTextData, quint8 trackNo) :
    d_ptr(new TrackPrivate)
{
    Q_D(Track);

    d->mCdioDevice = cdioDevice;
    d->mCdTextData = cdTextData;
    d->mTrackNo = trackNo;
}

Track::Track(const Track &other) :
    d_ptr(new TrackPrivate)
{
    Q_D(Track);

    d->mCdioDevice = other.d_func()->mCdioDevice;
    d->mCdTextData = other.d_func()->mCdTextData;
    d->mTrackNo = other.d_func()->mTrackNo;
}

Track::~Track()
{
    delete d_ptr;
}

// track information and access to data

quint8 Track::trackNo()
{
    Q_D(Track);
    return d->mTrackNo;
}

quint32 Track::length()
{
    Q_D(Track);

    msf_t trackMsf;
    cdio_get_track_msf(d->mCdioDevice, d->mTrackNo, &trackMsf);
    return cdio_audio_get_msf_seconds(&trackMsf);
}

AudioFile *Track::audioFile(AudioFile::DataMode dataMode)
{
    Q_D(Track);
    return new AudioFile(d->mCdioDevice, d->mTrackNo, dataMode);
}

// cdtext primary data

QString Track::title()
{
    Q_D(Track);

    if (!d->mCdTextData) {
        return i18n("Track %1", QString::number(d->mTrackNo));
    }

    QString title = cdtext_get_const(d->mCdTextData, CDTEXT_FIELD_TITLE, d->mTrackNo);
    if (title.isEmpty() || title.isNull()) {
        return i18n("Track %1", QString::number(d->mTrackNo));
    }
    return title;
}

QString Track::artist()
{
    Q_D(Track);

    if (!d->mCdTextData) {
        return i18n("Unknown Artist");
    }

    QString artist = cdtext_get_const(d->mCdTextData, CDTEXT_FIELD_PERFORMER, d->mTrackNo);
    if (artist.isEmpty() || artist.isNull()) {
        return i18n("Unknown Artist");
    }
    return artist;
}

// the rest don't need to have default data, so just return whatever we get

QString Track::songwriter()
{
    Q_D(Track);
    return cdtext_get_const(d->mCdTextData, CDTEXT_FIELD_SONGWRITER, d->mTrackNo);
}

QString Track::composer()
{
    Q_D(Track);
    return cdtext_get_const(d->mCdTextData, CDTEXT_FIELD_COMPOSER, d->mTrackNo);
}

QString Track::arranger()
{
    Q_D(Track);
    return cdtext_get_const(d->mCdTextData, CDTEXT_FIELD_ARRANGER, d->mTrackNo);
}

QString Track::genre()
{
    Q_D(Track);
    return cdtext_get_const(d->mCdTextData, CDTEXT_FIELD_GENRE, d->mTrackNo);
}

QString Track::cdtextMessage()
{
    Q_D(Track);
    return cdtext_get_const(d->mCdTextData, CDTEXT_FIELD_MESSAGE, d->mTrackNo);
}

QString Track::upcEan()
{
    Q_D(Track);
    return cdtext_get_const(d->mCdTextData, CDTEXT_FIELD_UPC_EAN, d->mTrackNo);
}

QString Track::isrc()
{
    Q_D(Track);
    return cdtext_get_const(d->mCdTextData, CDTEXT_FIELD_ISRC, d->mTrackNo);
}
