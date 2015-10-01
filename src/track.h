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

#ifndef TRACK_H
#define TRACK_H

#include <QtGlobal>
#include <QString>

#include <cdio/cdio.h>
#include <cdio/cdtext.h>
#include <cdio/audio.h>

#include "audiofile.h"
#include "kcompactdisc_export.h"

namespace KCompactDisc {

class KCOMPACTDISC_EXPORT Track
{
    public:

    Track(CdIo_t *cdioDevice, cdtext_t *cdTextData, quint8 trackNo);
    Track(const Track &other);
    ~Track();

    // track information
    quint8 trackNo();
    quint32 length();

    // cdtext data
    QString title();
    QString artist();
    QString songwriter();
    QString composer();
    QString arranger();
    QString genre();

    // cdtext extra data
    QString cdtextMessage();
    QString upcEan();
    QString isrc();

    // audio data
    AudioFile *audioFile(AudioFile::DataMode dataMode);

    private:

    class TrackPrivate;
    TrackPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Track)
};

} // namespace KCompactDisc

#endif // TRACK_H
