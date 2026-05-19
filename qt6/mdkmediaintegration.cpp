/*
 * Copyright (C) 2022 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#include "mdkmediaintegration.h"
#include "mdkplayercontrol.h"
#include <private/qplatformvideosink_p.h>
#ifdef MDK_ABI
# include "../iodevice.h"
#endif

MDKMediaIntegration::MDKMediaIntegration() = default;
MDKMediaIntegration::~MDKMediaIntegration() = default;

MDKMediaPlayerResult MDKMediaIntegration::createPlayer(QMediaPlayer *player)
{
#ifdef MDK_ABI
    QMediaIO::registerOnce();
#endif
    return new MDKPlayerControl(player);
}

MDKVideoSinkResult MDKMediaIntegration::createVideoSink(QVideoSink *sink)
{
    return new QPlatformVideoSink(sink);
}
