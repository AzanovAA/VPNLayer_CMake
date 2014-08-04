#pragma once

#include <QThread>
#include <QXmlStreamReader>
#include <QStringList>
#include <QMutex>

struct ServerInfo
{
	QString description_;
	QString ip_;
	bool	pptp_;
	bool	l2tp_;
	QString l2tpKey_;
	bool	openvpn_;
    bool    sstp_;
    QStringList dns_;
    QString ovpnUrl_;
};


class GetServers : public QThread
{
	Q_OBJECT

public:
	GetServers(QObject *parent = NULL);

    QVector<ServerInfo> servers();
    QByteArray configOVPN();

signals:
    void updateFinished(bool bSuccess, QString errorStr, bool bConfigChanged);

protected:
	virtual void run();
private:
    bool	bExistData_;
    bool    bErrorDuringDownload_;
	QString errorStr_;
	QVector<ServerInfo> servers_;
    QByteArray configOVPN_;
    QMutex lock_;

    void loadDataFromFiles(QString csvPath, QString OVPNPath);
};

