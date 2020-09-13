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

#include "audio_phonon.h"
#include "audio.h"

#include <QByteArray>
#include <QDataStream>
#include <QTimer>
#include <QWaitCondition>
#include <QMutex>

#include <phonon/audiooutput.h>
#include <phonon/audiopath.h>
#include <phonon/mediaobject.h>

LibWMPcmPlayer::LibWMPcmPlayer() : AbstractMediaStream(NULL),
    m_media(NULL),
    m_cmd(WM_CDM_UNKNOWN),
    m_blk(NULL)
{
    Phonon::AudioOutput* m_output = new Phonon::AudioOutput(Phonon::MusicCategory, this);
    Phonon::AudioPath* m_path = new Phonon::AudioPath(this);
    m_path->addOutput(m_output);
    m_media = new Phonon::MediaObject(this);
    m_media->addAudioPath(m_path);
    m_media->setCurrentSource(this);
    setStreamSeekable(false);
    setStreamSize(0xffffffff);

    connect(this, SIGNAL(cmdChanged(int)), this, SLOT(executeCmd(int)));
    connect(this, SIGNAL(nextBuffer(cdda_block*)), this, SLOT(playBuffer(cdda_block*)));
    connect(m_media, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
        this, SLOT(stateChanged(Phonon::State,Phonon::State)));

    DEBUGLOG("writeHeader\n");
    writeData( wavHeader() );
    DEBUGLOG("writeHeader end\n");
}

LibWMPcmPlayer::~LibWMPcmPlayer()
{
    stop();
}

QByteArray LibWMPcmPlayer::wavHeader() const
{
    QByteArray data;
    QDataStream stream( &data, QIODevice::WriteOnly );
    stream.setByteOrder( QDataStream::LittleEndian );
    stream
        << 0x46464952 //"RIFF"
        << static_cast<quint32>( 0x7FFFFFFF )
        << 0x45564157 //"WAVE"
        << 0x20746D66 //"fmt "           //Subchunk1ID
        << static_cast<quint32>( 16 )    //Subchunk1Size
        << static_cast<quint16>( 1 )     //AudioFormat
        << static_cast<quint16>( 2 )     //NumChannels
        << static_cast<quint32>( 44100 ) //SampleRate
        << static_cast<quint32>( 2*2*44100 )//ByteRate
        << static_cast<quint16>( 2*2 )   //BlockAlign
        << static_cast<quint16>( 16 )    //BitsPerSample
        << 0x61746164 //"data"                   //Subchunk2ID
        << static_cast<quint32>( 0x7FFFFFFF-36 )//Subchunk2Size
        ;

    return data;
}

void LibWMPcmPlayer::reset()
{
    setStreamSeekable(false);
    setStreamSize(0xffffffff);
    DEBUGLOG("writeHeader\n");
    writeData( wavHeader() );
    DEBUGLOG("writeHeader end\n");
}

void LibWMPcmPlayer::needData()
{
    DEBUGLOG("needData\n");
    m_mutex.lock();
    m_readyToPlay.wakeAll();
    m_mutex.unlock();

}

void LibWMPcmPlayer::setNextBuffer(struct cdda_block *blk)
{
    Q_EMIT nextBuffer(blk);
    m_mutex.lock();
    m_readyToPlay.wait(&m_mutex);
    m_mutex.unlock();
}

void LibWMPcmPlayer::playBuffer(struct cdda_block *blk)
{
    if(m_cmd != WM_CDM_PLAYING) {
        Q_EMIT cmdChanged(WM_CDM_PLAYING);
        m_cmd = WM_CDM_PLAYING;
    }
    writeData(QByteArray(blk->buf, blk->buflen));

}

void LibWMPcmPlayer::pause(void)
{
    if(m_cmd != WM_CDM_PAUSED) {
        Q_EMIT cmdChanged(WM_CDM_PAUSED);
        m_cmd = WM_CDM_PAUSED;

        m_readyToPlay.wakeAll();
    }
}

void LibWMPcmPlayer::stop(void)
{
    if(m_cmd != WM_CDM_STOPPED) {
        Q_EMIT cmdChanged(WM_CDM_STOPPED);
        m_cmd = WM_CDM_STOPPED;

        m_readyToPlay.wakeAll();
    }
}

void LibWMPcmPlayer::executeCmd(int cmd)
{
    switch(cmd) {
    case WM_CDM_PLAYING:
DEBUGLOG("set play\n");
        m_media->play();
        break;
    case WM_CDM_PAUSED:
DEBUGLOG("set pause\n");
        m_media->pause();
        break;
    case WM_CDM_STOPPED:
DEBUGLOG("set stop\n");
        m_media->stop();
        break;
    default:
        cmd = WM_CDM_STOPPED;
        break;
    }
}

void LibWMPcmPlayer::stateChanged( Phonon::State newstate, Phonon::State oldstate )
{
    DEBUGLOG("stateChanged from %i to %i\n", oldstate, newstate);
}

static LibWMPcmPlayer *PhononObject = NULL;

int phonon_open(void)
{
    DEBUGLOG("phonon_open\n");

    if(PhononObject) {
        ERRORLOG("Already initialized!\n");
        return -1;
    }

    PhononObject = new LibWMPcmPlayer();

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
    DEBUGLOG("phonon_play %ld samples, frame %i\n",
        blk->buflen / (2 * 2), blk->frame);

    if(!PhononObject) {
        ERRORLOG("Unable to play\n");
        blk->status = WM_CDM_CDDAERROR;
        return -1;
    }

    PhononObject->setNextBuffer(blk);

    return 0;
}

/*
 * Pause the audio immediately.
 */
int
phonon_pause(void)
{
    DEBUGLOG("phonon_pause\n");

    if(!PhononObject) {
        ERRORLOG("Unable to pause\n");
        return -1;
    }

    PhononObject->pause();

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
    phonon_pause,
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
