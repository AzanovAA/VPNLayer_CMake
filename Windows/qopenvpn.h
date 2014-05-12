#ifndef QOPENVPN_H
#define QOPENVPN_H

#include <QObject>
#include <windows.h>
#include <QProcess>

class QOpenVPN : public QObject
{
	Q_OBJECT

public:
	QOpenVPN(QObject *parent);
	~QOpenVPN();

	bool initialize();
	bool tapInstalled();
	bool connect(QString username, QString password, QString ovpnFile, QString configPath);
	void disconnect(bool bEmitSignal);
	bool isConnect() { return bConnected_; }
	
signals:
	void connected();
	void disconnected(bool errorDisconnect);
	void error(QString str);
	void log(QString str);

private slots:
	void onReadyRead();
	void onFinished();


private:
	bool		bConnected_;
	QString		openVPNExePath_;
	QString		exitEventName_;
	HANDLE		hExitEvent_;
	QString		userName_;
	QString		password_;
	QStringList errorsWhileConnecting_;

	QProcess	process_;

	bool checkVersion();
	bool containsWhileConnectingError(QString line, QString &err);
	
};

#endif // QOPENVPN_H
