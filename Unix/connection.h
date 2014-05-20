#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include "../utils.h"

class QProcess;

class Connection : public QObject
{
	Q_OBJECT

public:
	Connection(QObject *parent);
	~Connection();

    bool initialize();
    void connect(PROTOCOL_TYPE protocol, QString serverIP, QString inUsername, QString inPassword, QString ovpnFile, QString l2tpKey, QStringList dns);
    void disconnect(bool bEmitSignal);
    bool isConnected() { return bConnected_; }
    bool tapInstalled();

signals:
    void connected();
    void disconnected(bool errorDisconnect);
    void error(QString error);
    void statisticsChanged(double download, double upload);

private slots:
    void onReadyReadStandardOutput();
    void onFinished();

private:
    bool    bConnected_;
    bool    bDoDisconnect_;

    QProcess *process_;

    void connectPPTP(QString serverIP, QString inUsername, QString inPassword);
};

#endif // CONNECTION_H
