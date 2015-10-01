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

#ifndef DISC_H
#define DISC_H

#include <QObject>
#include "track.h"
#include "kcompactdisc_export.h"

namespace KCompactDisc {

class KCOMPACTDISC_EXPORT Disc : public QObject
{
    Q_OBJECT

    public:

    explicit Disc(const QUrl &deviceNode, QObject *parent = 0);
    virtual ~Disc();

    QString albumArtist();
    QString albumTitle();

    quint8 numTracks();
    QList<Track> tracks();
    Track track(quint8 trackNo);

    private:

    class DiscPrivate;
    DiscPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Disc)
};

} // namespace KCompactDisc

#endif // DISC_H
