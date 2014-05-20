#include "getservers.h"
#include <QtNetwork>
#include "downloadfile.h"
#include <QTextStream>
#include <QApplication>

extern QWidget *mmm;

GetServers::GetServers(QObject *parent)
    : QThread(parent), bExistData_(false), bErrorDuringDownload_(false)
{
    QString appPath = QApplication::applicationDirPath();

#if defined Q_OS_WIN
    QString savedCsvPath = appPath + "/Data/netlist.csv";
    QString savedOVPNPath = appPath + "/Data/netlist.ovpn";
#elif defined Q_OS_MAC
    QString savedCsvPath = appPath + "/../Resources/Bin/netlist.csv";
    QString savedOVPNPath = appPath + "/../Resources/Bin/netlist.ovpn";
#elif defined Q_OS_UNIX
    QString savedCsvPath = appPath + "/Data/netlist.csv";
    QString savedOVPNPath = appPath + "/Data/netlist.ovpn";
#endif
    loadDataFromFiles(savedCsvPath, savedOVPNPath);

}

void GetServers::loadDataFromFiles(QString csvPath, QString OVPNPath)
{
    servers_.clear();
    configOVPN_.clear();

    QFile savedCsvFile(csvPath);
    QFile savedOVPNFile(OVPNPath);
    if (savedCsvFile.open(QIODevice::ReadOnly))
    {
        QString dataStr = savedCsvFile.readAll();
        QStringList lines = dataStr.split('\r');
        for (int i = 1; i < lines.count(); i++)
        {
            QString curLine = lines[i];
            QStringList values = curLine.split(';');
            if (values.count() == 14)
            {
                ServerInfo si;
                si.description_ = values[0].trimmed();
                si.ip_ = values[1].trimmed();
                si.pptp_ = values[2].trimmed().contains("Yes", Qt::CaseInsensitive);
                si.l2tp_ = values[3].trimmed().contains("Yes", Qt::CaseInsensitive);
                si.openvpn_ = values[5].trimmed().contains("Yes", Qt::CaseInsensitive);
                si.l2tpKey_ = values[6];

                QString dns1 = values[8].trimmed();
                QString dns2 = values[9].trimmed();
                QString dns3 = values[10].trimmed();

                if (!dns1.isEmpty())
                    si.dns_ << dns1;
                if (!dns2.isEmpty())
                    si.dns_ << dns2;
                if (!dns3.isEmpty())
                    si.dns_ << dns3;

                si.ovpnUrl_ = values[11];
                servers_ << si;
            }
        }

        if (savedOVPNFile.open(QIODevice::ReadOnly))
        {
            configOVPN_ = savedOVPNFile.readAll();
            bExistData_ = true;
            savedOVPNFile.close();
        }

        savedCsvFile.close();
    }
}


QVector<ServerInfo> GetServers::servers()
{
    QMutexLocker locker(&lock_);
    if (bExistData_)
        return servers_;
}

QByteArray GetServers::configOVPN()
{
    QMutexLocker locker(&lock_);
    if (bExistData_)
        return configOVPN_;
}


