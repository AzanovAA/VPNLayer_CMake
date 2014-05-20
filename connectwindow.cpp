#include "connectwindow.h"
#include "log.h"
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QFile>
#include <QDebug>
#include <QApplication>
#include "mainwindow.h"
#include "showlog.h"
#include "downloadfile.h"

ConnectWindow::ConnectWindow(QWidget *parent)
	: QWidget(parent), state_(STATE_DISCONNECTED), connection_(this)
{
	ui.setupUi(this);

    mainWindow_ = (MainWindow *)parent;

    if (!connection_.initialize())
	{
		QMessageBox::information(this, QApplication::applicationName(), tr("OpenVPN initialize error"));
		QTimer::singleShot(1, parent, SLOT(close()));
		return;
	}

	if (!connection_.tapInstalled())
	{
		QMessageBox::question(this, QApplication::applicationName(), tr("TAP drivers not installed."));
		QTimer::singleShot(1, this, SLOT(close()));
		return;
	}

	ui.cbServer->setItemDelegate(new RowHeightDelegate());
	ui.cbProtocol->setItemDelegate(new RowHeightDelegate());

	connect(ui.btnConnect, SIGNAL(clicked()), SLOT(onClickConnect()));
	connect(ui.btnSettings, SIGNAL(clicked()), SIGNAL(settings()));

	connect(&connection_, SIGNAL(connected()), SLOT(onConnected()));
	connect(&connection_, SIGNAL(disconnected(bool)), SLOT(onDisconnected(bool)));
	connect(&connection_, SIGNAL(error(QString)), SLOT(onConnectionError(QString)));
	connect(&connection_, SIGNAL(statisticsChanged(double, double)), SLOT(onStatisticsChanged(double, double)));

	connect(ui.cbServer, SIGNAL(currentIndexChanged(const QString &)), SLOT(onServerChanged(const QString &)));

    connect(ui.btnShowLog, SIGNAL(clicked()), SLOT(onShowLog()));

	/*fillServersList();
	for (QMap<QString, QString>::iterator it = servers_.begin(); it != servers_.end(); ++it)
	{
		ui.cbServer->addItem(it.key());
	}*/
}

ConnectWindow::~ConnectWindow()
{
	connection_.disconnect(false);
}


void ConnectWindow::onClickConnect()
{
	QSettings settings;
	QString login = settings.value("login").toString();
	QString password = settings.value("password").toString();

	if (state_ == STATE_DISCONNECTED)
	{
        /*ui.btnConnect->setStyleSheet("QPushButton {"
			"border-image :url(:/MainWindow/Images/btnOff.png);}");
		state_ = STATE_CONNECTED;*/

		if (connection_.isConnected())
		{
			ALog::Out("connection_.isConnected()");
			return;
		}

        PROTOCOL_TYPE protocol;
		if (ui.cbProtocol->currentText() == tr("PPTP"))
		{
			protocol = PPTP;
		}
		else if (ui.cbProtocol->currentText() == tr("L2TP"))
		{
			protocol = L2TP;
		}
		else
		{
			protocol = OpenVPN;
            ServerInfo si = servers_[ui.cbServer->currentText()];
            makeOVPNFile(settings.value("openVPNPort", "TCP 443").toString(), servers_[ui.cbServer->currentText()].ip_, configOVPN_);
        }

		ui.btnConnect->setStyleSheet("QPushButton {"
			"border-image :url(:/MainWindow/Images/btnConnecting.png);}");

        // delete log file
        QString pathToLog = QApplication::applicationDirPath() + "/log.txt";
        QFile::remove(pathToLog);
		
		state_ = STATE_CONNECTING;
		timerImageNum_ = 1;
		timerId_ = startTimer(500);
         connection_.connect(protocol, servers_[ui.cbServer->currentText()].ip_, login, password, QString(), servers_[ui.cbServer->currentText()].l2tpKey_, servers_[ui.cbServer->currentText()].dns_);
        mainWindow_->setTrayStatusConnecting();
	}
	else if (state_ == STATE_CONNECTED)
	{
		connection_.disconnect(true);
	}
	else if (state_ == STATE_CONNECTING)
	{
		connection_.disconnect(true);
	}
}

void ConnectWindow::onConnected()
{
	killTimer(timerId_);
	ui.btnConnect->setStyleSheet("QPushButton {"
		"border-image :url(:/MainWindow/Images/btnOn.png);}");
	state_ = STATE_CONNECTED;
    mainWindow_->setTrayStatusConnected();
}

void ConnectWindow::onDisconnected(bool withError)
{
	killTimer(timerId_);
	ui.btnConnect->setStyleSheet("QPushButton {"
		"border-image :url(:/MainWindow/Images/btnOff.png);}");
	state_ = STATE_DISCONNECTED;

	ui.lblDownload->setText("0 KB/s");
	ui.lblUpload->setText("0 KB/s");

	if (withError)
	{

		QSettings settings;
		if (settings.value("reconnectAutomatically").toString() == "true")
		{
			onClickConnect();
        }
	}
    mainWindow_->setTrayStatusDisconnected();
}

