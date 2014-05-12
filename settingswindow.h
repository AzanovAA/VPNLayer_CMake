#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QWidget>
#include "ui_settingswindow.h"

class SettingsWindow : public QWidget
{
	Q_OBJECT

public:
	SettingsWindow(QWidget *parent = 0);
	~SettingsWindow();
	void loadSettings();
	void saveSettings();

signals:
	void back();
	void returnToSignUp();

private slots:
	void onBack();

private:
	Ui::SettingsWindow ui;
};

#endif // SETTINGSWINDOW_H
