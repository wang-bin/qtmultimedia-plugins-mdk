/*
 * Copyright (C) 2026 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#include "mdkplatformmediaplugin.h"
#include "mdkmediaintegration.h"

MDKMediaPlugin::MDKMediaPlugin(QObject *parent)
    : QPlatformMediaPlugin(parent)
{
}

QPlatformMediaIntegration *MDKMediaPlugin::create(const QString &key)
{
    if (key == QLatin1String("mdk"))
        return new MDKMediaIntegration;
    return nullptr;
}
