#ifndef CONNECTION_H
#define CONNECTION_H

#include "../Utils.h"
#include <QObject>
#include <SystemConfiguration/SCSchemaDefinitions.h>
#include <SystemConfiguration/SCNetwork.h>
#include <SystemConfiguration/SCNetworkConnection.h>
#include <SystemConfiguration/SCNetworkConfiguration.h>
#include "QOpenVPN.h"

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

protected:
    void timerEvent(QTimerEvent *event);

private:
    SCPreferencesRef m_prefs;
    CFStringRef m_appName;
    CFStringRef m_l2tpServiceName;
    CFStringRef m_pptpServiceName;
    CFStringRef m_l2tpServiceId;
    CFStringRef m_pptpServiceId;

    SCNetworkServiceRef m_l2tpService;
    SCNetworkServiceRef m_pptpService;

    SCNetworkConnectionRef m_connection;
    bool    bConnected_;

    QOpenVPN openManager_;

    int	timerId_;
    bool bFirstCalcStat_;
    SInt64 prevBytesRcved_;
    SInt64 prevBytesXmited_;
    bool   bDisconnectInitiatedByUser_;


    CFStringRef initService(CFStringRef type ,CFStringRef name);
    SCNetworkConnectionRef initConnection(CFStringRef serviceId , CFDictionaryRef options);

    void connectPPTP(QString serverIP, QString inUsername, QString inPassword);
    void connectL2TP(QString serverIP, QString inUsername, QString inPassword, QString inL2tpKey);

    static void callback(SCNetworkConnectionRef, SCNetworkConnectionStatus, void *);
};

#endif // CONNECTION_H
