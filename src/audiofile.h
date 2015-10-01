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

#ifndef AUDIOFILE_H
#define AUDIOFILE_H

#include <QIODevice>
#include <cdio/cdio.h>
#include <kcompactdisc_export.h>

namespace KCompactDisc {

class KCOMPACTDISC_EXPORT AudioFile : public QIODevice
{
    Q_OBJECT

    public:

    enum DataMode {
        RawData = 0x01,
        WaveFile = 0x02
    };

    explicit AudioFile(CdIo_t *cdioDevice, quint16 trackNo, DataMode dataMode = RawData, QObject *parent = 0);
    virtual ~AudioFile();

    bool open(OpenMode mode) Q_DECL_OVERRIDE;
    bool atEnd() const Q_DECL_OVERRIDE;
    bool isSequential() const Q_DECL_OVERRIDE;
    bool canReadLine() const Q_DECL_OVERRIDE;
    bool reset() Q_DECL_OVERRIDE;
    bool seek(qint64 pos) Q_DECL_OVERRIDE;
    qint64 size() const Q_DECL_OVERRIDE;

    protected:

    qint64 readData(char *data, qint64 maxlen) Q_DECL_OVERRIDE;
    qint64 writeData(const char *data, qint64 len) Q_DECL_OVERRIDE;

    private:

    CdIo_t *mCdioDevice;
    DataMode mDataMode;
    quint16 mTrackNo;
    quint64 mSectorsPerRead;

    lsn_t mLsnStart;
    lsn_t mLsnCurrent;
    lsn_t mLsnEnd;
    quint32 mDataSize;
    qint64 mCurrentOffset;

    struct WavHeader {
        char    RiffChunkId[4];
        quint32 RiffChunkSize;
        char    RiffChunkFormat[4];

        char    FormatChunkId[4];
        quint32 FormatChunkSize;

        quint16 AudioFormat;
        quint16 NumChannels;
        quint32 SampleRate;
        quint32 ByteRate;
        quint16 BlockAlign;
        quint16 BitsPerSample;

        char    DataChunkId[4];
        quint32 DataChunkSize;
    } mWavHeader;
};

} // namespace KCompactDisc

#endif // AUDIOFILE_H
