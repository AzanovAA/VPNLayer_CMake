#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include "ui_loginwindow.h"

class LoginWindow : public QWidget
{
	Q_OBJECT

public:
	LoginWindow(QWidget *parent = 0);
	~LoginWindow();
	void loadSettings();

signals:
	void signedIn(QString login, QString password);

private slots:
	void onSignIn();
	void onSignUpLink(const QString &str);

private:
	Ui::LoginWindow ui;
};

#endif // LOGINWINDOW_H
