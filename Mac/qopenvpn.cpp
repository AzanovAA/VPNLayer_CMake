#include <QApplication>
#include <QFile>
#include <QDebug>
#include <QUuid>
#include "qopenvpn.h"
#include "../Log.h"

QOpenVPN::QOpenVPN(QObject *parent)
    : QObject(parent), bConnected_(false), exitEventName_("qopenvpn_exit_event_2"), bDnsChanged_(false)
{
	errorsWhileConnecting_ << "TLS Error: Need PEM pass phrase for private key" <<
		"EVP_DecryptFinal:bad decrypt" <<
		"PKCS12_parse:mac verify failure" <<
		"Received AUTH_FAILED control message" <<
		"Auth username is empty" <<
		"error=certificate has expired" <<
		"error=certificate is not yet valid";

    QObject::connect(&process_, SIGNAL(readyReadStandardOutput()), SLOT(onReadyRead()));
    QObject::connect(&process_, SIGNAL(readyReadStandardError()), SLOT(onReadyRead()));
    QObject::connect(&process_, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(onFinished()));

    //connect( conn , SIGNAL(started())
    //       , this , SLOT(processStarted()) );
    //connect( conn , SIGNAL(finished(int,QProcess::ExitStatus))
     //      , this , SLOT(processFinished()) );
    //connect( conn , SIGNAL(readyReadStandardOutput())
    //       , this , SLOT(processHasOutput()) );
    //connect( conn , SIGNAL(readyReadStandardError())
     //      , this , SLOT(processHasOutput()) );

    uuid_ = QUuid::createUuid().toString();
}

QOpenVPN::~QOpenVPN()
{
	disconnect(false);
}

bool QOpenVPN::initialize()
{
    QString appPath = QApplication::applicationDirPath();
    openVPNExePath_ = appPath + "/../Resources/Bin/openvpn";

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

bool QOpenVPN::connect(QString username, QString password, QString ovpnFile, QString configPath, QStringList dns)
{
    dns_ = dns;
    QString tool = QApplication::applicationDirPath()
                     + "/../Resources/Bin/settings";

        {
            QProcess process;
            process.start( tool, QStringList() << "fix_permissions" );
            process.waitForFinished();
        }

        {
            QProcess process;
            process.start( tool, QStringList() << "unload_kexts" );
            process.waitForFinished();
        }

        {
            QProcess process;
            process.start( tool, QStringList() << "dns_init" );
            process.waitForFinished();
        }

        { // remember current gateway
            QProcess process;
            process.start( tool, QStringList() << "gateway" );
            process.waitForFinished();

            QByteArray output = process.readAllStandardOutput();

            m_gateway_ = QString::fromUtf8(output).trimmed();
        }

    start(username, password, QString(), QString(), ovpnFile);


    //hExitEvent_ = CreateEventW (NULL, TRUE, FALSE, (wchar_t *)exitEventName_.utf16());
    //if (hExitEvent_ == NULL)
    //{
    //	emit error("Error creating exit event");
    //	return(false);
    //}

    /*userName_ = username;
	password_ = password;

	QStringList commandParams;
    commandParams << "--config" << ovpnFile;

	process_.setWorkingDirectory(configPath);
	process_.setProcessChannelMode(QProcess::MergedChannels);
	process_.start(openVPNExePath_, commandParams, QIODevice::ReadWrite | QIODevice::Text);
*/
    /*QString tool = QApplication::applicationDirPath() + "/../Resources/Bin/start";
    uuid_ = QUuid::createUuid().toString();

    process_.start( tool, QStringList()
        << uuid_
        << "protocol"
        << "host" << "port" << ovpnFile );

    process_.write( QString("%1\n%2\n").arg(username).arg(password).toUtf8() );
*/
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
        qDebug() << line;
        /*if (line.contains("Enter Auth Username:", Qt::CaseInsensitive))
		{
            process_.write(userName_.toUtf8());
            process_.write("\n");
        }
		else if (line.contains("Enter Auth Password:", Qt::CaseInsensitive))
		{
            process_.write(password_.toUtf8());
            process_.write("\n");
        }
        else*/ if (line.contains("Initialization Sequence Completed", Qt::CaseInsensitive))
		{
			bConnected_ = true;

            QString tool = QApplication::applicationDirPath()
                             + "/../Resources/Bin/settings";

            QProcess process;
            process.start( tool, QStringList() << "dns_on" );
            process.waitForFinished();
            bDnsChanged_ = true;

			emit connected();
			ALog::Out(line.trimmed());
		}
       /* else if (line.contains("AUTH_FAILED", Qt::CaseInsensitive))
        {
            emit log(line);
            emit error("Incorrect user name or password");
            ALog::Out(line.trimmed());
        }
        /*else if (containsWhileConnectingError(line, err))
		{
			emit log(line);
			emit error(err);
			ALog::Out(line.trimmed());
		}
		else if (bConnected_ && line.contains("process restarting", Qt::CaseInsensitive))
		{
            process_.blockSignals(true);
            process_.terminate();
            process_.waitForFinished();
            process_.blockSignals(false);


                emit disconnected(true);
			bConnected_ = false;
        }*/
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
    qDebug() << "Didddddddddddd";
    process_.blockSignals(true);

    QString toolStop = QApplication::applicationDirPath() + "/../Resources/Bin/stop";
    QProcess::execute(toolStop, QStringList() << uuid_);
    process_.waitForFinished();
    process_.blockSignals(false);

    QString tool = QApplication::applicationDirPath()
                     + "/../Resources/Bin/settings";
        {
            QProcess process;
            process.start( tool, QStringList() << "unload_kexts" );
            process.waitForFinished();
        }


        if (bDnsChanged_)
        {
            QProcess process;
            process.start( tool, QStringList() << "dns_off" );
            process.waitForFinished();
            bDnsChanged_ = false;
        }


	if (bConnected_ && bEmitSignal)
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
        /*if (hExitEvent_)
		{
			CloseHandle(hExitEvent_);
			hExitEvent_ = NULL;
        }*/
		emit disconnected(true);
	}
    else
        emit error("Error during connection. Check username and password");
}

void QOpenVPN::start(QString username, QString password, QString host, QString port, QString config)
{
    QString tool = QApplication::applicationDirPath() + "/../Resources/Bin/start";

    process_.start( tool, QStringList()
        << uuid_
        << QString("openvpn")
        << host << port << config );

    process_.write( QString("%1\n%2\n").arg(username).arg(password).toUtf8() );
}
