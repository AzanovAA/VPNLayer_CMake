#include "connection.h"
#include <QApplication>
#include "../Log.h"
#include <QMessageBox>

const wchar_t *Connection::g_szConnName = L"VpnLayer"; 
QObject *Connection::g_Connection = NULL;
QString Connection::g_str;

#pragma comment(lib, "rasapi32.lib")

Connection::Connection(QObject *parent)
	: QObject(parent), connHandle_(NULL), openManager_(this)
{
	g_Connection = this;

	QObject::connect(&openManager_, SIGNAL(connected()), SIGNAL(connected()));
	QObject::connect(&openManager_, SIGNAL(disconnected(bool)), SIGNAL(disconnected(bool)));
	QObject::connect(&openManager_, SIGNAL(error(QString)), SIGNAL(error(QString)));
}

Connection::~Connection()
{
}

void CALLBACK Connection::RasDialFunc( UINT unMsg, RASCONNSTATE rasconnstate, DWORD dwError )
{
	if (rasconnstate == RASCS_OpenPort) {
		g_str = "OpenPort";
	}
	else if (rasconnstate == RASCS_PortOpened) {
		g_str = "PortOpened";
	}
	else if (rasconnstate == RASCS_ConnectDevice) {
		g_str = "ConnectDevice";
	}
	else if (rasconnstate == RASCS_DeviceConnected) {
		g_str = "DeviceConnected";
	}
	else if (rasconnstate == RASCS_AllDevicesConnected) {
		g_str = "AllDevicesConnected";
	}
	else if (rasconnstate == RASCS_Authenticate) {
		g_str = "Authenticate";
	}
	else if (rasconnstate == RASCS_AuthNotify) {
		g_str = "AuthNotify";
	}
	else if (rasconnstate == RASCS_AuthRetry) {
		g_str = "AuthRetry";
	}
	else if (rasconnstate == RASCS_AuthCallback) {
		g_str = "AuthCallback";
	}
	else if (rasconnstate == RASCS_AuthChangePassword) {
		g_str = "AuthChangePassword";
	}
	else if (rasconnstate == RASCS_AuthProject) {
		g_str = "AuthProject";
	}
	else if (rasconnstate == RASCS_AuthLinkSpeed) {
		g_str = "AuthLinkSpeed";
	}
	else if (rasconnstate == RASCS_AuthAck) {
		g_str = "AuthAck";
	}
	else if (rasconnstate == RASCS_ReAuthenticate) {
		g_str = "ReAuthenticate";
	}
	else if (rasconnstate == RASCS_Authenticated) {
		g_str = "Authenticated";
	}
	else if (rasconnstate == RASCS_PrepareForCallback) {
		g_str = "PrepareForCallback";
	}
	else if (rasconnstate == RASCS_WaitForModemReset) {
		g_str = "WaitForModemReset";
	}
	else if (rasconnstate == RASCS_WaitForCallback) {
		g_str = "WaitForCallback";
	}
	else if (rasconnstate == RASCS_Projected) {
		g_str = "Projected";
	}
	else if (rasconnstate == RASCS_StartAuthentication) {
		g_str = "StartAuthentication";
	}
	else if (rasconnstate == RASCS_CallbackComplete) {
		g_str = "CallbackComplete";
	}
	else if (rasconnstate == RASCS_LogonNetwork) {
		g_str = "LogonNetwork";
	}
	else if (rasconnstate == RASCS_SubEntryConnected) {
		g_str = "SubEntryConnected";
	}
	else if (rasconnstate == RASCS_SubEntryDisconnected) {
		g_str = "SubEntryDisconnected";
	}
	else if (rasconnstate == RASCS_Interactive) {
		g_str = "Interactive";
	}
	else if (rasconnstate == RASCS_RetryAuthentication) {
		g_str = "RetryAuthentication";
	}
	else if (rasconnstate == RASCS_CallbackSetByCaller) {
		g_str = "CallbackSetByCaller";
	}
	else if (rasconnstate == RASCS_PasswordExpired) {
		g_str = "PasswordExpired";
	}
	else if (rasconnstate == RASCS_InvokeEapUI) {
		g_str = "InvokeEapUI";
	}
	else if (rasconnstate == RASCS_Connected) {
		g_str = "Connected";
		if (dwError == 0)
		{
			QApplication::postEvent(g_Connection,new QEvent(QEvent::Type(UM_CONNECTED)));
		}
	}
	else if (rasconnstate == RASCS_Disconnected) {
		g_str = "Disconnected";
	}

	if (dwError == 0) {
		g_str += ": Ok";
	}
	else {
		wchar_t strErr[1024];
		RasGetErrorString(dwError, strErr, 1024);
		g_str += ": " + QString::fromWCharArray(strErr);

		QApplication::postEvent(g_Connection,new QEvent(QEvent::Type(UM_ERROR)));
	}

	ALog::Out(g_str);
}

