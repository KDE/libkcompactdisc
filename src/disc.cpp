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

#include "disc.h"

#include <QUrl>
#include <QDebug>
#include <QFile>

#include <KLocalizedString>

#include <cdio/cdio.h>
#include <cdio/cdtext.h>

using namespace KCompactDisc;

class Disc::DiscPrivate
{
    public:

    CdIo_t *mCdioDevice;
    cdtext_t *mCdTextData;
    QUrl mDeviceNode;
    quint8 mNumTracks;
    QList<Track> mTracks;
    QString mAlbumArtist;
    QString mAlbumTitle;
};

Disc::Disc(const QUrl &deviceNode, QObject *parent) :
    QObject(parent),
    d_ptr(new DiscPrivate)
{
    Q_D(Disc);

    // initialize the cdio interface
    d->mCdioDevice = cdio_open(deviceNode.toLocalFile().toAscii().constData(), DRIVER_UNKNOWN);
    if (!d->mCdioDevice) {
        qFatal("failed to open cdio device");
    }
    d->mDeviceNode = deviceNode;
    d->mCdTextData = cdio_get_cdtext(d->mCdioDevice);

    // fill up the list of tracks
    d->mTracks.clear();
    d->mNumTracks = 0;

    d->mNumTracks = cdio_get_num_tracks(d->mCdioDevice);
    for (int i = 1; i <= d->mNumTracks; ++i) {
        d->mTracks.append(Track(d->mCdioDevice, d->mCdTextData, i));
    }

    if (d->mCdTextData) {
        d->mAlbumArtist = QString(cdtext_get_const(d->mCdTextData, CDTEXT_FIELD_PERFORMER, 0)).trimmed();
        d->mAlbumTitle = QString(cdtext_get_const(d->mCdTextData, CDTEXT_FIELD_TITLE, 0)).trimmed();
    } else {
        d->mAlbumArtist = i18n("Unknown Artist");
        d->mAlbumTitle = i18n("Unknown Album");
    }

    // todo: calculate cddb discid and musicbrainz discid
}

Disc::~Disc()
{
    Q_D(Disc);
    if (d->mCdTextData) {
        cdtext_destroy(d->mCdTextData);
    }

    if (d->mCdioDevice) {
        cdio_destroy(d->mCdioDevice);
    }

    d->mTracks.clear();
    delete d_ptr;
}

QString Disc::albumArtist()
{
    Q_D(Disc);
    return d->mAlbumArtist;
}

QString Disc::albumTitle()
{
    Q_D(Disc);
    return d->mAlbumTitle;
}

quint8 Disc::numTracks()
{
    Q_D(Disc);
    return d->mNumTracks;
}

QList<Track> Disc::tracks()
{
    Q_D(Disc);
    return d->mTracks;
}
