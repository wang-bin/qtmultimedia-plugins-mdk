# qtmultimedia-plugins-mdk
qt multimedia plugins implemented on top of [mdk-sdk](https://github.com/wang-bin/mdk-sdk)

Supports **Qt 5** (mediaservice plugin) and **Qt 6.5+** (multimedia plugin).

Qt6 architecture and data flow: [docs/qt6-plugin-design.md](docs/qt6-plugin-design.md).

## Select the backend

| Qt | Environment variable |
|----|----------------------|
| **Qt5** | `QT_MULTIMEDIA_PREFERRED_PLUGINS=mdk` (plugin key historically `mdkservice` in [`qt5/mdkmediaservice.json`](qt5/mdkmediaservice.json)) |
| **Qt6** | `QT_MEDIA_BACKEND=mdk` (plugin key `mdk`, install under `plugins/multimedia/`) |

## Features
- All formats. You can replace ffmpeg library in the sdk to support more formats
- GPU decoders (hardcoded because of qtmultimedia limitation, see [QTBUG-74393](https://bugreports.qt.io/browse/QTBUG-74393))
- QRhi-backed OpenGL / Metal / D3D / Vulkan rendering (Qt6: zero-copy when `QVideoSink::rhi()` exposes a supported backend; there is no CPU/FBO fallback)
- HDR tone mapping
- Tracks, metadata, loops, buffer ranges, `QAudioOutput` volume/mute/device routing (Qt6)

## Build Qt6 (CMake)

Requires Qt **6.5+**. Uses [mdk-sdk](https://github.com/wang-bin/mdk-sdk) via `FindMDK.cmake` (`target_link_libraries(... mdk)`).

```bash
# Extract or symlink mdk-sdk into this directory (default MDK_SDK=./mdk-sdk), then:
cmake -B build -DCMAKE_PREFIX_PATH=$HOME/Qt/6.11.0/macos
cmake --build build
cmake --install build   # installs plugin + mdk.framework into the Qt prefix
```

On macOS, `cmake --install` copies `mdk.framework` into Qt's `lib/` so the plugin can resolve `@rpath/mdk.framework`. On Linux/other Unix targets, the plugin uses `$ORIGIN/../../lib` to locate the MDK runtime installed in Qt's `lib/`. Without the runtime in the expected Qt directory, `QT_MEDIA_BACKEND=mdk` silently falls back to another backend.

Optional: `-DMDK_SDK=/path/to/mdk-sdk` if the SDK is not at `./mdk-sdk`.

Optional: `-DMDK_USE_QT_RHI_TEXTURE_POOL=ON` enables independent Qt RHI frame-slot textures with Qt 6.8.2 or later. It defaults to `OFF`; Qt 6.5 through 6.8.1 use the legacy shared-target path.

Enable: `export QT_MEDIA_BACKEND=mdk` and run a Qt Multimedia example.

## Build Qt5 (qmake)

- Download an SDK from https://sourceforge.net/projects/mdk-sdk/files/nightly or https://github.com/wang-bin/mdk-sdk/releases
- Extract (or symlink) as `mdk-sdk` in this dir
- Build [`qt5/mdkmediaservice.pro`](qt5/mdkmediaservice.pro) and install. In Qt Creator you can add a **Make** step with **Make arguments: install**. Plugin and mdk runtime files install into the Qt dir.
- Enable with `QT_MULTIMEDIA_PREFERRED_PLUGINS=mdk`

>> Note: mdk-sdk-apple.tar.xz contains xcframework which is not supported yet; use mdk-sdk-macOS.tar.xz or mdk-sdk-iOS.tar.xz instead.

## Issues
- Qt for Android adds an ABI suffix to shared libraries for multi-ABI; libmdk has no such suffix
- Qt6 `QIODevice` playback uses `stream:` + `appendBuffer` (no MediaIO in the public SDK)
- Audio device selection uses property `"audio.device"` (`setAudioDevice` / `audioDevices` / `onError` / `activeTracks` getter are not in the current packaged SDK headers yet)