void GetServers::run()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager();
	QEventLoop eventLoop;
	eventLoop.connect(manager, SIGNAL(finished(QNetworkReply *)), SLOT(quit()));

    QString strRequest = "https://s3.amazonaws.com/glbls-us/server-list/netlist.csv";

	QUrl url(strRequest);
	QNetworkReply *reply = manager->get(QNetworkRequest(url));
	eventLoop.exec();

	if (reply->error() != QNetworkReply::NoError)
	{
        bErrorDuringDownload_ = true;
		errorStr_ = reply->errorString();
        emit updateFinished(false, errorStr_, false);
		return;
	}
	
    QVector<ServerInfo> downloadServers;
    QByteArray downloadConfigOVPN;

    QByteArray replyBytes = reply->readAll();
    QString replyStr = replyBytes;
	QStringList lines = replyStr.split('\r');
    for (int i = 1; i < lines.count(); i++)
	{
		QString curLine = lines[i];
		QStringList values = curLine.split(';');
        if (values.count() == 14)
		{
			ServerInfo si;
			si.description_ = values[0].trimmed();
			si.ip_ = values[1].trimmed();
			si.pptp_ = values[2].trimmed().contains("Yes", Qt::CaseInsensitive);
			si.l2tp_ = values[3].trimmed().contains("Yes", Qt::CaseInsensitive);
			si.openvpn_ = values[5].trimmed().contains("Yes", Qt::CaseInsensitive);
			si.l2tpKey_ = values[6];

            QString dns1 = values[8].trimmed();
            QString dns2 = values[9].trimmed();
            QString dns3 = values[10].trimmed();

            if (!dns1.isEmpty())
                si.dns_ << dns1;
            if (!dns2.isEmpty())
                si.dns_ << dns2;
            if (!dns3.isEmpty())
                si.dns_ << dns3;

            QString ovpnUrl = values[11];
            QString crtUrl = values[12];
            QString keyUrl = values[13];

            if (!ovpnUrl.isEmpty() && !crtUrl.isEmpty() && !keyUrl.isEmpty())
            {
                DownloadFile ovpnFile(ovpnUrl);
                ovpnFile.start();
                ovpnFile.wait();
                if (!ovpnFile.success())
                {
                    bErrorDuringDownload_ = true;
                    errorStr_ = "Error during download config for OpenVPN from server";
                    emit updateFinished(false, errorStr_, false);
                    return;
                }

                QTextStream ds(&downloadConfigOVPN, QIODevice::WriteOnly);
                ds << ovpnFile.data();

            }
            downloadServers << si;
		}
    }

    // compare existing data with updated data
    QString appPath = QApplication::applicationDirPath();

#if defined Q_OS_WIN
    QString savedCsvPath = appPath + "/Data/netlist.csv";
    QString savedOVPNPath = appPath + "/Data/netlist.ovpn";
#elif defined Q_OS_MAC
    QString savedCsvPath = appPath + "/../Resources/Bin/netlist.csv";
    QString savedOVPNPath = appPath + "/../Resources/Bin/netlist.ovpn";
#elif defined Q_OS_UNIX
    QString savedCsvPath = appPath + "/Data/netlist.csv";
    QString savedOVPNPath = appPath + "/Data/netlist.ovpn";
#endif

    QByteArray sigSavedCsv;

    {
        QCryptographicHash hash( QCryptographicHash::Sha1 );
        QFile file(savedCsvPath);

        if (file.open(QIODevice::ReadOnly))
        {
            hash.addData( file.readAll() );
        }
        sigSavedCsv = hash.result();
    }

    QByteArray sigSavedOVPN;
    {
        QCryptographicHash hash( QCryptographicHash::Sha1 );
        QFile file(savedOVPNPath);

        if (file.open(QIODevice::ReadOnly))
        {
            hash.addData( file.readAll() );
        }
        sigSavedOVPN = hash.result();
    }

    QByteArray sigNewCsv;
    {
        QCryptographicHash hash( QCryptographicHash::Sha1 );
        hash.addData( replyBytes );
        sigNewCsv = hash.result();
    }

    QByteArray sigNewOVPN;
    {
        QCryptographicHash hash( QCryptographicHash::Sha1 );
        hash.addData( downloadConfigOVPN );
        sigNewOVPN = hash.result();
    }

    //qDebug() << sigSavedCsv;
    //qDebug() << sigNewCsv;
    //qDebug() << sigSavedOVPN;
    //qDebug() << sigNewOVPN;

    if (sigSavedCsv != sigNewCsv || sigSavedOVPN != sigNewOVPN)
    {
        {
            QFile file(savedCsvPath);

            if (file.open(QIODevice::WriteOnly))
            {
                file.write(replyBytes);
                file.close();
            }
        }
        {
            QFile file(savedOVPNPath);

            if (file.open(QIODevice::WriteOnly))
            {
                file.write(downloadConfigOVPN);
                file.close();
            }
        }

        loadDataFromFiles(savedCsvPath, savedOVPNPath);
        emit updateFinished(true, QString(), true);
    }

    reply->deleteLater();
    manager->deleteLater();
}

