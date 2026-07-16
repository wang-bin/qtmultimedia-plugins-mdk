/*
 * Copyright (C) 2026 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#pragma once

#include "qtcompat.h"
#include <QtMultimedia/private/qplatformmediaintegration_p.h>

class MDKMediaIntegration final : public QPlatformMediaIntegration
{
public:
    MDKMediaIntegration();
    ~MDKMediaIntegration() override;

    MDK_MEDIA_RESULT(QPlatformMediaPlayer) createPlayer(QMediaPlayer *player) override;
    MDK_MEDIA_RESULT(QPlatformVideoSink) createVideoSink(QVideoSink *sink) override;
};
