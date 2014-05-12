cmake_minimum_required( VERSION 2.8.10 )

SET(PROJECT_NAME "VPNLayer")
PROJECT(${PROJECT_NAME})

SET( QT_USE_QTNETWORK true )

IF (WIN32)
   SET(PLATFORM_SOURCES Windows/connection.cpp Windows/qopenvpn.cpp)
   SET(PLATFORM_HEADERS Windows/connection.h Windows/qopenvpn.h)
   SET(PLATFORM_RC VpnLayer.rc) 
ELSE(APPLE)
   FIND_LIBRARY( COREFOUNDATION_LIBRARY      CoreFoundation )
   FIND_LIBRARY( CORESERVICES_LIBRARY        CoreServices )
   FIND_LIBRARY( SECURITY_LIBRARY            Security )
   FIND_LIBRARY( SYSTEMCONFIGURATION_LIBRARY SystemConfiguration )
   FIND_LIBRARY( APPKIT_LIBRARY AppKit )

   SET(PLATFORM_SOURCES Mac/connection.cpp Mac/qopenvpn.cpp Mac/MacApplication.mm)
   SET(PLATFORM_HEADERS Mac/connection.h Mac/qopenvpn.h Mac/MacApplication.h)
   SET(EXTRA_LIBS ${COREFOUNDATION_LIBRARY}
                ${CORESERVICES_LIBRARY}
                ${SECURITY_LIBRARY}
                ${SYSTEMCONFIGURATION_LIBRARY}
                ${APPKIT_LIBRARY})
ENDIF()


SET(VPNLAYER_SOURCES connectwindow.cpp getservers.cpp log.cpp loginwindow.cpp main.cpp mainwindow.cpp settingswindow.cpp showlog.cpp downloadfile.cpp QtSingleApplication/qtlocalpeer.cpp QtSingleApplication/qtsingleapplication.cpp QtSingleApplication/qtsinglecoreapplication.cpp ${PLATFORM_SOURCES})
SET(VPNLAYER_HEADERS connectwindow.h getservers.h log.h loginwindow.h mainwindow.h settings.h settingswindow.h showlog.h downloadfile.h utils.h QtSingleApplication/qtlocalpeer.h QtSingleApplication/qtsingleapplication.h QtSingleApplication/qtsinglecoreapplication.h ${PLATFORM_HEADERS} )
SET(VPNLAYER_FORMS connectwindow.ui loginwindow.ui mainwindow.ui settingswindow.ui showlog.ui)
SET(VPNLAYER_RESOURCES mainwindow.qrc)

IF (WIN32)
   SET(APP_TYPE WIN32)
   SET(CMAKE_CXX_FLAGS "/Zc:wchar_t-")
   SET(CMAKE_EXE_LINKER_FLAGS "/MANIFEST:NO")
ELSE(APPLE)
   SET(APP_TYPE MACOSX_BUNDLE)
ENDIF()

FIND_PACKAGE(Qt4 REQUIRED)

QT4_WRAP_CPP( VPNLAYER_HEADERS_MOC ${VPNLAYER_HEADERS} )
QT4_WRAP_UI       ( VPNLAYER_FORMS_HEADERS  ${VPNLAYER_FORMS} )
QT4_ADD_RESOURCES ( VPNLAYER_RESOURCES_RCC ${VPNLAYER_RESOURCES} )

INCLUDE( ${QT_USE_FILE} )
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

ADD_EXECUTABLE( ${PROJECT_NAME} ${APP_TYPE} ${VPNLAYER_SOURCES} ${VPNLAYER_HEADERS_MOC} ${VPNLAYER_FORMS_HEADERS} ${VPNLAYER_RESOURCES_RCC} ${PLATFORM_RC} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${QT_LIBRARIES} ${EXTRA_LIBS})

IF (APPLE)

   ADD_EXECUTABLE( start    ${CMAKE_SOURCE_DIR}/Mac/Helpers/start.c )
   ADD_EXECUTABLE( stop     ${CMAKE_SOURCE_DIR}/Mac/Helpers/stop.c )
   ADD_EXECUTABLE( settings ${CMAKE_SOURCE_DIR}/Mac/Helpers/settings.c)

   FILE( COPY        ${CMAKE_SOURCE_DIR}/Mac/Kext
      DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources/ )

   FILE( COPY        ${CMAKE_SOURCE_DIR}/Mac/icon.icns
      DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources/ )

   FILE( COPY        ${CMAKE_SOURCE_DIR}/Mac/Bin
      DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources/ )

   SET( MACOSX_BUNDLE_ICON_FILE "icon.icns" )
   SET_TARGET_PROPERTIES( ${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME} )
   SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Mac/Info.plist)

SET_TARGET_PROPERTIES( start     PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CONFIGURATION}/${PROJECT_NAME}.app/Contents/Resources/Bin )
SET_TARGET_PROPERTIES( stop      PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CONFIGURATION}/${PROJECT_NAME}.app/Contents/Resources/Bin )
SET_TARGET_PROPERTIES( settings  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CONFIGURATION}/${PROJECT_NAME}.app/Contents/Resources/Bin )

   ADD_CUSTOM_COMMAND( TARGET ${PROJECT_NAME} COMMAND macdeployqt ARGS ${CMAKE_CURRENT_BINARY_DIR}/\${CONFIGURATION}/${PROJECT_NAME}.app )

   ADD_EXECUTABLE( pptpl2tp ${CMAKE_SOURCE_DIR}/Mac/Helpers/pptpl2tp.cpp)
   TARGET_LINK_LIBRARIES( pptpl2tp ${EXTRA_LIBS})
  

ENDIF()
