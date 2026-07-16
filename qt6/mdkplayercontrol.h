/*
 * Copyright (C) 2026 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#pragma once

#include <QtMultimedia/private/qplatformmediaplayer_p.h>
#include <QtMultimedia/qmediametadata.h>
#include <QObject>
#include <QUrl>

#include "mdk/Player.h"
#include "mdk/global.h"

class QVideoSink;
class QOpenGLFramebufferObject;
class QOffscreenSurface;
class QOpenGLContext;
class QTimer;
class QPlatformAudioOutput;

class MDKPlayerControl final : public QObject, public QPlatformMediaPlayer
{
    Q_OBJECT
public:
    explicit MDKPlayerControl(QMediaPlayer *player = nullptr);
    ~MDKPlayerControl() override;

    qint64 duration() const override;
    qint64 position() const override;
    void setPosition(qint64 position) override;

    float bufferProgress() const override;
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
    bool canPlayQrc() const override { return false; }

    void setAudioOutput(QPlatformAudioOutput *output) override;
    void setVideoSink(QVideoSink *sink) override;

    QMediaMetaData metaData() const override { return metaData_; }

    void setLoops(int loops) override;

    int trackCount(TrackType type) override;
    QMediaMetaData trackMetaData(TrackType type, int streamNumber) override;
    int activeTrack(TrackType type) override;
    void setActiveTrack(TrackType type, int streamNumber) override;

    MDK_NS::Player *player() { return &player_; }

private Q_SLOTS:
    void onFrameAvailable();
    void applyAudioOutput();
    void pumpStreamBuffer();

private:
    void ensureGLContext();
    void updateMetaData();
    void resetTrackCache();
    void stopStreamPump();
    static MDK_NS::MediaType toMdk(TrackType type);

    MDK_NS::Player player_;
    QVideoSink *sink_ = nullptr;
    QPlatformAudioOutput *audioOutput_ = nullptr;
    QUrl url_;
    QIODevice *stream_ = nullptr;
    qint64 duration_ = 0;
    int video_w_ = 0;
    int video_h_ = 0;
    MDK_NS::MediaStatus status_{};
    QMediaMetaData metaData_;
    int activeTracks_[NTrackTypes] = { 0, 0, -1 };

    QOffscreenSurface *surface_ = nullptr;
    QOpenGLContext *glContext_ = nullptr;
    QOpenGLFramebufferObject *fbo_ = nullptr;
    QTimer *positionTimer_ = nullptr;
    QTimer *streamPump_ = nullptr;
};
