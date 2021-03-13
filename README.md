# qtmultimedia-plugins-mdk
qt multimedia plugins implemented on top of [mdk-sdk](https://github.com/wang-bin/mdk-sdk)

Multimedia plugins are looked up in alphabetical order, so mdk plugin may be not selected. Since Qt5.13 environment var `QT_MULTIMEDIA_PREFERRED_PLUGINS=mdk` can enable mdk as backend.

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

## Issues
- Android install dir is different, you may need to copy so files manually to `plugins/mediaservice`,  `libmdk.so` and `libffmpeg.so` to `lib`
- Qt for android adds abi suffix to shared libraries to support muliti abi, libmdk has no such suffix
