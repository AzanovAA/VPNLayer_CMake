#ifndef QOPENVPN_H
#define QOPENVPN_H

#include <QObject>
#include <QProcess>

class QOpenVPN : public QObject
{
	Q_OBJECT

public:
	QOpenVPN(QObject *parent);
	~QOpenVPN();

	bool initialize();
	bool tapInstalled();
    bool connect(QString username, QString password, QString ovpnFile, QString configPath, QStringList dns);
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
    bool        bDnsChanged_;
	QString		openVPNExePath_;
	QString		exitEventName_;
    //HANDLE		hExitEvent_;
	QString		userName_;
	QString		password_;
	QStringList errorsWhileConnecting_;

	QProcess	process_;
    QString     uuid_;
    QString     m_gateway_;
    QStringList dns_;


	bool checkVersion();
	bool containsWhileConnectingError(QString line, QString &err);
	
    void start(QString username, QString password, QString host, QString port, QString config);
};

#endif // QOPENVPN_H
