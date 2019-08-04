/*
 * Copyright (C) 2018 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#include "plugin.h"
#include "mediaplayerservice.h"

QMediaService* MDKPlugin::create(QString const& key)
{
    if (key == QLatin1String(Q_MEDIASERVICE_MEDIAPLAYER))
        return new MediaPlayerService();
    qWarning("MDKPlugin: unsupported key: %s", qPrintable(key));
    return nullptr;
}

void MDKPlugin::release(QMediaService *service)
{
    delete service;
}

QMediaServiceProviderHint::Features MDKPlugin::supportedFeatures(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_MEDIAPLAYER)
        return QMediaServiceProviderHint::VideoSurface; //QMediaServiceProviderHint::StreamPlayback
    return QMediaServiceProviderHint::Features();
}
