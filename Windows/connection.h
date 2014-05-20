#ifndef CONNECTION_H
#define CONNECTION_H

#include <Windows.h>
#define WINVER 0x0501
#define UNICODE
#include <ras.h>
#include <raserror.h>
#include <QObject>
#include "../Utils.h"
#include "QOpenVPN.h"


class Connection : public QObject
{
	Q_OBJECT

public:
	Connection(QObject *parent);
	~Connection();

	bool initialize();
    void connect(PROTOCOL_TYPE protocol, QString serverIP, QString username, QString password, QString ovpnFile, QString l2tpKey, QStringList dns);
	void disconnect(bool bEmitSignal);
	bool isConnected() { return connHandle_ != NULL; }
	bool tapInstalled();
	
signals:
	void connected();
	void disconnected(bool errorDisconnect);
	void error(QString error);
	void statisticsChanged(double download, double upload);

protected:
	void customEvent(QEvent *event);
	void timerEvent(QTimerEvent *event);

private:
	enum {UM_CONNECTED = 1001, UM_ERROR = 1002};

	HRASCONN connHandle_;
	int	timerId_;
	bool bFirstCalcStat_;
	DWORD prevBytesRcved_;
	DWORD prevBytesXmited_;

	QOpenVPN openManager_;

	static const wchar_t *g_szConnName; 
	static QObject *g_Connection;
	static QString g_str;
	static void CALLBACK RasDialFunc(UINT unMsg, RASCONNSTATE rasconnstate, DWORD dwError);
};

#endif // CONNECTION_H
