/*
 * Copyright (C) 2022 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#include "mdkplayercontrol.h"
#include "mdk/MediaInfo.h"
#include "mdk/global.h"
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QTimer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <QMetaObject>
#ifdef MDK_ABI
# include "mdk/VideoFrame.h"
#endif

using namespace MDK_NS;

static QMediaPlayer::PlaybackState toQt(State value)
{
    switch (value) {
    case State::Playing: return QMediaPlayer::PlayingState;
    case State::Paused:  return QMediaPlayer::PausedState;
    case State::Stopped: return QMediaPlayer::StoppedState;
    }
    return QMediaPlayer::StoppedState;
}

static QMediaPlayer::MediaStatus toQt(MediaStatus value)
{
    switch (value) {
    case MediaStatus::NoMedia:  return QMediaPlayer::NoMedia;
    case MediaStatus::Invalid:  return QMediaPlayer::InvalidMedia;
    default: break;
    }
    if (test_flag(value & MediaStatus::Loading))
        return QMediaPlayer::LoadingMedia;
    if (test_flag(value & MediaStatus::Stalled))
        return QMediaPlayer::StalledMedia;
    if (test_flag(value & MediaStatus::Buffering))
        return QMediaPlayer::BufferingMedia;
    if (test_flag(value & MediaStatus::Buffered))
        return QMediaPlayer::BufferedMedia;
    if (test_flag(value & MediaStatus::End))
        return QMediaPlayer::EndOfMedia;
    if (test_flag(value & MediaStatus::Loaded))
        return QMediaPlayer::LoadedMedia;
    return QMediaPlayer::NoMedia;
}

#ifdef MDK_ABI
static QVideoFrameFormat::PixelFormat toQt(PixelFormat fmt)
{
    switch (fmt) {
    case PixelFormat::YUV420P: return QVideoFrameFormat::Format_YUV420P;
    case PixelFormat::UYVY422: return QVideoFrameFormat::Format_UYVY;
    case PixelFormat::NV12:    return QVideoFrameFormat::Format_NV12;
    case PixelFormat::YV12:    return QVideoFrameFormat::Format_YV12;
    case PixelFormat::BGRA:    return QVideoFrameFormat::Format_BGRA8888;
    case PixelFormat::BGRX:    return QVideoFrameFormat::Format_BGRX8888;
    default:                   return QVideoFrameFormat::Format_Invalid;
    }
}
#endif // MDK_ABI

MDKPlayerControl::MDKPlayerControl(QMediaPlayer *player)
    : QObject(player)
    , QPlatformMediaPlayer(player)
{
    player_.setVideoDecoders({
#if defined(Q_OS_WIN)
        "MFT:d3d=11", "MFT:d3d=9", "D3D11",
#elif defined(Q_OS_DARWIN)
        "VT", "VideoToolbox",
#elif defined(Q_OS_ANDROID)
        "AMediaCodec:java=0:async=1",
#elif defined(Q_OS_LINUX)
        "VAAPI", "VDPAU",
#endif
        "CUDA", "FFmpeg"});

    player_.onStateChanged([this](State value) {
        QMetaObject::invokeMethod(this, [this, value] {
            stateChanged(toQt(value));
            if (toQt(value) == QMediaPlayer::PlayingState)
                positionTimer_->start();
            else
                positionTimer_->stop();
        }, Qt::QueuedConnection);
    });

    // Single media-status handler: tracks video dimensions AND notifies Qt.
    // setVideoSink() must NOT replace this handler.
    player_.onMediaStatusChanged([this](MediaStatus value) {
        if (flags_added(status_, value, MediaStatus::Loaded)) {
            if (!player_.mediaInfo().video.empty()) {
                auto &c = player_.mediaInfo().video[0].codec;
                video_w_ = c.width;
                video_h_ = c.height;
            }
        }
        status_ = value;
        QMetaObject::invokeMethod(this, [this, value] {
            mediaStatusChanged(toQt(value));
        }, Qt::QueuedConnection);
        return true;
    });

    player_.onEvent([this](const MediaEvent &e) {
        if (e.error < 0) {
            if (e.category == "decoder.audio" || e.category == "decoder.video") {
                QMetaObject::invokeMethod(this, [this] {
                    error(QMediaPlayer::FormatError,
                          tr("Unsupported media, a codec is missing."));
                }, Qt::QueuedConnection);
            }
        }
        return false;
    });

    positionTimer_ = new QTimer(this);
    positionTimer_->setInterval(100);
    connect(positionTimer_, &QTimer::timeout, this, [this] {
        positionChanged(player_.position());
    });
}

MDKPlayerControl::~MDKPlayerControl()
{
    // Clear MDK callbacks before any destruction to avoid callbacks on a dead object.
    player_.setRenderCallback(nullptr);
    player_.onMediaStatusChanged(nullptr);
    player_.onStateChanged(nullptr);
    player_.onEvent(nullptr);

    // FBO holds OpenGL resources; free them with the context current.
    if (fbo_) {
        if (glContext_ && surface_ && glContext_->makeCurrent(surface_)) {
            delete fbo_;
            glContext_->doneCurrent();
        } else {
            delete fbo_;
        }
        fbo_ = nullptr;
    }
    // glContext_ and surface_ are QObject children, deleted automatically.
}

QMediaPlayer::PlaybackState MDKPlayerControl::state() const
{
    return toQt(player_.state());
}

QMediaPlayer::MediaStatus MDKPlayerControl::mediaStatus() const
{
    return toQt(player_.mediaStatus());
}

qint64 MDKPlayerControl::duration() const
{
    return duration_;
}

qint64 MDKPlayerControl::position() const
{
    return player_.position();
}

void MDKPlayerControl::setPosition(qint64 position)
{
    player_.seek(position);
}

float MDKPlayerControl::bufferProgress() const
{
    return 0.0f;
}

bool MDKPlayerControl::isVideoAvailable() const
{
    return has_v_;
}

bool MDKPlayerControl::isSeekable() const
{
    return true;
}

QMediaTimeRange MDKPlayerControl::availablePlaybackRanges() const
{
    return QMediaTimeRange();
}

qreal MDKPlayerControl::playbackRate() const
{
    return qreal(player_.playbackRate());
}

void MDKPlayerControl::setPlaybackRate(qreal rate)
{
    const auto old = playbackRate();
    if (old == rate)
        return;
    player_.setPlaybackRate(float(rate));
    if (playbackRate() != old)
        playbackRateChanged(playbackRate());
}

QUrl MDKPlayerControl::media() const
{
    return url_;
}

const QIODevice *MDKPlayerControl::mediaStream() const
{
    return nullptr;
}

void MDKPlayerControl::setMedia(const QUrl &url, QIODevice *stream)
{
    stop();
    url_ = url;

    if (stream) {
        // Encode the QIODevice pointer as a URI that iodevice.cpp's QMediaIO handler understands.
        // The caller must ensure the stream outlives the playback session.
        player_.setMedia(QString("qio:%1").arg(qintptr(stream)).toUtf8().constData());
    } else if (url.isLocalFile()) {
        player_.setMedia(url.toLocalFile().toUtf8().constData());
    } else {
        player_.setMedia(url.toString().toUtf8().constData());
    }

    positionChanged(0);
    player_.waitFor(State::Stopped);
    player_.prepare(0, [this](int64_t position, bool *) {
        if (position < 0) {
            QMetaObject::invokeMethod(this, [this] {
                error(QMediaPlayer::ResourceError, tr("Failed to load source."));
            }, Qt::QueuedConnection);
        }
        const auto &info = player_.mediaInfo();
        duration_ = info.duration;
        has_a_ = !info.audio.empty();
        has_v_ = !info.video.empty();
        if (has_v_) {
            auto &c = info.video[0].codec;
            video_w_ = c.width;
            video_h_ = c.height;
        }
        QMetaObject::invokeMethod(this, [this] {
            durationChanged(duration_);
            audioAvailableChanged(has_a_);
            videoAvailableChanged(has_v_);
            seekableChanged(true);
        }, Qt::QueuedConnection);
#if defined(MDK_VERSION_CHECK)
# if MDK_VERSION_CHECK(0, 5, 0)
        return true;
# endif
#endif
        // MDK < 0.5.0: callback returns void, no explicit return needed.
    });
}

void MDKPlayerControl::play()
{
    player_.setState(State::Playing);
}

void MDKPlayerControl::pause()
{
    player_.setState(State::Paused);
}

void MDKPlayerControl::stop()
{
    player_.setState(State::Stopped);
}

void MDKPlayerControl::setAudioOutput(QPlatformAudioOutput * /*output*/)
{
    // MDK manages its own audio output internally.
}

