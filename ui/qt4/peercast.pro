TEMPLATE = app

FORMS = mainform.ui

DEPENDPATH += .
INCLUDEPATH += . ../../core ../../core/common

unix {
 QMAKE_CXXFLAGS += -std=c++11
 LIBS += -lboost_filesystem -lboost_system -lboost_regex
 DEFINES += _UNIX
 SOURCES += ../../core/unix/compat.cpp ../../core/unix/usys.cpp ../../core/unix/usocket.cpp ../../core/unix/utf8.cpp
}

win32 {
 LIBS += -lwsock32
 DEFINES += WIN32 QT
 SOURCES += ../../core/win32/wsys.cpp ../../core/win32/wsocket.cpp ../../core/win32/utf8.c
}

macx {
 LIBS += 
 DEFINES += _APPLE
 ICON = peercast.icns
 TARGET = PeerCast
 CONFIG += x86 ppc sdk
 QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.4u.sdk

 system(mkdir -p PeerCast.app/Contents/MacOS)
 system(cp -r ../html PeerCast.app/Contents/MacOS)
 system(cp peercast.xpm PeerCast.app/Contents/MacOS)
}

QT += qt3support
CONFIG += qt warn_off release 

SOURCES += ../../core/common/socket.cpp ../../core/common/servent.cpp ../../core/common/servhs.cpp ../../core/common/servmgr.cpp ../../core/common/xml.cpp ../../core/common/stream.cpp ../../core/common/sys.cpp ../../core/common/gnutella.cpp ../../core/common/html.cpp ../../core/common/channel.cpp ../../core/common/http.cpp ../../core/common/inifile.cpp ../../core/common/peercast.cpp ../../core/common/stats.cpp ../../core/common/mms.cpp ../../core/common/mp3.cpp ../../core/common/nsv.cpp ../../core/common/ogg.cpp ../../core/common/url.cpp ../../core/common/icy.cpp ../../core/common/pcp.cpp ../../core/common/jis.cpp ../../core/common/admin.cpp ../../core/common/localfs.cpp ../../core/common/template.cpp ../../core/common/cgi.cpp ../../core/common/str.cpp ../../core/common/util.cpp

HEADERS += gui.h listitem.h main.h
SOURCES += gui.cpp listitem.cpp main.cpp

FORMS += 
