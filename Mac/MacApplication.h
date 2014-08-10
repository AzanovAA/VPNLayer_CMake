#ifndef MACCOCOAAPPLICATION_H
#define MACCOCOAAPPLICATION_H

#include <QApplication.h>
#include <QMenu>
#include <QShortcut>
#include "../QtSingleApplication/qtsingleapplication.h"

class MacApplication : public QtSingleApplication
{
    Q_OBJECT
public:
    MacApplication( int& argc, char* argv[] );
    ~MacApplication();
    // This method to be public due to lack of friend classes in Objective-C and
    // the lack inheritance of Objective-C classes from C++ ones.
    void dockIconClickEvent();
    void setMainWidget(QWidget *widget) { mainWidget_ = widget; }

    void activateIgnoring();

signals:
    void setupDockEventMonitor();

private slots:
    void onSetupDockEventMonitor();
    void onSetupNotificationCenter();
private:
    class Private;
    Private* m_private;
    QWidget *mainWidget_;
};

#endif
