
/*
 * Copyright (C) 2018 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#include "mediaplayerservice.h"
#include "mediaplayercontrol.h"
#include "renderercontrol.h"
#ifdef QT_MULTIMEDIAWIDGETS_LIB
#include "videowidgetcontrol.h"
#endif //QT_MULTIMEDIAWIDGETS_LIB

MediaPlayerService::MediaPlayerService(QObject* parent)
    : QMediaService(parent)
    , mpc_(new MediaPlayerControl(parent))
{
    qInfo("create service...");
}

QMediaControl* MediaPlayerService::requestControl(const char* name)
{
    if (qstrcmp(name, QMediaPlayerControl_iid) == 0)
        return mpc_;
    if (qstrcmp(name, QVideoRendererControl_iid) == 0)
        return new RendererControl(mpc_, this);
#ifdef QT_MULTIMEDIAWIDGETS_LIB
    if (qstrcmp(name, QVideoWidgetControl_iid) == 0)
        return new VideoWidgetControl(mpc_, this);
#endif //QT_MULTIMEDIAWIDGETS_LIB
    qWarning("MediaPlayerService: unsupported control: %s", qPrintable(name));
    return nullptr;
}

void MediaPlayerService::releaseControl(QMediaControl*)
{
    qInfo("release control...");
}
