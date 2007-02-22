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

#include <config.h>

#include <QThread>
#include <QByteArray>
#include <QDataStream>
#include <QWaitCondition>
#include <QMutex>

#include <phonon/audiooutput.h>
#include <phonon/audiopath.h>
#include <phonon/bytestream.h>
#include "audio.h"
#include "audio_phonon.h"

LibWMPcmPlayer::LibWMPcmPlayer()
{
    DEBUGLOG("LibWMPcmPlayer\n");

    m_stream = NULL;
    blk = NULL;

    Phonon::AudioOutput* m_output = new Phonon::AudioOutput(Phonon::MusicCategory, this);
    Phonon::AudioPath* m_path = new Phonon::AudioPath(this);
    m_path->addOutput(m_output);
    m_stream = new Phonon::ByteStream(this);
    m_stream->addAudioPath(m_path);
    m_stream->setStreamSeekable(false);
    m_stream->setStreamSize(0x7FFFFFFF);

    connect(m_stream, SIGNAL(needData()), this, SLOT(playNextBuffer()));

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream
        << 0x46464952 //"RIFF"
        << static_cast<quint32>(0x7FFFFFFF)
        << 0x45564157 //"WAVE"
        << 0x20746D66 //"fmt "           //Subchunk1ID
        << static_cast<quint32>(16)    //Subchunk1Size
        << static_cast<quint16>(1)     //AudioFormat
        << static_cast<quint16>(2)     //NumChannels
        << static_cast<quint32>(44100 ) //SampleRate
        << static_cast<quint32>(2*2*44100)//ByteRate
        << static_cast<quint16>(2*2)   //BlockAlign
        << static_cast<quint16>(16)    //BitsPerSample
        << 0x61746164 //"data"              //Subchunk2ID
        << static_cast<quint32>(0x7FFFFFFF-36)//Subchunk2Size
        ;
    m_stream->writeData(data);

    DEBUGLOG("LibWMPcmPlayer end\n");
}

LibWMPcmPlayer::~LibWMPcmPlayer()
{
    DEBUGLOG("~LibWMPcmPlayer\n");

    stop();
    delete m_stream;
    m_stream = NULL;
    blk = NULL;
}

void LibWMPcmPlayer::run(void)
{
}

void LibWMPcmPlayer::setNextBuffer(struct cdda_block *new_blk)
{
    DEBUGLOG("setNextBuffer\n");
    mutex.lock();
    DEBUGLOG("setNextBuffer a\n");
    blk = new_blk;
    DEBUGLOG("setNextBuffer b\n");
    bufferPlayed.wait(&mutex);
    DEBUGLOG("setNextBuffer c\n");
    mutex.unlock();
    DEBUGLOG("setNextBuffer end\n");
}

void LibWMPcmPlayer::playNextBuffer()
{
    DEBUGLOG("playNextBuffer\n");
    mutex.lock();

    if(blk)
        m_stream->writeData(QByteArray(blk->buf, blk->buflen));
    blk = NULL;

    bufferPlayed.wakeAll();

    mutex.unlock();
    DEBUGLOG("playNextBuffer end\n");
}

static LibWMPcmPlayer *PhononObject = NULL;

int phonon_open(void)
{
    DEBUGLOG("phonon_open\n");

    if(PhononObject) {
        ERRORLOG("Allready initialized!\n");
        return -1;
    }

    PhononObject = new LibWMPcmPlayer;

    return 0;
}

int phonon_close(void)
{
    DEBUGLOG("phonon_close\n");

    if(!PhononObject) {
        ERRORLOG("Unable to close\n");
        return -1;
    }

    delete PhononObject;

    PhononObject = NULL;

    return 0;
}

/*
 * Play some audio and pass a status message upstream, if applicable.
 * Returns 0 on success.
 */
int
phonon_play(struct cdda_block *blk)
{
    DEBUGLOG("phonon_play %ld frames, %ld bytes\n", blk->buflen / (2 * 2), blk->buflen);

    if(!PhononObject) {
        ERRORLOG("Unable to play\n");
        blk->status = WM_CDM_CDDAERROR;
        return -1;
    }

    PhononObject->setNextBuffer(blk);

    return 0;
}

/*
 * Stop the audio immediately.
 */
int
phonon_stop(void)
{
    DEBUGLOG("phonon_stop\n");

    if(!PhononObject) {
        ERRORLOG("Unable to stop\n");
        return -1;
    }

    PhononObject->stop();

    return 0;
}

/*
 * Get the current audio state.
 */
int
phonon_state(struct cdda_block *blk)
{
    DEBUGLOG("phonon_state\n");

    return -1; /* not implemented yet for PHONON */
}

static struct audio_oops phonon_oops = {
    phonon_open,
    phonon_close,
    phonon_play,
    phonon_stop,
    phonon_state,
    NULL,
    NULL
};

extern "C" struct audio_oops*
setup_phonon(const char *dev, const char *ctl)
{
    DEBUGLOG("setup_phonon\n");

    phonon_open();

    return &phonon_oops;
}

#include "audio_phonon.moc"
