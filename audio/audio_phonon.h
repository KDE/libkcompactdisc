/*  This file is part of the KDE project
    Copyright (C) 2006 Alexander Kern <alex.kern@gmx.de>

    based on example for Phonon Architecture, Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef __AUDIO_PHONON_H__
#define __AUDIO_PHONON_H__

#include <QThread>
#include <QByteArray>
#include <QDataStream>
#include <QWaitCondition>
#include <QMutex>

#include <phonon/audiooutput.h>
#include <phonon/audiopath.h>
#include <phonon/bytestream.h>

namespace Phonon { class ByteStream; }

class LibWMPcmPlayer : public QThread {
    Q_OBJECT

public:
    LibWMPcmPlayer();
    ~LibWMPcmPlayer();

    void stop() { m_stream->stop(); }
    void setNextBuffer(struct cdda_block *new_blk);
    virtual void run(void);

public slots:
    void playNextBuffer();

private:
    Phonon::ByteStream* m_stream;
    struct cdda_block *blk;
    QWaitCondition bufferPlayed;
    QMutex mutex;
};

#endif /* __AUDIO_PHONON_H__ */
