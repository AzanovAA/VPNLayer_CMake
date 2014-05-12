#include "connection.h"
#include <QApplication>
#include <QDebug>

Connection *g_connectionThis;

Connection::Connection(QObject *parent)
    : QObject(parent), bConnected_(false), openManager_(this), m_connection(NULL), bDisconnectInitiatedByUser_(false)
{
    QObject::connect(&openManager_, SIGNAL(connected()), SIGNAL(connected()));
    QObject::connect(&openManager_, SIGNAL(disconnected(bool)), SIGNAL(disconnected(bool)));
    QObject::connect(&openManager_, SIGNAL(error(QString)), SIGNAL(error(QString)));

    g_connectionThis = this;
    m_l2tpServiceId  = NULL;
    m_pptpServiceId  = NULL;

    QString qAppName = qApp->applicationName();

    m_appName = CFStringCreateWithCString( NULL
                         , QString(qAppName).toUtf8().data()
                         , kCFStringEncodingUTF8 );
    m_l2tpServiceName = CFStringCreateWithCString( NULL
                          , QString(qAppName + ":L2TP").toUtf8().data()
                          , kCFStringEncodingUTF8 );
    m_pptpServiceName = CFStringCreateWithCString( NULL
                          , QString(qAppName + ":PPTP").toUtf8().data()
                          , kCFStringEncodingUTF8 );

    m_prefs        = NULL;
    m_l2tpService  = NULL;
    m_pptpService  = NULL;

    AuthorizationFlags flags = kAuthorizationFlagDefaults
                             | kAuthorizationFlagExtendRights
                             | kAuthorizationFlagInteractionAllowed
                             | kAuthorizationFlagPreAuthorize;
    OSStatus err;
    AuthorizationRef auth;
    err = AuthorizationCreate(0, kAuthorizationEmptyEnvironment, flags, &auth);
    if (err == noErr)
        m_prefs = SCPreferencesCreateWithAuthorization(0, m_appName, 0, auth);
    else
        m_prefs = SCPreferencesCreate(0, m_appName, 0);

    if (!m_prefs) return;

    CFArrayRef connArray = SCNetworkServiceCopyAll(m_prefs);
    if (!connArray) return;

    for (CFIndex i = 0, sz = CFArrayGetCount(connArray); i < sz; ++i)
    {
        SCNetworkServiceRef service;
        service = (SCNetworkServiceRef) CFArrayGetValueAtIndex(connArray, i);

        CFStringRef serviceName = SCNetworkServiceGetName(service);

        if (kCFCompareEqualTo == CFStringCompare(m_l2tpServiceName
                                   , serviceName
                                   , kCFCompareCaseInsensitive)
        ) { m_l2tpServiceId = SCNetworkServiceGetServiceID(service); }

        if (kCFCompareEqualTo == CFStringCompare(m_pptpServiceName
                                   , serviceName
                                   , kCFCompareCaseInsensitive)
        ) { m_pptpServiceId = SCNetworkServiceGetServiceID(service); }
    }

    CFRelease(connArray);
}

