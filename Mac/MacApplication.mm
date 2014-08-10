#include <Cocoa/Cocoa.h>

#include "MacApplication.h"

#include <QShortcut>
#include <QShortcutEvent>
#include <QDebug>

extern void qt_mac_set_dock_menu(QMenu*);

@interface DockIconClickEventHandler : NSObject
{
@public
    MacApplication* macApplication;
}
- (void)handleDockClickEvent:(NSAppleEventDescriptor*)event withReplyEvent:(NSAppleEventDescriptor*)replyEvent;
@end

@implementation DockIconClickEventHandler
- (void)handleDockClickEvent:(NSAppleEventDescriptor*)event withReplyEvent:(NSAppleEventDescriptor*)replyEvent {
    if (macApplication)
        macApplication->dockIconClickEvent();
}
@end

//@interface NotificationCenterDelegate : NSObject <NSUserNotificationCenterDelegate>
//@end


//@implementation NotificationCenterDelegate
//- (BOOL)userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification
//{
//    return YES;
//}
//@end

class MacApplication::Private {
public:
    Private();
    ~Private();
    NSEvent* cocoaEventFilter( NSEvent* incomingEvent );
    void setupCocoaEventHandler() const;

    NSAutoreleasePool* pool;
    DockIconClickEventHandler* dockIconClickEventHandler;
    //NotificationCenterDelegate *notificationCenterDelegate;
};

MacApplication::Private::Private()
    : pool( 0 ), dockIconClickEventHandler( 0 )
{
    pool = [[NSAutoreleasePool alloc] init];
    dockIconClickEventHandler = [[DockIconClickEventHandler alloc] init];
    //notificationCenterDelegate = [[NotificationCenterDelegate alloc] init];
}

MacApplication::Private::~Private()
{
    [pool drain];
}


void MacApplication::Private::setupCocoaEventHandler() const
{
    // TODO: This apparently uses a legacy API and we should be using the
    // applicationShouldHandleReopen:hasVisibleWindows: method on
    // NSApplicationDelegate but this isn't possible without nasty runtime
    // reflection hacks until Qt is fixed. If this breaks, shout at them :)
    [[NSAppleEventManager sharedAppleEventManager]
     setEventHandler:dockIconClickEventHandler
     andSelector:@selector(handleDockClickEvent:withReplyEvent:)
     forEventClass:kCoreEventClass
     andEventID:kAEReopenApplication];
}

void MacApplication::onSetupDockEventMonitor()
{
    m_private->setupCocoaEventHandler();
}

void MacApplication::onSetupNotificationCenter()
{
    // [[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:m_private->notificationCenterDelegate];
}


MacApplication::MacApplication( int& argc, char* argv[] )
    : QtSingleApplication( argc, argv )
    , m_private( new MacApplication::Private() )
    , mainWidget_(NULL)

{
    m_private->dockIconClickEventHandler->macApplication = this;

    connect(this, SIGNAL(setupDockEventMonitor()), SLOT(onSetupDockEventMonitor()), Qt::QueuedConnection);
   // connect(this, SIGNAL(setupDockEventMonitor()), SLOT(onSetupNotificationCenter()), Qt::QueuedConnection);
    emit setupDockEventMonitor();
}

MacApplication::~MacApplication()
{
    delete m_private;
}


void MacApplication::dockIconClickEvent()
{
    if (mainWidget_)
    {
        mainWidget_->show();
        mainWidget_->setWindowState(mainWidget_->windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
    }
}

void MacApplication::activateIgnoring()
{
    [NSApp activateIgnoringOtherApps:YES];
}

