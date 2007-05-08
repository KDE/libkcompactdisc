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

#include <QByteArray>
#include <QTimer>
#include <QWaitCondition>
#include <QMutex>

#include <Phonon/AbstractMediaStream>

namespace Phonon { class MediaObject; }

class LibWMPcmPlayer : public Phonon::AbstractMediaStream {
    Q_OBJECT

public:
    LibWMPcmPlayer();
    ~LibWMPcmPlayer();

    QByteArray wavHeader() const;
    void setNextBuffer(struct cdda_block *blk);

public slots:
    void play(void);
    void pause(void);
    void stop(void);
    void moreData(void);
    void executeCmd(int cmd);
    void stateChanged( Phonon::State newstate, Phonon::State oldstate );

protected:
    void needData();
    void enoughData();

signals:
    void cmdChanged(int cmd);

private:
    Phonon::MediaObject* m_media;
    unsigned char m_cmd;
    struct cdda_block *m_blk;
    QWaitCondition m_readyToPlay;
    QMutex m_mutex;
};

#endif /* __AUDIO_PHONON_H__ */
