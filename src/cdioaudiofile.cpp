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

#include "cdioaudiofile.h"

#include <QtGlobal>
#include <QDebug>

CdioAudioFile::CdioAudioFile(CdIo_t *cdioDevice, quint16 trackNo, QObject *parent) :
    QIODevice(parent),
    mCdioDevice(cdioDevice),
    mTrackNo(trackNo),
    mSectorsPerRead(64),
    mLsnStart(0),
    mLsnCurrent(0),
    mLsnEnd(0)
{}

CdioAudioFile::~CdioAudioFile()
{}

bool CdioAudioFile::open(OpenMode mode)
{
    if (mode != QIODevice::ReadOnly) {
        return false;
    }

    mLsnStart = cdio_get_track_lsn(mCdioDevice, mTrackNo);
    mLsnEnd = cdio_get_track_last_lsn(mCdioDevice, mTrackNo);

    if (mLsnStart == CDIO_INVALID_LSN || mLsnEnd == CDIO_INVALID_LSN) {
        return false;
    }
    mLsnCurrent = mLsnStart;

    qDebug() << mLsnStart << mLsnCurrent << mLsnEnd;

    return QIODevice::open(mode);
}

bool CdioAudioFile::atEnd() const
{
    return (mLsnCurrent >= mLsnEnd);
}

bool CdioAudioFile::isSequential() const
{
    return false;
}

bool CdioAudioFile::canReadLine() const
{
    return false;
}

bool CdioAudioFile::seek(qint64 pos)
{
    if (!QIODevice::seek(pos)) {
        return false;
    }

    lsn_t lsnPos = pos / CDIO_CD_FRAMESIZE_RAW;
    mLsnCurrent = mLsnStart + lsnPos;

    return true;
}

bool CdioAudioFile::reset()
{
    return seek(0);
}

qint64 CdioAudioFile::size() const
{
    return (qint64)((mLsnCurrent - mLsnStart) * CDIO_CD_FRAMESIZE_RAW);
}

qint64 CdioAudioFile::readData(char *data, qint64 maxlen)
{
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
    return (sectorCount * CDIO_CD_FRAMESIZE_RAW);
}

qint64 CdioAudioFile::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return -1;
}
