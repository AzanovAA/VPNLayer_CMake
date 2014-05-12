#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"
#include "loginwindow.h"
#include "connectwindow.h"
#include "settingswindow.h"
#include "getservers.h"
#include <qDebug>
#include <QSystemTrayIcon>
#include <QMenu>

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
    MainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
	~MainWindow();

    void setTrayStatusConnected();
    void setTrayStatusDisconnected(bool bShowMessage = true);
    void setTrayStatusConnecting();

protected:
	void closeEvent(QCloseEvent *event);

private slots:
	void onSignIn(QString login, QString password);
	void onSettings();
	void onBack();
	void onReturnToSignUp();
    void onTimer();
    void onShow();
    void onAbout();
    void onExit();

    void onUpdateFinished(bool bSuccess, QString errorStr, bool bConfigChanged);


private:
	Ui::MainWindowClass ui;
	LoginWindow *loginWindow_;
	ConnectWindow *connectWindow_;
	SettingsWindow *settingsWindow_;
	GetServers *getServers_;
	bool autoConnect_;
    QSystemTrayIcon *trayIcon_;
    QMenu   *trayMenu_;
    QAction *actShow_, *actAbout_, *actExit_;
    QTimer *timer_;
    int state_;
};

#endif // MAINWINDOW_H