void ConnectWindow::onConnectionError(QString msg)
{
    if (!msg.isEmpty())
    {
        ALog::Out("onConnectionError()");
        QMessageBox::information(this, QApplication::applicationName(), msg);
    }
    killTimer(timerId_);
    ui.btnConnect->setStyleSheet("QPushButton {"
        "border-image :url(:/MainWindow/Images/btnOff.png);}");
    state_ = STATE_DISCONNECTED;

    ui.lblDownload->setText("0 KB/s");
    ui.lblUpload->setText("0 KB/s");
    mainWindow_->setTrayStatusDisconnected(false);
}

void ConnectWindow::setEnabled(bool enabled)
{
}

void ConnectWindow::setServers(const QVector<ServerInfo> &servers)
{
	ui.cbServer->clear();
	servers_.clear();
	foreach(const ServerInfo &si, servers)
	{
		servers_[si.description_] = si;
	}

	for (QMap<QString, ServerInfo>::iterator it = servers_.begin(); it != servers_.end(); ++it)
	{
		ui.cbServer->addItem(it.key());
	}
}

void ConnectWindow::onServerChanged(const QString &server)
{
	QString curProtocol = ui.cbProtocol->currentText();

	ui.cbProtocol->clear();
	const ServerInfo &si = servers_[server];
	if (si.pptp_)
	{
		ui.cbProtocol->addItem("PPTP");
	}
	if (si.l2tp_)
	{
		ui.cbProtocol->addItem("L2TP");
	}
	if (si.openvpn_)
	{
		ui.cbProtocol->addItem("OpenVPN");
	}

	int ind = ui.cbProtocol->findText(curProtocol);
	if (ind != -1)
		ui.cbProtocol->setCurrentIndex(ind);
	else
		ui.cbProtocol->setCurrentIndex(0);
}

void ConnectWindow::makeOVPNFile(QString protocolPort, QString server, QByteArray configData)
{
    QString appPath = QApplication::applicationDirPath();
    QString OVPNPath = appPath + "/../Resources/Bin/config.ovpn";

    QStringList strs = protocolPort.split(" ");
	QString protocol, port;
	if (strs.count() == 2)
	{
		protocol = strs[0];
		port = strs[1];
	}
	else
	{
		Q_ASSERT(false);
		return;
	}

    QFile configFile(OVPNPath);

    if (configFile.open(QIODevice::WriteOnly))
    {
        if (protocol.contains("TCP", Qt::CaseInsensitive))
        {
            configFile.write("proto tcp-client\r\n");
        }
        else
        {
            configFile.write("proto udp\r\n");
        }

        QString str = "remote " + server + "\r\n";
        configFile.write(str.toLocal8Bit());
        str = "rport " + port + "\r\n";
        configFile.write(str.toLocal8Bit());

        configFile.write("ca ca.crt\r\n");
        configFile.write("tls-auth ta.key 1\r\n");

        //str = "status file.txt 1\r\n";
        //ovpnFile.write(str.toLocal8Bit());

        //QByteArray arr = file.readAll();
        configFile.write(configData);
        configFile.close();
    }
}


void ConnectWindow::timerEvent(QTimerEvent *event)
{
	if (timerImageNum_ == 1)
	{
		timerImageNum_ = 2;
		ui.btnConnect->setStyleSheet("QPushButton {"
			"border-image :url(:/MainWindow/Images/btnConnecting2.png);}");
	}
	else
	{
		timerImageNum_ = 1;
		ui.btnConnect->setStyleSheet("QPushButton {"
			"border-image :url(:/MainWindow/Images/btnConnecting.png);}");
	}
}

void ConnectWindow::onStatisticsChanged(double download, double upload)
{
    //qDebug() << download;
	ui.lblDownload->setText(QString::number(download / 1024.0, 'f', 2) + " KB/s");
	ui.lblUpload->setText(QString::number(upload / 1024.0, 'f', 2) + " KB/s");
}

void ConnectWindow::saveSettings()
{
	QSettings settings;
	settings.setValue("Server", ui.cbServer->currentText());
	settings.setValue("Protocol", ui.cbProtocol->currentText());
}

void ConnectWindow::loadSettings()
{
	QSettings settings;
	QString server = settings.value("Server").toString();
	QString protocol = settings.value("Protocol").toString();

	int ind = ui.cbServer->findText(server);
	if (ind != -1)
		ui.cbServer->setCurrentIndex(ind);

	ind = ui.cbProtocol->findText(protocol);
	if (ind != -1)
		ui.cbProtocol->setCurrentIndex(ind);
}

void ConnectWindow::autoConnect()
{
	onClickConnect();
}

void ConnectWindow::onShowLog()
{
    QScopedPointer<ShowLog> showLog(new ShowLog(this));
    showLog->exec();
}

