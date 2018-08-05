#-------------------------------------------------
#
# Project created by QtCreator 2018-08-02T16:34:29
#
#-------------------------------------------------

QT       += multimedia
qtHaveModule(widgets): QT += multimediawidgets

TARGET = mdkmediaservice

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        mediaplayerservice.cpp \
        mediaplayercontrol.cpp \
        renderercontrol.cpp \
        plugin.cpp

HEADERS += \
        mediaplayerservice.h \
        mediaplayercontrol.h \
        renderercontrol.h \
        plugin.h

qtHaveModule(widgets) {
  SOURCES += videowidgetcontrol.cpp
  HEADERS += videowidgetcontrol.h
}
unix {
    target.path = /usr/lib
    INSTALLS += target
}


INCLUDEPATH += $$PWD/../../mdk-sdk/include /usr/local/include
macx {
  QMAKE_CXXFLAGS += -F$$PWD/../../mdk-sdk/lib -F/usr/local/lib
  LIBS += -F/usr/local/lib -framework mdk
} else {
  LIBS += -L$$PWD/../../mdk-sdk/lib -lmdk
}

mac {
  RPATHDIR *= @executable_path/Frameworks /usr/local/lib
  QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
  isEmpty(QMAKE_LFLAGS_RPATH): QMAKE_LFLAGS_RPATH=-Wl,-rpath,
  for(R,RPATHDIR) {
    QMAKE_LFLAGS *= \'$${QMAKE_LFLAGS_RPATH}$$R\'
  }
}


PLUGIN_TYPE = mediaservice
PLUGIN_CLASS_NAME = MDKPlugin
load(qt_plugin)

OTHER_FILES += mdk_mediaservice.json
