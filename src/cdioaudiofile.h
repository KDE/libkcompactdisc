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

#ifndef CDIOAUDIOFILE_H
#define CDIOAUDIOFILE_H

#include <QIODevice>
#include <cdio/cdio.h>

class CdioAudioFile : public QIODevice
{
    Q_OBJECT

    public:

    explicit CdioAudioFile(CdIo_t *cdioDevice, quint16 trackNo, QObject *parent = 0);
    virtual ~CdioAudioFile();

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
    quint16 mTrackNo;
    quint64 mSectorsPerRead;
    lsn_t mLsnStart;
    lsn_t mLsnCurrent;
    lsn_t mLsnEnd;
};

#endif // CDIOAUDIOFILE_H