void Connection::customEvent( QEvent *event )
{
	if (event->type() == (QEvent::Type)UM_CONNECTED)
	{
		emit connected();
		bFirstCalcStat_ = true;
		prevBytesRcved_ = 0;
		prevBytesXmited_ = 0;
		timerId_ = startTimer(1000);
	}
	else if (event->type() == (QEvent::Type)UM_ERROR)
	{
		disconnect(false);
		emit error(g_str);
	}
}

void Connection::connect( PROTOCOL_TYPE protocol, QString serverIP, QString username, QString password , QString ovpnFile, QString l2tpKey, QStringList dns)
{
	if (protocol == OpenVPN) 
	{
		ALog::Out("Do connect to OpenVPN");

		QString appPath = QApplication::applicationDirPath();
		QString OVPNPath = "config.ovpn";
		QString ConfigPath = appPath + "/Data";
		openManager_.connect(username, password, OVPNPath, ConfigPath);

		return;
	}

	RASDEVINFO *devinfo;
	DWORD dwSize;
	DWORD dwNumberDevices = 0;

	dwSize = 0;

	RasEnumDevices(0, &dwSize, &dwNumberDevices);
	devinfo = (RASDEVINFO*)malloc(dwSize);
	devinfo[0].dwSize = sizeof(RASDEVINFO);

	DWORD err = RasEnumDevices(devinfo, &dwSize, &dwNumberDevices);
	if (err != 0) 
	{
		QString errMsg = "RasEnumDevices return error code: " + err;
		ALog::Out(errMsg);
		free(devinfo);
		emit error("RasEnumDevices return error code");
		return;
	}

	int DeviceInd = -1;
	QString selProtocol;
	if (protocol == PPTP)
	{
		selProtocol = "pptp";
		ALog::Out("Do connect to PPTP");
	}
	else 
	{
		selProtocol = "l2tp";
		ALog::Out("Do connect to L2TP");
	}
	
	for (DWORD i = 0; i < dwNumberDevices; i++) 
	{
		QString devtype = QString::fromUtf16((const ushort *)devinfo[i].szDeviceType);
		QString devname = QString::fromUtf16((const ushort *)devinfo[i].szDeviceName);
		devtype = devtype.toLower();
		devname = devname.toLower();

		if (devtype.contains("vpn") && devname.contains(selProtocol)) 
		{
			DeviceInd = i;
			break;
		}
	}

	if (DeviceInd == -1) 
	{
		ALog::Out("Unable to find device with VPN PPTP");
		free(devinfo);
		emit error("Unable to find device with VPN PPTP");
		return;
	}

	RASENTRY re;
	DWORD dwEntrySize;

	memset(&re, 0, sizeof(re));
	re.dwSize = sizeof(re);
	dwEntrySize = sizeof(re);

    //wcscpy(re.szLocalPhoneNumber, serverIP.toStdWString().data());
    wcscpy(re.szLocalPhoneNumber, (wchar_t*)serverIP.utf16());
	wcscpy(re.szDeviceName, devinfo[DeviceInd].szDeviceName);
	wcscpy(re.szDeviceType, devinfo[DeviceInd].szDeviceType);

	if (selProtocol == "pptp")
	{
		re.dwfOptions = 0x20011C10;
		re.dwFramingProtocol = RASFP_Ppp;
		re.dwType = RASET_Vpn;
		re.dwVpnStrategy = VS_PptpOnly;
		re.dwfNetProtocols = RASNP_Ip;
		re.dwEncryptionType = ET_RequireMax;
		re.dwRedialCount = 99;
		re.dwRedialPause = 60;
		re.dwfOptions2 = 263;
	}
	else if (selProtocol == "l2tp")
	{
		re.dwfOptions = 0x20011C10;
		re.dwFramingProtocol = RASFP_Ppp;
		re.dwType = RASET_Vpn;
		re.dwVpnStrategy = VS_L2tpOnly;
		re.dwfNetProtocols = RASNP_Ip;
		re.dwEncryptionType = ET_RequireMax;
		re.dwRedialCount = 99;
		re.dwRedialPause = 60;
		re.dwfOptions2 = 279;
	}
	
	err = RasSetEntryProperties(NULL, g_szConnName, &re, dwEntrySize, NULL, NULL);
	if (err != 0) 
	{
		free(devinfo);
		emit error("RasSetEntryProperties return error code");
		return;
	}

	if (selProtocol == "l2tp")
	{
		RASCREDENTIALS ras_cre_psk = {0};
		ras_cre_psk.dwSize = sizeof(ras_cre_psk);
        ras_cre_psk.dwMask = RASCM_PreSharedKey; //0x00000010; //RASCM_PreSharedKey;
		wcscpy(ras_cre_psk.szPassword, (wchar_t *)l2tpKey.utf16());
        err = RasSetCredentials(NULL, g_szConnName, &ras_cre_psk, FALSE);
        /*if (err != 0)
		{
			free(devinfo);
			emit error("RasSetCredentials return error code");
			return;
        }*/
	}

	RASDIALPARAMS dialparams;

	memset(&dialparams, 0, sizeof(dialparams));
	dialparams.dwSize = sizeof(dialparams);
	wcscpy(dialparams.szEntryName, g_szConnName);

    //wcscpy(dialparams.szUserName, username.toStdWString().data());
    //wcscpy(dialparams.szPassword, password.toStdWString().data());
    wcscpy(dialparams.szUserName, (wchar_t *)username.utf16());
    wcscpy(dialparams.szPassword, (wchar_t *)password.utf16());

	err = RasSetEntryDialParams(NULL, &dialparams, FALSE);
	if (err != 0) 
	{
		QString errMsg = "RasSetEntryDialParams return error code: " + err;
		ALog::Out(errMsg);
		emit error("RasSetEntryDialParams return error code");
		return;
	}

	// Connect
	err = RasDial(NULL, NULL, &dialparams, 0, (void *)RasDialFunc, &connHandle_);
	if (err != 0) 
	{
		QString errMsg = "RasDial return error code: " + err;
		ALog::Out(errMsg);
		emit error("RasDial return error code");
		connHandle_ = NULL;
		return;
	}

	free(devinfo);
}

