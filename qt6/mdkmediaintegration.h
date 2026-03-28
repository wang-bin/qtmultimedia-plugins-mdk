/*
 * Copyright (C) 2022 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#pragma once
#include <private/qplatformmediaintegration_p.h>

class MDKMediaIntegration final : public QPlatformMediaIntegration {
public:
    MDKMediaIntegration();
    ~MDKMediaIntegration() override;

    QPlatformMediaPlayer *createPlayer(QMediaPlayer *player) override;
    QPlatformVideoSink *createVideoSink(QVideoSink *sink) override;
};
