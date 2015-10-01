#include <QObject>
#include <QDebug>
#include <QMetaObject>
#include <QCoreApplication>
#include <QtGlobal>
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

#include <KCompactDisc/Disc>
#include <KCompactDisc/Track>
#include <KCompactDisc/AudioFile>
#include <KCompactDisc/KCompactDisc>

#include <cdio/cdio.h>

class CDRipper : public QObject
{
    Q_OBJECT

    public:

    explicit CDRipper(QObject *parent = 0) :
        QObject(parent)
    {}

    public slots:

    void doRip()
    {
        KCompactDisc::Disc disc(KCompactDisc::defaultOpticalDriveNode(), this);

        qDebug() << "Disc is" << disc.albumTitle() << "by" << disc.albumArtist();
        qDebug() << "The disc has" << disc.numTracks() << "tracks";
        qDebug() << "Ripping:";

        for (auto track: disc.tracks()) {
            qDebug() << track.trackNo() << ":" << track.artist() << "-" << track.title();

            KCompactDisc::AudioFile *audioFile = track.audioFile(KCompactDisc::AudioFile::WaveFile);

            QString fileName = QString::number(track.trackNo()) + " - " + track.artist() + " - " + track.title() + ".wav";
            QDir saveDir(QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
            saveDir.mkpath(disc.albumArtist() + " - " + disc.albumTitle());
            saveDir.cd(disc.albumArtist() + " - " + disc.albumTitle());
            QFile destFile(saveDir.filePath(fileName));

            destFile.open(QFile::ReadWrite);
            audioFile->open(KCompactDisc::AudioFile::ReadOnly);

            char buf[512];
            while(!audioFile->atEnd()) {
                int read = audioFile->read(buf, 512);
                destFile.write(buf, read);
            }

            audioFile->close();
            destFile.close();
            audioFile->deleteLater();
        }

        qDebug() << "Ripping complete";
        qApp->exit();
    }
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    qDebug() << "CD Ripper Sample/Test";
    CDRipper ripper;
    QMetaObject::invokeMethod(&ripper, "doRip", Qt::QueuedConnection);

    return app.exec();
}

#include "ripdisc.moc"
