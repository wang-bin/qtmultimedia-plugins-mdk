/*
 * Copyright (C) 2026 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#include "mdkmediaintegration.h"
#include "mdkplayercontrol.h"
#include "mdkvideosink.h"

MDKMediaIntegration::MDKMediaIntegration()
    : QPlatformMediaIntegration(QLatin1String("mdk"))
{
}

MDKMediaIntegration::~MDKMediaIntegration() = default;

MDK_MEDIA_RESULT(QPlatformMediaPlayer) MDKMediaIntegration::createPlayer(QMediaPlayer *player)
{
    return MDK_MEDIA_OK(new MDKPlayerControl(player));
}

MDK_MEDIA_RESULT(QPlatformVideoSink) MDKMediaIntegration::createVideoSink(QVideoSink *sink)
{
    return MDK_MEDIA_OK(new MDKVideoSink(sink));
}
