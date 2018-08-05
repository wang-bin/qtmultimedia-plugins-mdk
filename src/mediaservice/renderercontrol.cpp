/*
 * Copyright (C) 2018 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
// GLTextureVideoBuffer render to fbo texture, texture as frame
// map to host: store tls ui context in map(GLTextureArray), create tls context shared with ui ctx, download
// move to mdk public NativeVideoBuffer::fromTexture()
#include "renderercontrol.h"
#include "mediaplayercontrol.h"
#include "mdk/VideoFrame.h"
#include <QAbstractVideoBuffer>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>
#include <iostream>

static QVideoFrame::PixelFormat toQt(PixelFormat fmt) {
    switch (fmt) {
    case PixelFormat::YUV420P: return QVideoFrame::Format_YUV420P;
    case PixelFormat::UYVY422: return QVideoFrame::Format_UYVY;
    case PixelFormat::NV12: return QVideoFrame::Format_NV12;
    case PixelFormat::YV12: return QVideoFrame::Format_YV12;
    case PixelFormat::BGRA: return QVideoFrame::Format_BGRA32;
    case PixelFormat::BGRX: return QVideoFrame::Format_BGR32;
    default: return QVideoFrame::Format_Invalid;
    }
}

class HostVideoBuffer final: public QAbstractPlanarVideoBuffer
{
public:
    HostVideoBuffer(const VideoFrame& frame)
        : QAbstractPlanarVideoBuffer(NoHandle)
        , frame_(frame)
    {
    }

    MapMode mapMode() const override { return mode_; }

    int map(MapMode mode, int *numBytes, int bytesPerLine[4], uchar *data[4]) override {
        const int planes = frame_.format().planeCount();
        if (mode_ == mode)
            return planes;
        mode_ = mode;
        int bytes = 0;
        for (int i = 0; i < planes; ++i) {
            auto b = frame_.buffer(i);
            bytes += b->size();
            bytesPerLine[i] = b->stride();
            if (mode & WriteOnly) {
                data[i] = b->data();
            } else {
                data[i] = (uchar*)b->constData();
            }
        }
        if (numBytes)
            *numBytes = bytes;
        return planes;
    }

    void unmap() override {
        mode_ = NotMapped;
    }
private:
    MapMode mode_ = NotMapped;
    VideoFrame frame_;
};
/*
// TODO: qtmultimedia only support packed rgb gltexture handle, so offscreen rendering may be required
class HWVideoBuffer final: public QAbstractVideoBuffer {
public:
    HWVideoBuffer(NativeVideoBufferRef b) : QAbstractVideoBuffer(GLTextureHandle) {}
};
*/
RendererControl::RendererControl(MediaPlayerControl* player, QObject* parent) : QVideoRendererControl(parent)
    , mpc_(player)
{
    mpc_ = player;
}

QAbstractVideoSurface* RendererControl::surface() const
{
    return surface_;
}

void RendererControl::setSurface(QAbstractVideoSurface* surface)
{
    if (surface_ && surface_->isActive())
        surface_->stop();
    surface_ = surface;
    if (!surface) {
        mpc_->player()->setRenderCallback(nullptr);
        return;
    }
    mpc_->player()->setRenderCallback([this]{
        mpc_->player()->renderVideo(); // required to get frame
        VideoFrame v;
        mpc_->player()->getVideoFrame(&v);
        if (!v)
            return;
        // surface()->isFormatSupported()
        if (v.nativeBuffer()) {
            // offscreen rendering if necessary. rgb.
        } else {
            frame_ = QVideoFrame(new HostVideoBuffer(v), QSize(v.width(), v.height()), toQt(v.format().format()));
        }
        QMetaObject::invokeMethod(this, "presentInUIThread", Qt::AutoConnection);
    });
    const QSize r = surface->nativeResolution();
    // mdk player needs a vo. add before delivering a video frame
    mpc_->player()->setVideoSurfaceSize(r.width(), r.height());
    // connect(nativeResolutionChanged) // not important
}

void RendererControl::presentInUIThread()
{
    if (!surface_->isActive()) { // || surfaceFormat()!=
        QVideoSurfaceFormat format(frame_.size(), frame_.pixelFormat(), frame_.handleType());
        surface_->start(format);
    }
    surface_->present(frame_);
}