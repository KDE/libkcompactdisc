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

#include "audiofile.h"
#include <cstring>

#include <QtGlobal>
#include <QtEndian>

using namespace KCompactDisc;

AudioFile::AudioFile(CdIo_t *cdioDevice, quint16 trackNo, DataMode dataMode, QObject *parent) :
    QIODevice(parent),
    mCdioDevice(cdioDevice),
    mDataMode(dataMode),
    mTrackNo(trackNo),
    mSectorsPerRead(64),
    mLsnStart(0),
    mLsnCurrent(0),
    mLsnEnd(0),
    mDataSize(0),
    mCurrentOffset(0)
{}

AudioFile::~AudioFile()
{}

bool AudioFile::open(OpenMode mode)
{
    // we can only open in read-only mode
    if (mode != QIODevice::ReadOnly) {
        return false;
    }

    // get the logical sector numbers for the track start
    // and end
    mLsnStart = cdio_get_track_lsn(mCdioDevice, mTrackNo);
    mLsnEnd = cdio_get_track_last_lsn(mCdioDevice, mTrackNo);

    if (mLsnStart == CDIO_INVALID_LSN || mLsnEnd == CDIO_INVALID_LSN || mLsnEnd < mLsnStart) {
        return false;
    }
    mLsnCurrent = mLsnStart;
    mDataSize = quint32((mLsnEnd - mLsnStart) * CDIO_CD_FRAMESIZE_RAW);

    // populate the wave file header if we're presenting a
    // wave file
    if (mDataMode == WaveFile) {
        memcpy(mWavHeader.RiffChunkId, "RIFF", 4);
        memcpy(mWavHeader.RiffChunkFormat, "WAVE", 4);
        memcpy(mWavHeader.FormatChunkId, "fmt ", 4);
        memcpy(mWavHeader.DataChunkId, "data", 4);

        mWavHeader.FormatChunkSize = qToLittleEndian(16);
        mWavHeader.AudioFormat     = qToLittleEndian(1);
        mWavHeader.NumChannels     = qToLittleEndian(2);
        mWavHeader.SampleRate      = qToLittleEndian(44100);
        mWavHeader.ByteRate        = qToLittleEndian(176400);
        mWavHeader.BlockAlign      = qToLittleEndian(4);
        mWavHeader.BitsPerSample   = qToLittleEndian(16);

        mWavHeader.DataChunkSize   = qToLittleEndian(mDataSize);
        if (mDataSize % 2) {
            mWavHeader.RiffChunkSize = qToLittleEndian(mDataSize + 37);
        } else {
            mWavHeader.RiffChunkSize = qToLittleEndian(mDataSize + 36);
        }
    }

    // done
    return QIODevice::open(mode);
}

bool AudioFile::atEnd() const
{
    switch (mDataMode) {
    case RawData:
        return (mLsnCurrent >= mLsnEnd);
    case WaveFile:
        return (mCurrentOffset >= mDataSize + 44 + (mDataSize % 2));
    }

    return true;
}

bool AudioFile::isSequential() const
{
    return false;
}

bool AudioFile::canReadLine() const
{
    return false;
}

bool AudioFile::seek(qint64 pos)
{
    if (!QIODevice::seek(pos)) {
        return false;
    }

    mCurrentOffset = pos;
    if (mDataMode == WaveFile) {
        pos = qMin((pos - 44), qint64(0));
    }

    lsn_t lsnPos = (pos / CDIO_CD_FRAMESIZE_RAW);
    mLsnCurrent = mLsnStart + lsnPos;

    return true;
}

bool AudioFile::reset()
{
    return seek(0);
}

qint64 AudioFile::size() const
{
    switch(mDataMode) {
    case RawData:
        return qint64(mDataSize);
    case WaveFile:
        if (mDataSize % 2) {
            return qint64(mDataSize + 45);
        }
        return quint64(mDataSize + 44);
    }

    return 0;
}

qint64 AudioFile::readData(char *data, qint64 maxlen)
{
    // if we're in wave file mode, are we in the wav header section? if
    // yes, we'll read the wave header into the buffer first
    if ((mDataMode == WaveFile) && (mCurrentOffset < 44)) {
        qint64 headerLeft = 44 - mCurrentOffset;
        qint64 toWrite = qMin(headerLeft, maxlen);

        memcpy(data, &mWavHeader, toWrite);
        mCurrentOffset += toWrite;

        maxlen -= toWrite;
        data += toWrite;
    }

    // if we're in wave file mode and we need to generate a padding byte, give them that
    if ((mDataMode == WaveFile) && (mLsnCurrent == mLsnEnd) && (mDataSize % 2)) {
        data[0] = 0;
        ++mCurrentOffset;

        return 1;
    }

    // start by finding how many sectors we can read, max of 64 at once
    quint64 sectorCount = qMin((quint64)(maxlen / CDIO_CD_FRAMESIZE_RAW), mSectorsPerRead);

    // do we even have that many sectors left to read?
    if ((mLsnCurrent + (lsn_t)sectorCount) > mLsnEnd) {
        sectorCount = mLsnEnd - mLsnCurrent;
    }

    // do the reading. progressively halve the sectors per read to 8
    // in case of read errors
    do {
        int ret = cdio_read_audio_sectors(mCdioDevice, data, mLsnCurrent, sectorCount);
        if (ret != DRIVER_OP_SUCCESS) {
            mSectorsPerRead /= 2;
            if (mSectorsPerRead < 8) {
                return -1;
            }

            sectorCount = qMin(sectorCount, mSectorsPerRead);
            continue;
        }
        break;
    } while (true);

    mLsnCurrent += sectorCount;
    mCurrentOffset += (sectorCount * CDIO_CD_FRAMESIZE_RAW);
    return (sectorCount * CDIO_CD_FRAMESIZE_RAW);
}

qint64 AudioFile::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return -1;
}
