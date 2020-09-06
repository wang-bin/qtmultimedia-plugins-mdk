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

OTHER_FILES += mdkmediaservice.json

######## MDK SDK ##########
MDK_SDK = $$PWD/mdk-sdk
INCLUDEPATH += $$MDK_SDK/include
contains(QT_ARCH, x.*64) {
  android: MDK_ARCH = x86_64
  else: MDK_ARCH = x64
} else:contains(QT_ARCH, .*86) {
  MDK_ARCH = x86
} else:contains(QT_ARCH, a.*64) {
  android: MDK_ARCH = arm64-v8a
  else: MDK_ARCH = arm64
} else:contains(QT_ARCH, arm.*) {
  android: MDK_ARCH = armeabi-v7a
  else: MDK_ARCH = arm
}

mdk_runtime.path = $$[QT_INSTALL_LIBS]
macx|ios {
  MDK_ARCH=
  QMAKE_CXXFLAGS += -F$$MDK_SDK/lib
  LIBS += -F$$MDK_SDK/lib -framework mdk
  mdk_runtime.files = $$MDK_SDK/lib/*
} else {
  LIBS += -L$$MDK_SDK/lib/$$MDK_ARCH -lmdk
  win32: {
    LIBS += -L$$MDK_SDK/bin/$$MDK_ARCH # qtcreator will prepend $$LIBS to PATH to run targets
    mdk_runtime.files = $$MDK_SDK/bin/$$MDK_ARCH/mdk.* $$MDK_SDK/bin/$$MDK_ARCH/ffmpeg-?.dll
    mdk_runtime.path = $$[QT_INSTALL_BINS]
  } else {
    mdk_runtime.files = $$MDK_SDK/lib/$$MDK_ARCH/libmdk.* $$MDK_SDK/lib/$$MDK_ARCH/libffmpeg.*
  }
}
linux: LIBS += -Wl,-rpath-link,$$MDK_SDK/lib/$$MDK_ARCH # for libc++ symbols
INSTALLS += mdk_runtime

mac {
    RPATHDIR *= @executable_path/Frameworks $$[QT_INSTALL_LIBS] /usr/local/lib
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
    isEmpty(QMAKE_LFLAGS_RPATH): QMAKE_LFLAGS_RPATH = -Wl,-rpath,
    for(R,RPATHDIR) {
        QMAKE_LFLAGS *= \'$${QMAKE_LFLAGS_RPATH}$$R\'
    }
}

