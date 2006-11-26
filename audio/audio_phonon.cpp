/*
 *  Driver for Phonon Architecture, Matthias Kretz <kretz@kde.org>
 *
 *  adopted for libworkman cdda audio backend from Alexander Kern alex.kern@gmx.de
 *
 * This file comes under GPL license.
 */



#include <config.h>

#include <QThread>
#include <QByteArray>
#include <QDataStream>

#include <phonon/audiooutput.h>
#include <phonon/audiopath.h>
#include <phonon/bytestream.h>
#include "audio.h"

namespace Phonon { class ByteStream; }

class LibWMPcmPlayer : public QThread {
public:
    LibWMPcmPlayer() : m_stream(NULL), init_complete(0)
    {
    };

    void release()
    {
        if(m_stream)
            delete m_stream;
        m_stream = NULL;
        init_complete = 0;
    };

    ~LibWMPcmPlayer()
    {
        release();
    };

    void run()
    {
    };

    Phonon::ByteStream* m_stream;
    int init_complete;
};

static LibWMPcmPlayer PhononObject;

int phonon_open(void)
{
    DEBUGLOG("phonon_open\n");

    Phonon::AudioOutput* m_output = new Phonon::AudioOutput( Phonon::MusicCategory, &PhononObject);
    Phonon::AudioPath* m_path = new Phonon::AudioPath( &PhononObject );
    m_path->addOutput( m_output );
    PhononObject.m_stream = new Phonon::ByteStream( &PhononObject );
    PhononObject.m_stream->addAudioPath( m_path );
    PhononObject.m_stream->setStreamSeekable( false );
    PhononObject.m_stream->setStreamSize( 0x7FFFFFFF );

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
            << 0x61746164 //"data"              //Subchunk2ID
            << static_cast<quint32>( 0x7FFFFFFF-36 )//Subchunk2Size
            ;
    PhononObject.m_stream->writeData(data);
    PhononObject.init_complete = 1;

  return 0;
}

int phonon_close(void)
{
    DEBUGLOG("phonon_close\n");

    PhononObject.release();
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

    if(!PhononObject.init_complete) {
        if(phonon_open()) {
            ERRORLOG("Unable to phonon_open()\n");
            blk->status = WM_CDM_CDDAERROR;
            return -1;
        }
    }

    PhononObject.m_stream->writeData(QByteArray(blk->buf, blk->buflen));

    return 0;
}

/*
 * Stop the audio immediately.
 */
int
phonon_stop(void)
{
    DEBUGLOG("phonon_stop\n");

    PhononObject.release();
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

    return &phonon_oops;
}
