#include <QObject>
#include <QDebug>
#include <QMetaObject>
#include <QCoreApplication>
#include <QtGlobal>

#include <kcompactdisc.h>

class TestKCD : public QObject
{
    Q_OBJECT

    public:

    explicit TestKCD(QObject *parent = nullptr) :
        QObject(parent),
        mKcd(new KCompactDisc(KCompactDisc::Asynchronous))
    {}

    ~TestKCD() override
    {
        mKcd->deleteLater();
    }

    public Q_SLOTS:

    void doTest()
    {
        qDebug() << "Starting test";
        mKcd->setDevice(mKcd->defaultCdromDeviceName(), 50, true, "phonon");
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

        qApp->exit();
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
