#include "mainwindow.h"
#include "utils.h"
#include "log.h"
#include "settings.h"
#include <QMessageBox>
#include <QSettings>
#include <QTimer>

#ifdef Q_OS_MAC
    #include "Mac/MacApplication.h"
#endif

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags), loginWindow_(NULL), connectWindow_(NULL), autoConnect_(false), timer_(NULL)
{
	QCoreApplication::setOrganizationName("CPPP");
	QCoreApplication::setApplicationName("VpnLayer");

	ui.setupUi(this);
	setWindowTitle("VpnLayer " + QString(VERSION));

#if defined Q_OS_MAC
    setAttribute(Qt::WA_QuitOnClose, false);
#endif

    trayIcon_ = new QSystemTrayIcon(this);
    setTrayStatusDisconnected(false);

    trayMenu_ = new QMenu(this);
    actShow_ = new QAction("Show", this);
    actAbout_ = new QAction("About", this);
    actExit_ = new QAction("Quit", this);
    trayMenu_->addAction(actShow_);
    trayMenu_->addAction(actAbout_);
    trayMenu_->addAction(actExit_);
    trayIcon_->setContextMenu(trayMenu_);

    connect(actShow_, SIGNAL(triggered()), SLOT(onShow()));
    connect(actAbout_, SIGNAL(triggered()), SLOT(onAbout()));
    connect(actExit_, SIGNAL(triggered()), SLOT(onExit()));

    loginWindow_ = new LoginWindow(this);
	ui.verticalLayout->addWidget(loginWindow_);
	loginWindow_->loadSettings();
	connect(loginWindow_, SIGNAL(signedIn(QString, QString)), SLOT(onSignIn(QString, QString)));

    settingsWindow_ = new SettingsWindow(this);
	connect(settingsWindow_, SIGNAL(back()), SLOT(onBack()));
	connect(settingsWindow_, SIGNAL(returnToSignUp()), SLOT(onReturnToSignUp()));
	ui.verticalLayout->addWidget(settingsWindow_);
	settingsWindow_->hide();

	connectWindow_ = new ConnectWindow(this);
	connect(connectWindow_, SIGNAL(settings()), SLOT(onSettings()));
	ui.verticalLayout->addWidget(connectWindow_);
	connectWindow_->hide();

    getServers_ = new GetServers(this);
    connect(getServers_, SIGNAL(updateFinished(bool, QString, bool)), SLOT(onUpdateFinished(bool, QString, bool)));
	getServers_->start();

    QSettings settings;
	if (settings.value("savePass", "true").toString() == "true")
	{
		QString username = settings.value("login", "").toString();
		QString password = settings.value("password", "").toString();
		if (!username.isEmpty() && !password.isEmpty())
		{
			loginWindow_->hide();
			QString autoConnectOnStart = settings.value("autoConnectOnStart", "false").toString();
			if (autoConnectOnStart == "true")
			{
				autoConnect_ = true;
			}
			onSignIn(username, password);
		}
		else
		{
			adjustSize();
			setFixedSize(size());
		}
	}
	else
	{
		adjustSize();
		setFixedSize(size());
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::onSignIn(QString login, QString password)
{
    //getServers_->wait();
    //if (getServers_->success())
	{
		setFixedSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));

        loginWindow_->hide();

        connectWindow_->setServers(getServers_->servers());
        connectWindow_->setOVPNConfig(getServers_->configOVPN());
		connectWindow_->loadSettings();
        connectWindow_->show();
		adjustSize();
		setFixedSize(size());
		if (autoConnect_)
		{
			connectWindow_->autoConnect();
			autoConnect_ = false;
        }
	}
    //else
    //{
    //	QMessageBox::question(this, QApplication::applicationName(), tr("Servers list can't received."));
    //}
}

void MainWindow::onSettings()
{
	connectWindow_->saveSettings();
	setFixedSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
	connectWindow_->hide();
	settingsWindow_->loadSettings();
	settingsWindow_->show();
	adjustSize();
	setFixedSize(size());
}

void MainWindow::onBack()
{
	setFixedSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
	settingsWindow_->hide();
	settingsWindow_->saveSettings();
	connectWindow_->show();
	adjustSize();
	setFixedSize(size());
}

void MainWindow::onReturnToSignUp()
{
	setFixedSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
	settingsWindow_->hide();
	settingsWindow_->saveSettings();
	loginWindow_->loadSettings();
	loginWindow_->show();
	adjustSize();
	setFixedSize(size());

}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (settingsWindow_->isVisible())
	{
		settingsWindow_->saveSettings();
	}
	if (connectWindow_->isVisible())
	{
		connectWindow_->saveSettings();
    }
    QMainWindow::closeEvent(event);
}

//void MainWindow::onServersFinished()
//{
	//QMessageBox::information(this, QApplication::applicationName(), getServers_->errorString());
//}

void MainWindow::onUpdateFinished(bool bSuccess, QString errorStr, bool bConfigChanged)
{
    if (bSuccess)
    {
        if (bConfigChanged)
        {
            QMessageBox::information(this, QApplication::applicationName(), tr("Servers config was updated! New servers will be available after restart program"));
        }
    }
    else
    {
        QMessageBox::information(this, QApplication::applicationName(), tr("Servers list can't updated.") + errorStr);
    }
}

void MainWindow::setTrayStatusConnected()
{
    SAFE_DELETE(timer_);
    trayIcon_->setIcon(QIcon(":/MainWindow/Images/vpnlogo.ico"));
    trayIcon_->setToolTip("Connection is secured");

    QString msg = tr("Your internet traffic is encrypted and secured now!");
    trayIcon_->showMessage(tr("VPNLayer"), msg);
}

void MainWindow::setTrayStatusDisconnected(bool bShowMessage)
{
    SAFE_DELETE(timer_);
    trayIcon_->setIcon(QIcon(":/MainWindow/Images/vpnlogo-BLACK.ico"));
    trayIcon_->setToolTip("Connection is not secured");

    if (!trayIcon_->isVisible())
    {
        trayIcon_->show();
    }

    if (bShowMessage)
    {
        QString msg = tr("Your internet traffic is not secured anymore");
        trayIcon_->showMessage(tr("VPNLayer"), msg);
    }
}

void MainWindow::onTimer()
{
    if (state_ == 0)
    {
        trayIcon_->setIcon(QIcon(":/MainWindow/Images/vpnlogo.ico"));
        state_ = 1;
    }
    else
    {
        trayIcon_->setIcon(QIcon(":/MainWindow/Images/vpnlogo-BLACK.ico"));
        state_ = 0;
    }
}

void MainWindow::setTrayStatusConnecting()
{
    SAFE_DELETE(timer_);
    timer_ = new QTimer(this);
    timer_->setInterval(500);
    connect(timer_, SIGNAL(timeout()), SLOT(onTimer()));
    state_ = 0;
    timer_->start();
}

void MainWindow::onShow()
{
    show();
    setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
    activateWindow();
#ifdef Q_OS_MAC
    MacApplication *macApp = (MacApplication *)QApplication::instance();
    macApp->activateIgnoring();
#endif
    raise();
}

void MainWindow::onAbout()
{
    QMessageBox::information(this, QApplication::applicationName(), "VpnLayer " + QString(VERSION));
}

void MainWindow::onExit()
{
    close();
    QApplication::instance()->quit();
}

