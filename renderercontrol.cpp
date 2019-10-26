/*
 * Copyright (C) 2018-2019 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
// GLTextureVideoBuffer render to fbo texture, texture as frame
// map to host: store tls ui context in map(GLTextureArray), create tls context shared with ui ctx, download
// move to mdk public NativeVideoBuffer::fromTexture()
#include "renderercontrol.h"
#include "mediaplayercontrol.h"
#include "mdk/MediaInfo.h"
#include <QAbstractVideoBuffer>
#include <QAbstractVideoSurface>
#include <QVideoFrame>
#include <QVideoSurfaceFormat>
#include <QImage>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLContext>

#ifdef MDK_ABI
# include "mdk/VideoFrame.h"
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
#endif // MDK_ABI

// qtmultimedia only support packed rgb gltexture handle, so offscreen rendering may be required
class FBOVideoBuffer final : public QAbstractVideoBuffer {
public:
    FBOVideoBuffer(Player* player, QOpenGLFramebufferObject** fbo, int width, int height) : QAbstractVideoBuffer(GLTextureHandle)
        , w_(width), h_(height)
        , player_(player)
        , fbo_(fbo)
    {}

    MapMode mapMode() const override { return mode_; }

    uchar *map(MapMode mode, int *numBytes, int *bytesPerLine) override {
        if (mode_ == mode)
            return img_.bits();
        if (mode & WriteOnly)
            return nullptr;
        mode_ = mode;
        renderToFbo();
        img_ = (*fbo_)->toImage(false);
        if (numBytes)
            *numBytes = img_.byteCount();
        if (bytesPerLine)
            *bytesPerLine = img_.bytesPerLine();
        return img_.bits();
    }

    void unmap() override {
        mode_ = NotMapped;
    }

// current context is not null!
    QVariant handle() const override {
        auto that = const_cast<FBOVideoBuffer*>(this);
        that->renderToFbo();
        return (*fbo_)->texture();
    }

private:
    void renderToFbo() {
        auto f = QOpenGLContext::currentContext()->functions();
        GLint prevFbo = 0;
        f->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
        auto fbo = *fbo_;
        if (!fbo || fbo->size() != QSize(w_, h_)) {
            player_->scale(1.0f, -1.0f); // flip y in fbo
            player_->setVideoSurfaceSize(w_, h_);
            delete fbo;
            fbo = new QOpenGLFramebufferObject(w_, h_);
            *fbo_ = fbo;
        }
        fbo->bind();
        player_->renderVideo();
        f->glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
    }
private:
    MapMode mode_ = NotMapped;
    int w_;
    int h_;
    Player* player_;
    QOpenGLFramebufferObject** fbo_ = nullptr;
    QImage img_;
};

RendererControl::RendererControl(MediaPlayerControl* player, QObject* parent) : QVideoRendererControl(parent)
    , mpc_(player)
{
    mpc_ = player;
    connect(mpc_, &MediaPlayerControl::frameAvailable, this, &RendererControl::onFrameAvailable);
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
        mpc_->player()->setRenderCallback(nullptr); // surfcace is set to null before destroy, avoid invokeMethod() on invalid this
        return;
    }
    //const QSize r = surface->nativeResolution(); // may be (-1, -1)
    // mdk player needs a vo. add before delivering a video frame
    mpc_->player()->setVideoSurfaceSize(1,1);//r.width(), r.height());

    if (!mpc_->player()->mediaInfo().video.empty()) {
        auto& c = mpc_->player()->mediaInfo().video[0].codec;
        video_w_ = c.width;
        video_h_ = c.height;
    }

    mpc_->player()->onMediaStatusChanged([this](MediaStatus s){
        if (flags_added(status_, s, MediaStatus::Loaded)) {
            if (!mpc_->player()->mediaInfo().video.empty()) {
                auto& c = mpc_->player()->mediaInfo().video[0].codec;
                video_w_ = c.width;
                video_h_ = c.height;
            }
        }
        status_ = s;
        return true;
    });
}

void RendererControl::onFrameAvailable()
{
    if (!surface_)
        return;
#ifdef MDK_ABI
    mpc_->player()->renderVideo(); // required to get frame
    VideoFrame v;
    mpc_->player()->getVideoFrame(&v);
    if (!v)
        return;
    const auto& qfmt = toQt(v.format().format());
    QVideoFrame frame;
    if (v.nativeBuffer() || !surface_->isFormatSupported(QVideoSurfaceFormat(QSize(v.width(), v.height()), qfmt))) {
        frame = QVideoFrame(new FBOVideoBuffer(mpc_->player(), &fbo_, v.width(), v.height()), QSize(v.width(), v.height()), QVideoFrame::Format_BGR32); // RGB32 for qimage
    } else {
        frame = QVideoFrame(new HostVideoBuffer(v), QSize(v.width(), v.height()), qfmt);
    }
#else
    if (video_w_ <= 0 || video_h_ <= 0)
        return; // not playing, e.g. when stop() is called there is also a frameAvailable signal to update renderer which is required by mdk internally. if create fbo with an invalid size anyway, qt gl rendering will be broken forever
    QVideoFrame frame(new FBOVideoBuffer(mpc_->player(), &fbo_, video_w_, video_h_), QSize(video_w_, video_h_), QVideoFrame::Format_BGR32); // RGB32 for qimage
#endif // MDK_ABI
    if (!surface_->isActive()) { // || surfaceFormat()!=
        QVideoSurfaceFormat format(frame.size(), frame.pixelFormat(), frame.handleType());
        surface_->start(format);
    }
    surface_->present(frame); // main thread
}
