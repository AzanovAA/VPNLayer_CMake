#include "downloadfile.h"
#include <QtNetwork>

DownloadFile::DownloadFile(QString url, QObject *parent)
    : QThread(parent), success_(false), url_(url)
{

}

void DownloadFile::run()
{
	QNetworkAccessManager *manager = new QNetworkAccessManager();
	QEventLoop eventLoop;
	eventLoop.connect(manager, SIGNAL(finished(QNetworkReply *)), SLOT(quit()));

    QUrl url(url_);
	QNetworkReply *reply = manager->get(QNetworkRequest(url));
	eventLoop.exec();

	if (reply->error() != QNetworkReply::NoError)
	{
		success_ = false;
		return;
	}
	else
	{
		success_ = true;
	}
	
    data_ = reply->readAll();

    reply->deleteLater();
	manager->deleteLater();
}

