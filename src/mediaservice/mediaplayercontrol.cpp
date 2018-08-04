/*
 * Copyright (C) 2018 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#include "mediaplayercontrol.h"
#include "mdk/MediaInfo.h"

static QMediaPlayer::State toQt(State value) {
    switch (value) {
        case State::Playing: return QMediaPlayer::PlayingState;
        case State::Paused: return QMediaPlayer::PausedState;
        case State::Stopped: return QMediaPlayer::StoppedState;
    }
}

static QMediaPlayer::MediaStatus toQt(MediaStatus value) {
    switch (value) {
        case MediaStatus::UnknownMediaStatus: return QMediaPlayer::UnknownMediaStatus;
        case MediaStatus::NoMedia: return QMediaPlayer::NoMedia;
        case MediaStatus::LoadingMedia: return QMediaPlayer::LoadingMedia;
        case MediaStatus::LoadedMedia: return QMediaPlayer::LoadedMedia;
        case MediaStatus::StalledMedia: return QMediaPlayer::StalledMedia;
        case MediaStatus::BufferingMedia: return QMediaPlayer::BufferingMedia;
        case MediaStatus::BufferedMedia: return QMediaPlayer::BufferedMedia;
        case MediaStatus::EndOfMedia: return QMediaPlayer::EndOfMedia;
        case MediaStatus::InvalidMedia: return QMediaPlayer::InvalidMedia;
        default: return QMediaPlayer::UnknownMediaStatus;
    }
}

MediaPlayerControl::MediaPlayerControl(QObject* parent) : QMediaPlayerControl(parent)
{
    //player_.setVideoDecoders({"VideoToolbox", "FFmpeg"});
    player_.onStateChanged([this](State value){
        Q_EMIT stateChanged(toQt(value));
    });
    player_.onMediaStatusChanged([this](MediaStatus value){
        Q_EMIT mediaStatusChanged(toQt(value));
        return true;
    });
}


QMediaPlayer::State MediaPlayerControl::state() const
{
    return toQt(player_.state());
}

QMediaPlayer::MediaStatus MediaPlayerControl::mediaStatus() const
{
    return toQt(player_.mediaStatus());
}

qint64 MediaPlayerControl::duration() const
{
    return duration_;
}

qint64 MediaPlayerControl::position() const
{
    return player_.position();
}

void MediaPlayerControl::setPosition(qint64 position)
{
    player_.seek(position);
}

int MediaPlayerControl::volume() const
{
    return 0;
}

void MediaPlayerControl::setVolume(int volume)
{
}

bool MediaPlayerControl::isMuted() const
{
    return false;
}

void MediaPlayerControl::setMuted(bool muted)
{
}

int MediaPlayerControl::bufferStatus() const
{
    return 0;
}

bool MediaPlayerControl::isAudioAvailable() const
{
    return true;
}

bool MediaPlayerControl::isVideoAvailable() const
{
    return true;
}

bool MediaPlayerControl::isSeekable() const
{
    return true;
}

QMediaTimeRange MediaPlayerControl::availablePlaybackRanges() const
{
    return QMediaTimeRange();
}

qreal MediaPlayerControl::playbackRate() const
{
    return 1.0;
}

void MediaPlayerControl::setPlaybackRate(qreal rate)
{
}

QMediaContent MediaPlayerControl::media() const
{
    return QMediaContent();
}

const QIODevice* MediaPlayerControl::mediaStream() const
{
    return nullptr;
}

void MediaPlayerControl::setMedia(const QMediaContent& media, QIODevice*)
{
    // TODO: iodevice object to url "qiodevice:"
    stop();
    player_.setMedia(media.canonicalUrl().url().toUtf8().constData());
    Q_EMIT positionChanged(0);
    player_.waitFor(State::Stopped);
    player_.prepare(0, [this](int64_t position, bool*){
        const auto& info = player_.mediaInfo();
        duration_ = info.duration;
        has_a_ = !info.audio.empty();
        has_v_ = !info.video.empty();
        Q_EMIT durationChanged(duration_);
        Q_EMIT audioAvailableChanged(has_a_);
        Q_EMIT videoAvailableChanged(has_v_);
        Q_EMIT seekableChanged(true);
    });
}

void MediaPlayerControl::play()
{
    player_.setState(State::Playing);
}

void MediaPlayerControl::pause()
{
    player_.setState(State::Paused);
}

void MediaPlayerControl::stop()
{
    player_.setState(State::Stopped);
}