Connection::~Connection()
{
    if (m_connection)
    {
        SCNetworkConnectionUnscheduleFromRunLoop(m_connection, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        CFRelease(m_connection);
        m_connection = NULL;
    }
}

void Connection::connect(PROTOCOL_TYPE protocol, QString serverIP, QString inUsername, QString inPassword, QString ovpnFile, QString l2tpKey, QStringList dns)
{
    if (protocol == PPTP)
    {
        connectPPTP(serverIP, inUsername, inPassword);
    }
    else if (protocol == L2TP)
    {
        connectL2TP(serverIP, inUsername, inPassword, l2tpKey);
    }
    else if (protocol == OpenVPN)
    {
        //ALog::Out("Do connect to OpenVPN");
        QString appPath = QApplication::applicationDirPath();
        QString OVPNPath = "config.ovpn";
        QString ConfigPath = appPath + "/../Resources/Bin";
        openManager_.connect(inUsername, inPassword, OVPNPath, ConfigPath, dns);

        return;
    }
}

void Connection::connectPPTP(QString serverIP, QString inUsername, QString inPassword)
{
    if (!m_pptpServiceId)
        m_pptpServiceId = initService( kSCNetworkInterfaceTypePPTP
                                     , m_pptpServiceName );
    if (!m_pptpServiceId)
        return;

    std::string username = inUsername.toStdString();
    std::string password = inPassword.toStdString();
    std::string hostname = serverIP.toStdString();


    CFStringRef cfUsername = CFStringCreateWithCString( NULL
                                                      , username.c_str()
                                                      , kCFStringEncodingUTF8 );
    CFStringRef cfPassword = CFStringCreateWithCString( NULL
                                                      , password.c_str()
                                                      , kCFStringEncodingUTF8 );
    CFStringRef cfHostname = CFStringCreateWithCString( NULL
                                                      , hostname.c_str()
                                                      , kCFStringEncodingUTF8 );
   CFIndex index = 0;
    const void *pppKeys[3], *pppVals[3];
    pppKeys[index] = (void*) kSCPropNetPPPAuthName;
    pppVals[index] = (void*) cfUsername;
    index++;
    pppKeys[index] = (void*) kSCPropNetPPPAuthPassword;
    pppVals[index] = (void*) cfPassword;
    index++;
    pppKeys[index] = (void*) kSCPropNetPPPCommRemoteAddress;
    pppVals[index] = (void*) cfHostname;
    index++;

    CFDictionaryRef pppDialOptions;
    pppDialOptions = CFDictionaryCreate( NULL
                                       , pppKeys
                                       , pppVals
                                       , index
                                       , &kCFTypeDictionaryKeyCallBacks
                                       , &kCFTypeDictionaryValueCallBacks );
    CFStringRef     keys[] = { kSCEntNetPPP   };
    CFDictionaryRef vals[] = { pppDialOptions };

    CFDictionaryRef optionsForDial;
    optionsForDial = CFDictionaryCreate( NULL
                                       , (const void **) &keys
                                       , (const void **) &vals
                                       , 1
                                       , &kCFTypeDictionaryKeyCallBacks
                                       , &kCFTypeDictionaryValueCallBacks );
    bConnected_ = false;

    if (m_connection)
    {
        SCNetworkConnectionUnscheduleFromRunLoop(m_connection, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        CFRelease(m_connection);
        m_connection = NULL;
    }

    m_connection = initConnection( m_pptpServiceId, optionsForDial );
}

void Connection::connectL2TP(QString serverIP, QString inUsername, QString inPassword, QString inL2tpKey)
{
    if (!m_l2tpServiceId)
        m_l2tpServiceId = initService( kSCNetworkInterfaceTypeL2TP
                                     , m_l2tpServiceName );
    if (!m_l2tpServiceId)
        return;

    std::string username = inUsername.toStdString();
    std::string password = inPassword.toStdString();
    std::string hostname = serverIP.toStdString();
    std::string l2tpKey = inL2tpKey.toStdString();

    CFStringRef cfUsername = CFStringCreateWithCString( NULL
                                                      , username.c_str()
                                                      , kCFStringEncodingUTF8 );
    CFStringRef cfPassword = CFStringCreateWithCString( NULL
                                                      , password.c_str()
                                                      , kCFStringEncodingUTF8 );
    CFStringRef cfHostname = CFStringCreateWithCString( NULL
                                                      , hostname.c_str()
                                                      , kCFStringEncodingUTF8 );
    CFStringRef cfSecret = CFStringCreateWithCString( NULL
                                                    , l2tpKey.c_str()
                                                    , kCFStringEncodingUTF8 );
    CFIndex index = 0;
    const void *pppKeys[3], *pppVals[3];
    pppKeys[index] = (void*) kSCPropNetPPPAuthName;
    pppVals[index] = (void*) cfUsername;
    index++;
    pppKeys[index] = (void*) kSCPropNetPPPAuthPassword;
    pppVals[index] = (void*) cfPassword;
    index++;
    pppKeys[index] = (void*) kSCPropNetPPPCommRemoteAddress;
    pppVals[index] = (void*) cfHostname;
    index++;

    CFDictionaryRef pppDialOptions;
    pppDialOptions = CFDictionaryCreate( NULL
                                       , pppKeys
                                       , pppVals
                                       , index
                                       , &kCFTypeDictionaryKeyCallBacks
                                       , &kCFTypeDictionaryValueCallBacks );
    index = 0;
    const void *l2tpKeys[3], *l2tpVals[3];
    l2tpKeys[index] = (void*) kSCPropNetL2TPIPSecSharedSecret;
    l2tpVals[index] = (void*) cfSecret;
    index++;
    // l2tpKeys[index] = (void*) kSCPropNetL2TPIPSecSharedSecretEncryption;
    // l2tpVals[index] = (void*) kSCValNetL2TPIPSecSharedSecretEncryptionKeychain;
    // index++;
    l2tpKeys[index] = (void*) kSCPropNetL2TPTransport;
    l2tpVals[index] = (void*) kSCValNetL2TPTransportIPSec; // kSCValNetL2TPTransportIPSec
    index++;

    CFDictionaryRef l2tpDialOptions;
    l2tpDialOptions = CFDictionaryCreate( NULL
                                        , l2tpKeys
                                        , l2tpVals
                                        , index
                                        , &kCFTypeDictionaryKeyCallBacks
                                        , &kCFTypeDictionaryValueCallBacks );

    CFStringRef     keys[] = { kSCEntNetPPP   , kSCEntNetL2TP   };
    CFDictionaryRef vals[] = { pppDialOptions , l2tpDialOptions };

    CFDictionaryRef optionsForDial;
    optionsForDial = CFDictionaryCreate( NULL
                                       , (const void **) &keys
                                       , (const void **) &vals
                                       , 2
                                       , &kCFTypeDictionaryKeyCallBacks
                                       , &kCFTypeDictionaryValueCallBacks );
    bConnected_ = false;
    if (m_connection)
    {
        SCNetworkConnectionUnscheduleFromRunLoop(m_connection, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        CFRelease(m_connection);
        m_connection = NULL;
    }
    m_connection = initConnection( m_l2tpServiceId, optionsForDial );
 }



void Connection::disconnect(bool bEmitSignal)
{
    bDisconnectInitiatedByUser_ = true;
    SCNetworkConnectionStop(m_connection, TRUE);
    openManager_.disconnect(bEmitSignal);
}


bool Connection::initialize()
{
    return openManager_.initialize();
}

bool Connection::tapInstalled()
{
    return openManager_.tapInstalled();
}

CFStringRef Connection::initService( CFStringRef type , CFStringRef name )
{
    SCPreferencesLock(m_prefs, FALSE);

    SCNetworkInterfaceRef IF;
    IF = SCNetworkInterfaceCreateWithInterface(kSCNetworkInterfaceIPv4, type);
    if (IF)
        IF = SCNetworkInterfaceCreateWithInterface(IF, kSCNetworkInterfaceTypePPP);
    if (!IF) return NULL;

    SCNetworkServiceRef service;
    service = SCNetworkServiceCreate(m_prefs, IF);
    if (!service) return NULL;

    Boolean operationOk;
    operationOk = SCNetworkServiceSetName(service, name);
    if (!operationOk) return NULL;

    operationOk = SCNetworkServiceEstablishDefaultConfiguration(service);
    if (!operationOk) return NULL;

    SCNetworkProtocolRef protocol;
    protocol = SCNetworkServiceCopyProtocol(service, CFSTR("IPv4"));
    if (!protocol) return NULL;

    CFDictionaryRef ip4DialOptions = NULL;
    const void *ip4Keys[1];
    const void *ip4Vals[1];
    const int val = 1;
    ip4Keys[0] = (void *) kSCPropNetOverridePrimary;
    ip4Vals[0] = (void *) CFNumberCreate(NULL, kCFNumberIntType, &val);
    ip4DialOptions = CFDictionaryCreate( NULL
                                       , (const void**)ip4Keys
                                       , (const void**)ip4Vals
                                       , 1
                                       , &kCFTypeDictionaryKeyCallBacks
                                       , &kCFTypeDictionaryValueCallBacks );
    operationOk = SCNetworkProtocolSetConfiguration( protocol, ip4DialOptions );

    SCNetworkSetRef set = SCNetworkSetCopyCurrent(m_prefs);
    if (!set) return NULL;

    if (set && SCNetworkSetAddService(set, service))
    {
        if (!SCPreferencesCommitChanges(m_prefs))
            qDebug() << "Error adding service (on commit) : " << SCErrorString(SCError());

        if (!SCPreferencesApplyChanges(m_prefs))
            qDebug() << "Error adding service (on apply) : " << SCErrorString(SCError());

        SCPreferencesSynchronize(m_prefs);
        // sleep(2);
    }
    CFStringRef serviceId = SCNetworkServiceGetServiceID(service);

    SCPreferencesUnlock(m_prefs);

    CFRelease(set);
    CFRelease(ip4DialOptions);
    CFRelease(protocol);

    return serviceId;
}

SCNetworkConnectionRef Connection::initConnection(CFStringRef serviceId , CFDictionaryRef options)
{
    if (!serviceId) return NULL;

    for (int I = 0; I < 3; I++)
    {
        SCNetworkConnectionRef connection;
        connection = SCNetworkConnectionCreateWithServiceID( NULL
                       , serviceId
                       , Connection::callback
                       , NULL );
        if (!connection) return NULL;

        Boolean ok;
        ok = SCNetworkConnectionScheduleWithRunLoop( connection
                                                   , CFRunLoopGetCurrent()
                                                   , kCFRunLoopDefaultMode );
        if (!ok) return NULL;

        ok = SCNetworkConnectionStart(connection, options, false);
        if (ok) return connection;

        sleep(3);
    }

    return NULL;
}


void Connection::callback( SCNetworkConnectionRef connection, SCNetworkConnectionStatus status, void *info)
{
    CFDictionaryRef dict;
    CFIndex count;
    switch(status)
    {
        case kSCNetworkConnectionDisconnected:
           if (!g_connectionThis->bConnected_)
           {
               g_connectionThis->emit error(QString());
           }
           else
           {
               g_connectionThis->bConnected_ = false;
               g_connectionThis->killTimer(g_connectionThis->timerId_);

               if (g_connectionThis->bDisconnectInitiatedByUser_)
               {
                   g_connectionThis->bDisconnectInitiatedByUser_ = false;
                   g_connectionThis->emit disconnected(false);
               }
               else
               {
                   g_connectionThis->emit disconnected(true);
               }
           }
           //dict = SCNetworkConnectionCopyExtendedStatus(connection);
           //CFDictionaryApplyFunction(dict, printKeys, NULL);

        //count = CFDictionaryGetCount(ct);
           //CFDictionaryGetKeysAndValues(dict, )
           /*T->second->infoPtr->state = Vpn::Connection::Disconnected;

            that->Vpn::Connection::Controller::removed(T->second->infoPtr);

            CFRelease( T->second->connection );
            delete T->second;
            that->m_conn2info.erase(T);*/

            break;
        case kSCNetworkConnectionConnecting:
            /*T->second->infoPtr->state = Vpn::Connection::Connecting;

                that->Vpn::Connection::Controller::changed(T->second->infoPtr);*/
                break;
        case kSCNetworkConnectionConnected:
            {
                g_connectionThis->bConnected_ = true;
                g_connectionThis->emit connected();
                g_connectionThis->bFirstCalcStat_ = true;
                g_connectionThis->prevBytesRcved_ = 0;
                g_connectionThis->prevBytesXmited_ = 0;
                g_connectionThis->timerId_ = g_connectionThis->startTimer(1000);
            }
             break;
        case kSCNetworkConnectionDisconnecting:

                break;
        case kSCNetworkConnectionInvalid:

                break;

    }
}


static void printKeys (const void* key, const void* value, void* context)
{
  CFShow(key);
}

void Connection::timerEvent( QTimerEvent *event )
{
    if (bConnected_)
    {
        CFDictionaryRef dic = SCNetworkConnectionCopyStatistics(m_connection);
        if (dic)
        {
            CFDictionaryRef dictRef = (CFDictionaryRef)CFDictionaryGetValue(dic, CFSTR("PPP"));
            CFNumberRef numberRecvRef = (CFNumberRef)CFDictionaryGetValue(dictRef, kSCNetworkConnectionBytesIn);
            CFNumberRef numberSendRef = (CFNumberRef)CFDictionaryGetValue(dictRef, kSCNetworkConnectionBytesOut);

            SInt64 bytesRcved;
            CFNumberGetValue(numberRecvRef, kCFNumberSInt64Type, &bytesRcved);
            SInt64 bytesXmited;
            CFNumberGetValue(numberSendRef, kCFNumberSInt64Type, &bytesXmited);

            if (bFirstCalcStat_)
            {
                prevBytesRcved_ = bytesRcved;
                prevBytesXmited_ = bytesXmited;
                bFirstCalcStat_ = false;
            }
            else
            {
                emit statisticsChanged(bytesRcved - prevBytesRcved_, bytesXmited - prevBytesXmited_);
                prevBytesRcved_ = bytesRcved;
                prevBytesXmited_ = bytesXmited;
            }
         }
    }
}
