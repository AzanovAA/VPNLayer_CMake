#include "mainwindow.h"
#include <QApplication>
#include <QSharedMemory>
#include <QMessageBox>
#include <QNetworkConfigurationManager>
#include <QTime>

#if defined Q_OS_WIN
#include "QtSingleApplication/qtsingleapplication.h"
#else
#include "Mac/MacApplication.h"
#endif


int main(int argc, char *argv[])
{
#if defined Q_OS_WIN
    QtSingleApplication a(argc, argv);
#else
    MacApplication a(argc, argv);
#endif

    //if (a.isRunning())
      //  return !a.sendMessage(QObject::tr("Can't start more than one instance of the application."));

   /* QNetworkConfigurationManager networkConfig_;

	// wait for network connection maximum 60 sec, only if autostart enabled
	{
		QTime time;
		time.start();
		while (!networkConfig_.isOnline())
		{
#if defined Q_OS_WIN
            Sleep(10);
#else
            sleep(10);
#endif
			if (time.elapsed() > 60000)
			{
				QMessageBox msgBox;
				msgBox.setText( QObject::tr("No active internet connection. Please check.") );
				msgBox.setIcon( QMessageBox::Information );
				msgBox.exec();
				return 0;
			}
		}
    }*/

	MainWindow w;
	w.show();

#if defined Q_OS_MAC
   a.setMainWidget(&w);
#endif
    return a.exec();
}
