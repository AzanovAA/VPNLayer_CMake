TARGET   = httpservice
TEMPLATE = app
CONFIG   += console qt
QT = core network 

SOURCES  = main.cpp log.cpp
HEADERS = log.h

include(src/qtservice.pri)
#win32:LIBS += advapi32.lib
