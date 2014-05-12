#include <QApplication>
#include <QFile>
#include <QDebug>
#include "qopenvpn.h"
#include "../Log.h"

QOpenVPN::QOpenVPN(QObject *parent)
	: QObject(parent), bConnected_(false), hExitEvent_(NULL), exitEventName_("qopenvpn_exit_event_1")
{
	errorsWhileConnecting_ << "TLS Error: Need PEM pass phrase for private key" <<
		"EVP_DecryptFinal:bad decrypt" <<
		"PKCS12_parse:mac verify failure" <<
		"Received AUTH_FAILED control message" <<
		"Auth username is empty" <<
		"error=certificate has expired" <<
		"error=certificate is not yet valid";

    QObject::connect(&process_, SIGNAL(readyRead()), SLOT(onReadyRead()));
    QObject::connect(&process_, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(onFinished()));
}

QOpenVPN::~QOpenVPN()
{
	disconnect(false);
}

bool QOpenVPN::initialize()
{
	QString appPath = QApplication::applicationDirPath();
	openVPNExePath_ = appPath + "/Data/openvpn.exe";

	QFile file(openVPNExePath_);
	if(!file.exists())  
	{
		return false;
	}

	if (!checkVersion())
	{
		return false;
	}

	return true;
}

bool QOpenVPN::checkVersion()
{
	QProcess process(this);

	QStringList params;
	params << "--version";
	process.start(openVPNExePath_, params);
	process.waitForFinished();
	
	QString line = process.readLine();
	if (!line.contains("OpenVPN", Qt::CaseInsensitive))
		return false;
	
	if (line[8] == '2') // Majorversion = 2
	{
		if (line[10] == '0') // Minorversion = 0
		{
			return false;
		}
		else // > 2.0 
		{
			return true;
		}
	}
	else
	{
		if (line[8] == '1') // Majorversion = 1 
		{
			return false;
		}
		else // Majorversion != (1 || 2) 
		{
			return true;
		}
	}

	return false;
}

bool QOpenVPN::connect(QString username, QString password, QString ovpnFile, QString configPath)
{
	hExitEvent_ = CreateEventW (NULL, TRUE, FALSE, (wchar_t *)exitEventName_.utf16());
	if (hExitEvent_ == NULL)
	{
		emit error("Error creating exit event");
		return(false);
	}

	userName_ = username;
	password_ = password;

	QStringList commandParams;
	commandParams << "--service" << exitEventName_ << "0" << "--config" << ovpnFile;

	process_.setWorkingDirectory(configPath);
	process_.setProcessChannelMode(QProcess::MergedChannels);
	process_.start(openVPNExePath_, commandParams, QIODevice::ReadWrite | QIODevice::Text);

	return true;
}

bool QOpenVPN::containsWhileConnectingError(QString line, QString &err)
{
	foreach(QString s, errorsWhileConnecting_)
	{
		if (line.contains(s, Qt::CaseInsensitive))
		{
			return true;
		}
	}

	return false;
}

void QOpenVPN::onReadyRead()
{
	QByteArray data = process_.readLine();
	QString err;
	while (!data.isEmpty())
	{
		QString line = data;
		if (line.contains("Enter Auth Username:", Qt::CaseInsensitive))
		{
			process_.write(userName_.toLocal8Bit());
		}
		else if (line.contains("Enter Auth Password:", Qt::CaseInsensitive))
		{
			process_.write(password_.toLocal8Bit());
		}
		else if (line.contains("Initialization Sequence Completed", Qt::CaseInsensitive))
		{
			bConnected_ = true;
			emit connected();
			ALog::Out(line.trimmed());
		}
		else if (containsWhileConnectingError(line, err))
		{
			emit log(line);
			emit error(err);
			ALog::Out(line.trimmed());
		}
		else if (bConnected_ && line.contains("process restarting", Qt::CaseInsensitive))
		{
			if (hExitEvent_)
			{
				SetEvent(hExitEvent_);
				process_.blockSignals(true);
				process_.waitForFinished();
				process_.blockSignals(false);
				CloseHandle(hExitEvent_);
				hExitEvent_ = NULL;
			}

			emit disconnected(true);
			bConnected_ = false;
		}
		else
		{
			emit log(line);
			ALog::Out(line.trimmed());
		}
		
		//qDebug() << data;
		data = process_.readLine();
	}
}

void QOpenVPN::disconnect(bool bEmitSignal)
{
	if (hExitEvent_)
	{
		SetEvent(hExitEvent_);
		process_.blockSignals(true);
		process_.waitForFinished();
		process_.blockSignals(false);
		CloseHandle(hExitEvent_);
		hExitEvent_ = NULL;
	}

	if (/*bConnected_ &&*/ bEmitSignal)
	{
		emit disconnected(false);
	}
	bConnected_ = false;
}

bool QOpenVPN::tapInstalled()
{
	QProcess process(this);
	QStringList params;
	params << "--show-adapters";
	process.start(openVPNExePath_, params);

	QEventLoop eventLoop;
	eventLoop.connect(&process, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(quit()));
	eventLoop.exec();

	QString str = process.readAll();
	if (str.length() < 4)
		return false;

	if (str[str.length()-3] == ':')
		return false;

	return true;
}

void QOpenVPN::onFinished()
{
	if (bConnected_)
	{
		if (hExitEvent_)
		{
			CloseHandle(hExitEvent_);
			hExitEvent_ = NULL;
		}		
		emit disconnected(true);
	}
	else
	{
		emit error("OpenVPN connect error 2");
		emit disconnected(true);
	}
}
