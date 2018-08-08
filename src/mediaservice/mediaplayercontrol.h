/*
 * Copyright (C) 2018 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#pragma once
#include <QMediaPlayerControl>
#include "mdk/Player.h"
using namespace MDK_NS;

class MediaPlayerControl : public QMediaPlayerControl {
    Q_OBJECT
public:
    explicit MediaPlayerControl(QObject* parent = nullptr);

    Player* player() { return &player_; }

    QMediaPlayer::State state() const override;
    QMediaPlayer::MediaStatus mediaStatus() const override;
    qint64 duration() const override;
    qint64 position() const override;
    void setPosition(qint64 position) override;
    int volume() const override;
    void setVolume(int volume) override;
    bool isMuted() const override;
    void setMuted(bool muted) override;
    int bufferStatus() const override;
    bool isAudioAvailable() const override;
    bool isVideoAvailable() const override;
    bool isSeekable() const override;
    QMediaTimeRange availablePlaybackRanges() const override;
    qreal playbackRate() const override;
    void setPlaybackRate(qreal rate) override;
    QMediaContent media() const override;
    const QIODevice *mediaStream() const override;
    void setMedia(const QMediaContent &media, QIODevice *stream) override;
    void play() override;
    void pause() override;
    void stop() override;
Q_SIGNALS:
    void frameAvailable();
private:
    bool has_a_ = true;
    bool has_v_ = true;
    bool mute_ = false;
    int vol_ = 100;
    int64_t duration_ = 0;
    Player player_;
};
