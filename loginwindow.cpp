#include "loginwindow.h"
#include <QSettings>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>

LoginWindow::LoginWindow(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.btnSignIn, SIGNAL(clicked()), SLOT(onSignIn()));
	connect(ui.lblSignUp, SIGNAL(linkActivated(const QString &)), SLOT(onSignUpLink(const QString &)));
}

LoginWindow::~LoginWindow()
{
}

void LoginWindow::onSignIn()
{
	if (ui.edLogin->text().isEmpty() || ui.edPassword->text().isEmpty())
	{
		QMessageBox::information(this, QApplication::applicationName(), tr("You must enter username and password to sign in"));
	}
	else
	{
		QSettings settings;
		if (ui.cbSavePassword->isChecked())
		{
			settings.setValue("login", ui.edLogin->text());
			settings.setValue("password", ui.edPassword->text());
		}
		settings.setValue("savePass", ui.cbSavePassword->isChecked());
		emit signedIn(ui.edLogin->text(), ui.edPassword->text());
	}
}

void LoginWindow::onSignUpLink(const QString &str)
{
	QString link = "https://www.vpnlayer.com/signup";
	QDesktopServices::openUrl(QUrl(link));
}

void LoginWindow::loadSettings()
{
	QSettings settings;
	ui.edLogin->setText(settings.value("login", "").toString());
	ui.edPassword->setText(settings.value("password", "").toString());
	ui.cbSavePassword->setChecked(settings.value("savePass", "true").toString() == "true");
}
