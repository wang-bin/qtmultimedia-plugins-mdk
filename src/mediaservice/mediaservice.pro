#-------------------------------------------------
#
# Project created by QtCreator 2018-08-02T16:34:29
#
#-------------------------------------------------
TARGET = mdkmediaservice
TEMPLATE = lib
CONFIG += plugin
QT       += multimedia
qtHaveModule(widgets): QT += multimediawidgets

# rtti is disabled in mdk
CONFIG += rtti_off c++17

target.path = $$[QT_INSTALL_PLUGINS]
INSTALLS += target

SOURCES += \
        iodevice.cpp \
        mediaplayerservice.cpp \
        mediaplayercontrol.cpp \
        metadatareadercontrol.cpp \
        renderercontrol.cpp \
        plugin.cpp

HEADERS += \
        iodevice.h \
        mediaplayerservice.h \
        mediaplayercontrol.h \
        metadatareadercontrol.h \
        renderercontrol.h \
        plugin.h

qtHaveModule(widgets) {
  SOURCES += videowidgetcontrol.cpp
  HEADERS += videowidgetcontrol.h
}


INCLUDEPATH += $$PWD/../../mdk-sdk/include
macx {
  QMAKE_CXXFLAGS += -F$$PWD/../../mdk-sdk/lib -F/usr/local/lib
  LIBS += -F/usr/local/lib -framework mdk
} else {
  LIBS += -L$$PWD/../../mdk-sdk/lib -L$$[QT_INSTALL_LIBS] -lmdk
}


mac {
  RPATHDIR *= @executable_path/Frameworks /usr/local/lib
  QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
  isEmpty(QMAKE_LFLAGS_RPATH): QMAKE_LFLAGS_RPATH=-Wl,-rpath,
  for(R,RPATHDIR) {
    QMAKE_LFLAGS *= \'$${QMAKE_LFLAGS_RPATH}$$R\'
  }
}

OTHER_FILES += plugin.json