void MDKPlayerControl::setVideoSink(QVideoSink *sink)
{
    sink_ = sink;
    if (!sink) {
        player_.setRenderCallback(nullptr);
        return;
    }

    // Tell MDK a renderer is active (size updated when media loads via onMediaStatusChanged).
    player_.setVideoSurfaceSize(1, 1);

    player_.setRenderCallback([this](void *) {
        QMetaObject::invokeMethod(this, &MDKPlayerControl::onFrameAvailable,
                                  Qt::QueuedConnection);
    });
}

void MDKPlayerControl::ensureGLContext()
{
    if (glContext_)
        return;
    surface_ = new QOffscreenSurface(nullptr, this);
    surface_->create();
    glContext_ = new QOpenGLContext(this);
    // Share with the application's existing context so hardware-decoded textures are accessible.
    glContext_->setShareContext(QOpenGLContext::currentContext());
    glContext_->create();
}

void MDKPlayerControl::onFrameAvailable()
{
    if (!sink_)
        return;

#ifdef MDK_ABI
    // Attempt to obtain a host (CPU) frame directly, avoiding any GL round-trip.
    player_.renderVideo();
    VideoFrame v;
    player_.getVideoFrame(&v);
    if (v && !v.nativeBuffer()) {
        const auto qfmt = toQt(v.format().format());
        if (qfmt != QVideoFrameFormat::Format_Invalid) {
            const QSize sz(v.width(), v.height());
            QVideoFrame frame(QVideoFrameFormat(sz, qfmt));
            if (frame.map(QVideoFrame::WriteOnly)) {
                const int planes = v.format().planeCount();
                for (int i = 0; i < planes && i < frame.planeCount(); ++i) {
                    auto buf = v.buffer(i);
                    const auto *src = static_cast<const uint8_t *>(buf->constData());
                    auto *dst = frame.bits(i);
                    const int srcStride = buf->stride();
                    const int dstStride = frame.bytesPerLine(i);
                    // Plane height: Y plane is full height; chroma planes are scaled.
                    const int planeH = frame.mappedBytes(i) / dstStride;
                    for (int y = 0; y < planeH; ++y)
                        memcpy(dst + y * dstStride, src + y * srcStride,
                               qMin(srcStride, dstStride));
                }
                frame.unmap();
                sink_->setVideoFrame(frame);
                return;
            }
        }
    }
#endif // MDK_ABI

    // FBO path: used for hardware-decoded (native buffer) frames and when MDK_ABI is unavailable.
    if (video_w_ <= 0 || video_h_ <= 0)
        return;

    ensureGLContext();
    if (!glContext_ || !glContext_->makeCurrent(surface_))
        return;

    if (!fbo_ || fbo_->size() != QSize(video_w_, video_h_)) {
        // scale() sets an absolute Y-flip so the decoded image is upright in the QImage readback.
        // It is idempotent, so re-setting on FBO resize is safe.
        player_.scale(1.0f, -1.0f);
        player_.setVideoSurfaceSize(video_w_, video_h_);
        delete fbo_;
        fbo_ = new QOpenGLFramebufferObject(video_w_, video_h_);
    }

    fbo_->bind();
    player_.renderVideo();
    fbo_->release();

    // Pass false: Y-flip is already handled by player_.scale() above.
    QImage img = fbo_->toImage(false);
    glContext_->doneCurrent();

    if (img.isNull())
        return;

    img = img.convertToFormat(QImage::Format_RGBA8888);
    QVideoFrame frame(QVideoFrameFormat(img.size(), QVideoFrameFormat::Format_RGBA8888));
    if (frame.map(QVideoFrame::WriteOnly)) {
        auto *dst = frame.bits(0);
        const auto *src = img.constBits();
        const int dstStride = frame.bytesPerLine(0);
        const int srcStride = img.bytesPerLine();
        for (int y = 0; y < img.height(); ++y)
            memcpy(dst + y * dstStride, src + y * srcStride, qMin(srcStride, dstStride));
        frame.unmap();
    }
    sink_->setVideoFrame(frame);
}
