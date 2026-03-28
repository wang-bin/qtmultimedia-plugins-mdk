# qtmultimedia-plugins-mdk
qt multimedia plugins implemented on top of [mdk-sdk](https://github.com/wang-bin/mdk-sdk)

Supports **Qt 5** (mediaservice plugin) and **Qt 6.4+** (multimedia plugin).

For Qt5: Multimedia plugins are looked up in alphabetical order, so mdk plugin may be not selected. Since Qt5.13 environment var `QT_MULTIMEDIA_PREFERRED_PLUGINS=mdk` can enable mdk as backend.

> **Note for users upgrading from older builds:** the plugin key was renamed from `mdkservice` to `mdk`. Update `QT_MULTIMEDIA_PREFERRED_PLUGINS` accordingly.

For Qt6: Set environment variable `QT_MEDIA_BACKEND=mdk` to select the mdk backend.

## Features
- All formats. You can replace ffmpeg library in the sdk to support more formats
- GPU decoders (hardcoded because of qtmutimedia limitation, see [QTBUG-74393](https://bugreports.qt.io/browse/QTBUG-74393)
- Optimized OpenGL rendering
- HDR tone mapping

## Build
- Download a sdk from https://sourceforge.net/projects/mdk-sdk/files/nightly or https://github.com/wang-bin/mdk-sdk/releases
- Extract sdk into this dir
- Build and install. In QtCreator you can add a **Make** step with ***Make arguments: install***. Then plugin and mdk runtime files will be automatically installed to Qt dir
- Try an Qt multimedia example

The build system automatically detects whether Qt5 or Qt6 is used:
- **Qt5**: plugin is installed to `<Qt>/plugins/mediaservice/`
- **Qt6**: plugin is installed to `<Qt>/plugins/multimedia/`

>> Note: mdk-sdk-apple.tar.xz contains xcframework which is not supported yet in this project, use mdk-sdk-macOS.tar.xz or mdk-sdk-iOS.tar.xz instead

## Issues
- Qt for android adds abi suffix to shared libraries to support muliti abi, libmdk has no such suffix
