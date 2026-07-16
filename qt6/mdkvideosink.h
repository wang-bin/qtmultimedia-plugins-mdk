/*
 * Copyright (C) 2026 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#pragma once

#include <QtMultimedia/private/qplatformvideosink_p.h>

class MDKVideoSink final : public QPlatformVideoSink
{
public:
    explicit MDKVideoSink(QVideoSink *sink)
        : QPlatformVideoSink(sink)
    {
    }
};
