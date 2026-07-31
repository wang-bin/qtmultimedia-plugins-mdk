/*
 * Copyright (C) 2026 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#include "mdkplayercontrol.h"
#include "mdkrihiframe.h"

#include "mdk/MediaInfo.h"
#include "mdk/global.h"

#include <QtMultimedia/private/qplatformaudiooutput_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>
#include <QtMultimedia/qaudiooutput.h>
#include <QtMultimedia/qmediaplayer.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/qvideosink.h>

#include <QDateTime>
#include <QIODevice>
#include <QMetaObject>
#include <QStringList>
#include <QTimer>
#include <rhi/qrhi.h>

using namespace MDK_NS;

static QMediaPlayer::PlaybackState toQt(State value)
{
    switch (value) {
    case State::Playing:
        return QMediaPlayer::PlayingState;
    case State::Paused:
        return QMediaPlayer::PausedState;
    case State::Stopped:
        return QMediaPlayer::StoppedState;
    }
    return QMediaPlayer::StoppedState;
}

static QMediaPlayer::MediaStatus toQt(MediaStatus value)
{
    switch (value) {
    case MediaStatus::NoMedia:
        return QMediaPlayer::NoMedia;
    case MediaStatus::Invalid:
        return QMediaPlayer::InvalidMedia;
    default:
        break;
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

static void insertTag(QMediaMetaData &md, const std::string &key, const std::string &value)
{
    const QString v = QString::fromStdString(value);
    if (key == "title")
        md.insert(QMediaMetaData::Title, v);
    else if (key == "comment")
        md.insert(QMediaMetaData::Comment, v);
    else if (key == "description")
        md.insert(QMediaMetaData::Description, v);
    else if (key == "publisher")
        md.insert(QMediaMetaData::Publisher, v);
    else if (key == "copyright")
        md.insert(QMediaMetaData::Copyright, v);
    else if (key == "album" || key == "album_title")
        md.insert(QMediaMetaData::AlbumTitle, v);
    else if (key == "album_artist")
        md.insert(QMediaMetaData::AlbumArtist, v);
    else if (key == "author" || key == "artist")
        md.insert(QMediaMetaData::ContributingArtist, QStringList{ v });
    else if (key == "genre")
        md.insert(QMediaMetaData::Genre, QStringList{ v });
    else if (key == "composer")
        md.insert(QMediaMetaData::Composer, QStringList{ v });
    else if (key == "performer")
        md.insert(QMediaMetaData::LeadPerformer, QStringList{ v });
    else if (key == "track")
        md.insert(QMediaMetaData::TrackNumber, v.toInt());
    else if (key == "date" || key == "year")
        md.insert(QMediaMetaData::Date, QDateTime::fromString(v, Qt::ISODate));
}

MDKPlayerControl::MDKPlayerControl(QMediaPlayer *player)
    : QObject(player)
    , QPlatformMediaPlayer(player)
{
    const char *vdecs = nullptr;
    if (GetGlobalOption("video.decoders.hint", &vdecs))
        player_.setProperty("video.decoders", vdecs);

    // Enable demuxer cache so bufferedTimeRanges() can report useful data for network media.
    player_.setProperty("demux.buffer.ranges", "8");

    player_.onStateChanged([this](State value) {
        QMetaObject::invokeMethod(
            this,
            [this, value] {
                stateChanged(toQt(value));
                if (toQt(value) == QMediaPlayer::PlayingState)
                    positionTimer_->start();
                else
                    positionTimer_->stop();
            },
            Qt::QueuedConnection);
    });

    player_.onMediaStatus([this](MediaStatus, MediaStatus value) {
        if (flags_added(status_, value, MediaStatus::Loaded)) {
            if (!player_.mediaInfo().video.empty()) {
                const auto &c = player_.mediaInfo().video[0].codec;
                video_w_ = c.width;
                video_h_ = c.height;
            }
            updateMetaData();
            tracksChanged();
            activeTracksChanged();
        }
        if (test_flag(value & MediaStatus::Invalid)) {
            QMetaObject::invokeMethod(
                this,
                [this] {
                    error(QMediaPlayer::ResourceError, tr("Invalid media."));
                },
                Qt::QueuedConnection);
        }
        status_ = value;
        QMetaObject::invokeMethod(
            this,
            [this, value] {
                mediaStatusChanged(toQt(value));
            },
            Qt::QueuedConnection);
        return false;
    });

    // player_.onError(...) — not in current mdk-sdk; use onEvent for decoder failures.
    player_.onEvent([this](const MediaEvent &e) {
        if (e.error < 0 && (e.category == "decoder.audio" || e.category == "decoder.video")) {
            QMetaObject::invokeMethod(
                this,
                [this] {
                    error(QMediaPlayer::FormatError,
                          tr("Unsupported media, a codec is missing."));
                },
                Qt::QueuedConnection);
        }
        return false;
    });

    positionTimer_ = new QTimer(this);
    positionTimer_->setInterval(100);
    connect(positionTimer_, &QTimer::timeout, this, [this] {
        positionChanged(player_.position());
        bufferProgressChanged(bufferProgress());
    });

    streamPump_ = new QTimer(this);
    streamPump_->setInterval(10);
    connect(streamPump_, &QTimer::timeout, this, &MDKPlayerControl::pumpStreamBuffer);
}

MDKPlayerControl::~MDKPlayerControl()
{
    stopStreamPump();
    player_.setRenderCallback(nullptr);
    player_.onMediaStatus(nullptr);
    player_.onStateChanged(nullptr);
    player_.onEvent(nullptr);
    // player_.onError(nullptr); — not in current mdk-sdk

    if (sink_)
        sink_->setVideoFrame({});

    if (rhiCtx_) {
        std::lock_guard<std::mutex> lock(rhiCtx_->mutex);
        rhiCtx_->player = nullptr;
        rhiCtx_->reset();
        rhiCtx_.reset();
    }

    if (audioOutput_ && audioOutput_->q)
        audioOutput_->q->disconnect(this);
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
    if (duration_ <= 0)
        return 0.f;
    const int64_t bufferedMs = player_.buffered();
    return qBound(0.f, float(bufferedMs) / float(duration_), 1.f);
}

QMediaTimeRange MDKPlayerControl::availablePlaybackRanges() const
{
    QMediaTimeRange ranges;
    for (const auto &r : player_.bufferedTimeRanges())
        ranges.addInterval(r.start, r.end);
    if (ranges.isEmpty() && duration_ > 0) {
        const int64_t bufferedMs = player_.buffered();
        if (bufferedMs > 0)
            ranges.addInterval(0, qMin<qint64>(bufferedMs, duration_));
    }
    return ranges;
}

qreal MDKPlayerControl::playbackRate() const
{
    return qreal(player_.playbackRate());
}

void MDKPlayerControl::setPlaybackRate(qreal rate)
{
    const auto old = playbackRate();
    if (qFuzzyCompare(old, rate))
        return;
    player_.setPlaybackRate(float(rate));
    if (!qFuzzyCompare(playbackRate(), old))
        playbackRateChanged(playbackRate());
}

QUrl MDKPlayerControl::media() const
{
    return url_;
}

const QIODevice *MDKPlayerControl::mediaStream() const
{
    return stream_;
}

void MDKPlayerControl::stopStreamPump()
{
    if (streamPump_)
        streamPump_->stop();
}

void MDKPlayerControl::pumpStreamBuffer()
{
    if (!stream_ || !stream_->isOpen()) {
        stopStreamPump();
        return;
    }
    QByteArray chunk = stream_->read(64 * 1024);
    if (chunk.isEmpty()) {
        if (stream_->atEnd())
            stopStreamPump();
        return;
    }
    player_.appendBuffer(reinterpret_cast<const uint8_t *>(chunk.constData()), size_t(chunk.size()));
}

void MDKPlayerControl::setMedia(const QUrl &url, QIODevice *stream)
{
    stop();
    stopStreamPump();
    url_ = url;
    stream_ = stream;
    resetTrackCache();
    metaData_ = {};
    metaDataChanged();

    if (stream) {
        // Public SDK has no MediaIO; feed QIODevice via stream: + appendBuffer.
        player_.setMedia("stream:");
        if (!stream->isOpen())
            stream->open(QIODevice::ReadOnly);
        streamPump_->start();
        pumpStreamBuffer();
    } else if (url.isLocalFile()) {
        player_.setMedia(url.toLocalFile().toUtf8().constData());
    } else if (url.scheme() == QLatin1String("qrc")) {
        // Map qrc to a QFile-backed stream feed when possible is left to the app;
        // FFmpeg cannot open qrc: directly. Use QFile + QIODevice path from the app.
        player_.setMedia(url.toString().toUtf8().constData());
    } else {
        player_.setMedia(url.toString().toUtf8().constData());
    }

    positionChanged(0);
    player_.waitFor(State::Stopped);
    player_.prepare(0, [this](int64_t position, bool *) {
        if (position < 0) {
            QMetaObject::invokeMethod(
                this,
                [this] {
                    error(QMediaPlayer::ResourceError, tr("Failed to load source."));
                },
                Qt::QueuedConnection);
        }
        const auto &info = player_.mediaInfo();
        duration_ = info.duration;
        video_w_ = 0;
        video_h_ = 0;
        const bool hasA = !info.audio.empty();
        const bool hasV = !info.video.empty();
        if (hasV) {
            const auto &c = info.video[0].codec;
            video_w_ = c.width;
            video_h_ = c.height;
        }
        activeTracks_[AudioStream] = hasA ? 0 : -1;
        activeTracks_[VideoStream] = hasV ? 0 : -1;
        activeTracks_[SubtitleStream] = info.subtitle.empty() ? -1 : 0;

        QMetaObject::invokeMethod(
            this,
            [this, hasA, hasV, position] {
                durationChanged(duration_);
                audioAvailableChanged(hasA);
                videoAvailableChanged(hasV);
                seekableChanged(position >= 0);
                updateMetaData();
                tracksChanged();
                activeTracksChanged();
            },
            Qt::QueuedConnection);
        return true;
    });
}

void MDKPlayerControl::play()
{
    resetCurrentLoop();
    player_.set(State::Playing);
}

void MDKPlayerControl::pause()
{
    player_.set(State::Paused);
}

void MDKPlayerControl::stop()
{
    stopStreamPump();
    player_.set(State::Stopped);
}

void MDKPlayerControl::setAudioOutput(QPlatformAudioOutput *output)
{
    if (audioOutput_ == output)
        return;

    if (audioOutput_ && audioOutput_->q)
        audioOutput_->q->disconnect(this);

    audioOutput_ = output;
    if (audioOutput_ && audioOutput_->q) {
        connect(audioOutput_->q, &QAudioOutput::deviceChanged, this, &MDKPlayerControl::applyAudioOutput);
        connect(audioOutput_->q, &QAudioOutput::volumeChanged, this, &MDKPlayerControl::applyAudioOutput);
        connect(audioOutput_->q, &QAudioOutput::mutedChanged, this, &MDKPlayerControl::applyAudioOutput);
    }
    applyAudioOutput();
}

void MDKPlayerControl::applyAudioOutput()
{
    if (!audioOutput_) {
        player_.setMute(false);
        player_.setVolume(1.f);
        player_.setProperty("audio.device", std::string());
        return;
    }
    player_.setMute(audioOutput_->muted);
    player_.setVolume(audioOutput_->volume);
    const QByteArray id = audioOutput_->device.id();
    const std::string deviceId(id.constData(), size_t(id.size()));
    // player_.setAudioDevice(deviceId); — not in current mdk-sdk
    player_.setProperty("audio.device", deviceId);
}

void MDKPlayerControl::setVideoSink(QVideoSink *sink)
{
    if (sink_ == sink)
        return;

    if (sink_)
        sink_->setVideoFrame({});

    sink_ = sink;
    if (!sink) {
        player_.setRenderCallback(nullptr);
        if (rhiCtx_) {
            std::lock_guard<std::mutex> lock(rhiCtx_->mutex);
            rhiCtx_->reset();
        }
        return;
    }

    if (!rhiCtx_) {
        rhiCtx_ = std::make_shared<MDKRhiContext>();
        rhiCtx_->player = &player_;
    }

    player_.setVideoSurfaceSize(1, 1);
    player_.setRenderCallback([this](void *) {
        QMetaObject::invokeMethod(this, &MDKPlayerControl::onFrameAvailable, Qt::QueuedConnection);
    });
}

void MDKPlayerControl::setLoops(int loops)
{
    QPlatformMediaPlayer::setLoops(loops);
    if (loops == QMediaPlayer::Infinite)
        player_.setLoop(-1);
    else if (loops <= 1)
        player_.setLoop(0);
    else
        player_.setLoop(loops - 1);
}

MediaType MDKPlayerControl::toMdk(TrackType type)
{
    switch (type) {
    case VideoStream:
        return MediaType::Video;
    case AudioStream:
        return MediaType::Audio;
    case SubtitleStream:
        return MediaType::Subtitle;
    default:
        return MediaType::Unknown;
    }
}

int MDKPlayerControl::trackCount(TrackType type)
{
    const auto &info = player_.mediaInfo();
    switch (type) {
    case VideoStream:
        return int(info.video.size());
    case AudioStream:
        return int(info.audio.size());
    case SubtitleStream:
        return int(info.subtitle.size());
    default:
        return 0;
    }
}

QMediaMetaData MDKPlayerControl::trackMetaData(TrackType type, int streamNumber)
{
    QMediaMetaData md;
    const auto &info = player_.mediaInfo();
    const auto fillFrom = [&](const auto &stream) {
        for (const auto &kv : stream.metadata)
            insertTag(md, kv.first, kv.second);
        const auto lang = stream.metadata.find("language");
        if (lang != stream.metadata.end())
            md.insert(QMediaMetaData::Title, QString::fromStdString(lang->second));
    };

    switch (type) {
    case VideoStream:
        if (streamNumber >= 0 && streamNumber < int(info.video.size())) {
            const auto &s = info.video[size_t(streamNumber)];
            fillFrom(s);
            md.insert(QMediaMetaData::Resolution, QSize(s.codec.width, s.codec.height));
            md.insert(QMediaMetaData::VideoBitRate, int(s.codec.bit_rate));
            md.insert(QMediaMetaData::VideoFrameRate, qreal(s.codec.frame_rate));
            if (s.codec.codec && *s.codec.codec)
                md.insert(QMediaMetaData::Description, QString::fromUtf8(s.codec.codec));
        }
        break;
    case AudioStream:
        if (streamNumber >= 0 && streamNumber < int(info.audio.size())) {
            const auto &s = info.audio[size_t(streamNumber)];
            fillFrom(s);
            md.insert(QMediaMetaData::AudioBitRate, int(s.codec.bit_rate));
            if (s.codec.codec && *s.codec.codec)
                md.insert(QMediaMetaData::Description, QString::fromUtf8(s.codec.codec));
        }
        break;
    case SubtitleStream:
        if (streamNumber >= 0 && streamNumber < int(info.subtitle.size()))
            fillFrom(info.subtitle[size_t(streamNumber)]);
        break;
    default:
        break;
    }
    return md;
}

int MDKPlayerControl::activeTrack(TrackType type)
{
    if (type >= NTrackTypes)
        return -1;
    // player_.activeTracks(...) — getter not in current mdk-sdk; use cached value from setActiveTrack.
    // const auto tracks = player_.activeTracks(toMdk(type));
    // if (!tracks.empty())
    //     return *tracks.begin();
    return activeTracks_[type];
}

void MDKPlayerControl::setActiveTrack(TrackType type, int streamNumber)
{
    if (type >= NTrackTypes)
        return;
    activeTracks_[type] = streamNumber;
    std::set<int> tracks;
    if (streamNumber >= 0)
        tracks.insert(streamNumber);
    player_.setActiveTracks(toMdk(type), tracks);
    activeTracksChanged();
}

void MDKPlayerControl::resetTrackCache()
{
    activeTracks_[VideoStream] = 0;
    activeTracks_[AudioStream] = 0;
    activeTracks_[SubtitleStream] = -1;
}

void MDKPlayerControl::updateMetaData()
{
    QMediaMetaData md;
    const auto &info = player_.mediaInfo();
    md.insert(QMediaMetaData::Duration, qint64(info.duration));
    if (info.format && *info.format)
        md.insert(QMediaMetaData::Description, QString::fromUtf8(info.format));

    for (const auto &kv : info.metadata)
        insertTag(md, kv.first, kv.second);

    if (!info.audio.empty()) {
        const auto &p = info.audio[0].codec;
        md.insert(QMediaMetaData::AudioBitRate, int(p.bit_rate));
    }
    if (!info.video.empty()) {
        const auto &p = info.video[0].codec;
        md.insert(QMediaMetaData::VideoFrameRate, qreal(p.frame_rate));
        md.insert(QMediaMetaData::VideoBitRate, int(p.bit_rate));
        md.insert(QMediaMetaData::Resolution, QSize(p.width, p.height));
    }

    metaData_ = md;
    metaDataChanged();
}

void MDKPlayerControl::onFrameAvailable()
{
    if (!sink_ || video_w_ <= 0 || video_h_ <= 0 || !rhiCtx_)
        return;

    QRhi *rhi = sink_->rhi();
    if (!rhi)
        return;

    switch (rhi->backend()) {
#if QT_CONFIG(opengl)
    case QRhi::OpenGLES2:
#endif
#if QT_CONFIG(metal)
    case QRhi::Metal:
#endif
#if defined(Q_OS_WIN)
    case QRhi::D3D11:
    case QRhi::D3D12:
#endif
#if QT_CONFIG(vulkan)
    case QRhi::Vulkan:
#endif
        break;
    default:
        return;
    }

    QVideoFrameFormat format(QSize(video_w_, video_h_), QVideoFrameFormat::Format_RGBA8888);
    auto buffer = std::make_unique<MDKRhiVideoBuffer>(rhiCtx_, format.frameSize());
    sink_->setVideoFrame(QVideoFramePrivate::createFrame(std::move(buffer), std::move(format)));
}
