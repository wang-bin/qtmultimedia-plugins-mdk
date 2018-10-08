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
    return QMediaPlayer::StoppedState;
}

// TODO: mask value&xxx
static QMediaPlayer::MediaStatus toQt(MediaStatus value) {
    switch (value) {
        case MediaStatus::UnknownMediaStatus: return QMediaPlayer::UnknownMediaStatus;
        case MediaStatus::NoMedia: return QMediaPlayer::NoMedia;
        case MediaStatus::InvalidMedia: return QMediaPlayer::InvalidMedia;
        default: break;
    }
    if (test_flag(value & MediaStatus::LoadingMedia))
        return QMediaPlayer::LoadingMedia;
    if (test_flag(value & MediaStatus::StalledMedia))
        return QMediaPlayer::StalledMedia;
    if (test_flag(value & MediaStatus::BufferingMedia))
        return QMediaPlayer::BufferingMedia;
    if (test_flag(value & MediaStatus::BufferedMedia))
        return QMediaPlayer::BufferedMedia; // playing or paused
    if (test_flag(value & MediaStatus::EndOfMedia))
        return QMediaPlayer::EndOfMedia; // playback complete
    if (test_flag(value & MediaStatus::LoadedMedia))
        return QMediaPlayer::LoadedMedia; // connected, stopped. so last
    return QMediaPlayer::UnknownMediaStatus;
}

MediaPlayerControl::MediaPlayerControl(QObject* parent) : QMediaPlayerControl(parent)
{
    //player_.setVideoDecoders({"VideoToolbox", "FFmpeg"}); // no display for 2nd video
    player_.onStateChanged([this](State value){
        Q_EMIT stateChanged(toQt(value));
    });
    player_.onMediaStatusChanged([this](MediaStatus value){
        Q_EMIT mediaStatusChanged(toQt(value));
        return true;
    });
    player_.setRenderCallback([this](void*){
        Q_EMIT frameAvailable();
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
    return vol_;
}

void MediaPlayerControl::setVolume(int volume)
{
    vol_ = volume;
    player_.setVolume(float(volume)/100.0f);
}

bool MediaPlayerControl::isMuted() const
{
    return mute_;
}

void MediaPlayerControl::setMuted(bool muted)
{
    mute_ = muted;
    player_.setMute(muted);
}

int MediaPlayerControl::bufferStatus() const
{
    return 0;
}

bool MediaPlayerControl::isAudioAvailable() const
{
    return has_a_;
}

bool MediaPlayerControl::isVideoAvailable() const
{
    return has_v_;
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
    return qreal(player_.playbackRate());
}

void MediaPlayerControl::setPlaybackRate(qreal rate)
{
    const auto old = playbackRate();
    if (playbackRate() == rate)
        return;
    player_.setPlaybackRate(float(rate));
    if (playbackRate() != old) // success
        Q_EMIT playbackRateChanged(playbackRate());
}

QMediaContent MediaPlayerControl::media() const
{
    return QMediaContent();
}

const QIODevice* MediaPlayerControl::mediaStream() const
{
    return nullptr;
}

void MediaPlayerControl::setMedia(const QMediaContent& media, QIODevice* io)
{
    stop();
    if (io) {
        player_.setMedia(QString("qio:%1").arg(qintptr(io)).toUtf8().constData());
    } else {
        if (media.canonicalUrl().isLocalFile())
            player_.setMedia(media.canonicalUrl().toLocalFile().toUtf8().constData()); // for windows
        else
            player_.setMedia(media.canonicalUrl().url().toUtf8().constData());
    }

    Q_EMIT positionChanged(0);
    player_.waitFor(State::Stopped);
    player_.prepare(0, [this](int64_t position, bool*){
        Q_UNUSED(position)
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
