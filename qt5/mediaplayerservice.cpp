
/*
 * Copyright (C) 2018-2019 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#include "iodevice.h"
#include "mediaplayerservice.h"
#include "mediaplayercontrol.h"
#include "metadatareadercontrol.h"
#include "renderercontrol.h"
#ifdef QT_MULTIMEDIAWIDGETS_LIB
#include "videowidgetcontrol.h"
#endif //QT_MULTIMEDIAWIDGETS_LIB

MediaPlayerService::MediaPlayerService(QObject* parent)
    : QMediaService(parent)
    , mpc_(new MediaPlayerControl(parent))
{
#ifdef MDK_ABI
    QMediaIO::registerOnce();
#endif // MDK_ABI
    qInfo("create service...");
}

QMediaControl* MediaPlayerService::requestControl(const char* name)
{
    qInfo("requestControl %s", name);
    if (qstrcmp(name, QMetaDataReaderControl_iid) == 0) // requested when constructing QMediaObject(QMediaPlayer)
        return new MetaDataReaderControl(mpc_);
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

void MediaPlayerService::releaseControl(QMediaControl* control)
{
    qInfo() << "release control " << control;
    delete control;
    if (control == mpc_)
        mpc_ = nullptr;
}
