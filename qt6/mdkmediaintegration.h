/*
 * Copyright (C) 2022 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#pragma once
#include <private/qplatformmediaintegration_p.h>

#if QT_VERSION < QT_VERSION_CHECK(6, 10, 0)
using MDKMediaPlayerResult = QMaybe<QPlatformMediaPlayer *>;
using MDKVideoSinkResult = QMaybe<QPlatformVideoSink *>;
#else
using MDKMediaPlayerResult = q23::expected<QPlatformMediaPlayer *, QString>;
using MDKVideoSinkResult = q23::expected<QPlatformVideoSink *, QString>;
#endif

class MDKMediaIntegration final : public QPlatformMediaIntegration {
public:
    MDKMediaIntegration();
    ~MDKMediaIntegration() override;

    MDKMediaPlayerResult createPlayer(QMediaPlayer *player) override;
    MDKVideoSinkResult createVideoSink(QVideoSink *sink) override;
};
