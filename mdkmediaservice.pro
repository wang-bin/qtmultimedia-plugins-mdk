QT += multimedia
qtHaveModule(widgets): QT += multimediawidgets

# rtti is disabled in mdk
CONFIG += rtti_off c++1z c++17
gcc:isEmpty(QMAKE_CXXFLAGS_RTTI_ON): QMAKE_CXXFLAGS += -fno-rtti

QTDIR_build {
    # This is only for the Qt build. Do not use externally. We mean it.
    PLUGIN_TYPE = mediaservice
    PLUGIN_CLASS_NAME = MDKPlugin
    load(qt_plugin)
} else {
    TARGET = $$qtLibraryTarget(mdkmediaservice)
    TEMPLATE = lib
    CONFIG += plugin
    target.path = $$[QT_INSTALL_PLUGINS]/mediaservice
    INSTALLS += target
}

SOURCES += \
    iodevice.cpp \
    mediaplayerservice.cpp \
    mediaplayercontrol.cpp \
    metadatareadercontrol.cpp \
    renderercontrol.cpp \
    mdkmediaserviceplugin.cpp

HEADERS += \
    iodevice.h \
    mediaplayerservice.h \
    mediaplayercontrol.h \
    metadatareadercontrol.h \
    renderercontrol.h \
    mdkmediaserviceplugin.h

qtHaveModule(widgets) {
    SOURCES += videowidgetcontrol.cpp
    HEADERS += videowidgetcontrol.h
}

INCLUDEPATH += $$PWD/mdk-sdk/include
macx {
    QMAKE_CXXFLAGS += -F$$PWD/mdk-sdk/lib -F/usr/local/lib
    LIBS += -F$$PWD/mdk-sdk/lib -F/usr/local/lib -framework mdk
} else {
    LIBS += -L$$PWD/mdk-sdk/lib -L$$[QT_INSTALL_LIBS] -lmdk
}

mac {
    RPATHDIR *= @executable_path/Frameworks /usr/local/lib
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
    isEmpty(QMAKE_LFLAGS_RPATH): QMAKE_LFLAGS_RPATH = -Wl,-rpath,
    for(R,RPATHDIR) {
        QMAKE_LFLAGS *= \'$${QMAKE_LFLAGS_RPATH}$$R\'
    }
}

OTHER_FILES += mdkmediaservice.json
