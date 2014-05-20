#include "settingswindow.h"
#include <QSettings>
#include <QDir>
#include <QListView>

#if defined Q_OS_MAC
    #include <CoreFoundation/CoreFoundation.h>
    #include <CoreServices/CoreServices.h>
#endif

SettingsWindow::SettingsWindow(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	connect(ui.btnBack, SIGNAL(clicked()), SLOT(onBack()));
	connect(ui.lblSignUp, SIGNAL(linkActivated(const QString &)), SIGNAL(returnToSignUp()));

    ui.cbPort->setView(new QListView());
}

SettingsWindow::~SettingsWindow()
{
}

void SettingsWindow::onBack()
{
	emit back();
}

void SettingsWindow::saveSettings()
{
	QSettings settings;
	settings.setValue("savePass", ui.cbRememberPassword->isChecked());
	settings.setValue("launchOnStart", ui.cbLaunchOnStart->isChecked());
	settings.setValue("autoConnectOnStart", ui.cbAutoconnectOnStart->isChecked());
	settings.setValue("reconnectAutomatically", ui.cbReconnectAutomatically->isChecked());
	//settings.setValue("clearCookies", ui.cbClearCookies->isChecked());
    //settings.setValue("stopTraffic", ui.cbStopTraffic->isChecked());
	settings.setValue("openVPNPort", ui.cbPort->currentText());

#if defined Q_OS_WIN

	if (ui.cbLaunchOnStart->isChecked())
	{
		QSettings settingsRun("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
		QString exePath = QDir::toNativeSeparators(QApplication::applicationFilePath());
		settingsRun.setValue("VPNLayer", exePath);
	}
	else
	{
		QSettings settingsRun("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
		settingsRun.remove("VPNLayer");
	}
#elif defined Q_OS_MAC

    bool bHasAutoRunItem  = false;
    bool bNeedAutoRunItem = ui.cbLaunchOnStart->isChecked();
    CFURLRef appUrlRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());

    LSSharedFileListRef loginItems;
    loginItems = LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL );

    UInt32 seedValue;
    CFArrayRef loginArray = LSSharedFileListCopySnapshot(loginItems, &seedValue);

    LSSharedFileListItemRef loginItem = NULL;
    for (CFIndex I = 0, SZ = CFArrayGetCount(loginArray); I < SZ; ++I)
    {
        loginItem = (LSSharedFileListItemRef) CFArrayGetValueAtIndex(loginArray, I);

        CFURLRef itemUrlRef;
        if (noErr == LSSharedFileListItemResolve(loginItem, 0, &itemUrlRef, NULL))
        {
            if (CFStringCompare( CFURLGetString(itemUrlRef), CFURLGetString(appUrlRef) , 0) == 0)
            {
                if (!bNeedAutoRunItem)
                    LSSharedFileListItemRemove(loginItems, loginItem);
                else
                    bHasAutoRunItem = true;
                break;
            }
        }
    }
    if (!bHasAutoRunItem && bNeedAutoRunItem)
    {
        LSSharedFileListInsertItemURL(loginItems , kLSSharedFileListItemLast, NULL, NULL, appUrlRef, NULL, NULL );
    }

    if (loginArray) CFRelease(loginArray);

    CFRelease(appUrlRef);
    CFRelease(loginItems);

#endif

}

void SettingsWindow::loadSettings()
{
	QSettings settings;

	ui.cbRememberPassword->setChecked(settings.value("savePass", "false").toString() == "true");
	ui.cbLaunchOnStart->setChecked(settings.value("launchOnStart", "false").toString() == "true");
	ui.cbAutoconnectOnStart->setChecked(settings.value("autoConnectOnStart", "false").toString() == "true");
	ui.cbReconnectAutomatically->setChecked(settings.value("reconnectAutomatically", "false").toString() == "true");
	//ui.cbClearCookies->setChecked(settings.value("clearCookies", "true").toString() == "true");
    //ui.cbStopTraffic->setChecked(settings.value("stopTraffic", "true").toString() == "false");

	for (int i = 0; i < ui.cbPort->count(); i++)
	{
		if (ui.cbPort->itemText(i) == settings.value("openVPNPort"))
		{
			ui.cbPort->setCurrentIndex(i);
			break;
		}
	}
}