void Connection::disconnect(bool bEmitSignal)
{
	if (connHandle_) 
	{
		DWORD err = RasHangUp(connHandle_);
		/*if (err != ERROR_SUCCESS)
		{
			QMessageBox::information((QWidget *)parent(), "RasHangUp", "Error code: " + QString::number(err));
			return;
		}*/

		err = 0;
		while (err != ERROR_INVALID_HANDLE) {
			RASCONNSTATUS status;
			memset(&status, 0, sizeof(status));
			status.dwSize = sizeof(status);
			err = RasGetConnectStatus(connHandle_, &status);
			Sleep(1);
		}
		killTimer(timerId_);
		connHandle_ = NULL;
		emit disconnected(false);
	}
	if (openManager_.isConnect())
	{
		openManager_.disconnect(bEmitSignal);
	}
}

void Connection::timerEvent( QTimerEvent *event )
{
	if (connHandle_) 
	{
		RASCONNSTATUS status;
		memset(&status, 0, sizeof(status));
		status.dwSize = sizeof(status);
		DWORD err = RasGetConnectStatus(connHandle_, &status);
		if (err == ERROR_INVALID_HANDLE)
		{
			connHandle_ = NULL;
			killTimer(timerId_);
			emit disconnected(true);
		}
		else
		{
			RAS_STATS stats;
			stats.dwSize = sizeof(RAS_STATS);
			if (RasGetLinkStatistics(connHandle_, 1, &stats) == ERROR_SUCCESS)
			{
				if (bFirstCalcStat_)
				{
					prevBytesRcved_ = stats.dwBytesRcved;
					prevBytesXmited_ = stats.dwBytesXmited;
					bFirstCalcStat_ = false;
				}
				else
				{
					emit statisticsChanged(stats.dwBytesRcved - prevBytesRcved_, stats.dwBytesXmited - prevBytesXmited_);
					prevBytesRcved_ = stats.dwBytesRcved;
					prevBytesXmited_ = stats.dwBytesXmited;
				}
			}
		}
	}
}

bool Connection::initialize()
{
	return openManager_.initialize();
}

bool Connection::tapInstalled()
{
	return openManager_.tapInstalled();
}
