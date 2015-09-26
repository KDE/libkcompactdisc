#include <QObject>
#include <QDebug>
#include <QMetaObject>
#include <QCoreApplication>
#include <QtGlobal>

#include <KCompactDisc/KCompactDisc>

#include <cdio/cdio.h>

class TestKCD : public QObject
{
    Q_OBJECT

    public:

    explicit TestKCD(QObject *parent = 0) :
        QObject(parent),
        mKcd(new KCompactDisc)
    {}

    virtual ~TestKCD()
    {
        mKcd->deleteLater();
    }

    public slots:

    void doTest()
    {
        qDebug() << "Starting test";
        mKcd->setDevice(mKcd->defaultCdromDeviceName(), 50, true, "alsa");
        qDebug() << "";

        qDebug() << "We have" << mKcd->audioSystems().size() << "audo systems available:";
        for (auto system: mKcd->audioSystems()) {
            qDebug() << system;
        }
        qDebug() << "";

        qDebug() << "We have" << mKcd->cdromDeviceNames().size() << "cdrom drives available:";
        for (auto cdrom: mKcd->cdromDeviceNames()) {
            qDebug() << cdrom;
        }
        qDebug() << "";

        qDebug() << "The current cdrom drive loaded is:" << mKcd->deviceName();
        qDebug() << "The disc device node url is:" << mKcd->deviceUrl();
        qDebug() << "The disc status is" << mKcd->discStatus();
        qDebug() << "Does the drive have a disc in it:" << !mKcd->isNoDisc();
        qDebug() << "The number of tracks in the disc:" << mKcd->tracks();
        qDebug() << "The current track no:" << mKcd->trackPosition();

        qDebug() << "";
        qDebug() << "Running tests with Cdio";
        doCdioTest();

        qDebug() << "Here";
        mKcd->next();
        qDebug() << "Here";

       // qApp->exit();
    }

    void doCdioTest()
    {
        CdIo_t *_cd_dev = cdio_open(mKcd->deviceUrl().toLocalFile().toUtf8().constData(), DRIVER_UNKNOWN);
        if (!_cd_dev) {
            qDebug() << "Failed to open CdIo device";
            return;
        }

        discmode_t mode = cdio_get_discmode(_cd_dev);
        qDebug() << mode;
        qDebug() << discmode2str[cdio_get_discmode(_cd_dev)];

        cdio_destroy(_cd_dev);
    }

    private:

    KCompactDisc *mKcd;
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    qDebug() << "Testing libKF5CompactDisc";
    TestKCD test;
    QMetaObject::invokeMethod(&test, "doTest", Qt::QueuedConnection);

    return app.exec();
}

#include "testkcd.moc"
