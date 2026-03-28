/*
 * Copyright (C) 2022 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#pragma once
#include <private/qplatformmediaplayer_p.h>
#include "mdk/Player.h"
#include "mdk/global.h"
#include <QObject>
#include <QUrl>

class QVideoSink;
class QOpenGLFramebufferObject;
class QOffscreenSurface;
class QOpenGLContext;
class QTimer;

class MDKPlayerControl final : public QObject, public QPlatformMediaPlayer {
    Q_OBJECT
public:
    explicit MDKPlayerControl(QMediaPlayer *player = nullptr);
    ~MDKPlayerControl() override;

    QMediaPlayer::PlaybackState state() const override;
    QMediaPlayer::MediaStatus mediaStatus() const override;

    qint64 duration() const override;
    qint64 position() const override;
    void setPosition(qint64 position) override;

    float bufferProgress() const override;
    bool isVideoAvailable() const override;
    bool isSeekable() const override;
    QMediaTimeRange availablePlaybackRanges() const override;

    qreal playbackRate() const override;
    void setPlaybackRate(qreal rate) override;

    QUrl media() const override;
    const QIODevice *mediaStream() const override;
    void setMedia(const QUrl &url, QIODevice *stream) override;

    void play() override;
    void pause() override;
    void stop() override;

    bool streamPlaybackSupported() const override { return true; }
    void setAudioOutput(QPlatformAudioOutput *output) override;
    void setVideoSink(QVideoSink *sink) override;

    MDK_NS::Player *player() { return &player_; }

private Q_SLOTS:
    void onFrameAvailable();

private:
    void ensureGLContext();

    MDK_NS::Player player_;
    QVideoSink *sink_ = nullptr;
    QUrl url_;
    int64_t duration_ = 0;
    bool has_v_ = false;
    bool has_a_ = false;
    int video_w_ = 0;
    int video_h_ = 0;
    MDK_NS::MediaStatus status_{};

    QOffscreenSurface *surface_ = nullptr;
    QOpenGLContext *glContext_ = nullptr;
    QOpenGLFramebufferObject *fbo_ = nullptr;
    QTimer *positionTimer_ = nullptr;
};
