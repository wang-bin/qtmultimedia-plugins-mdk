
/*
 * Copyright (C) 2018-2019 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#pragma once
#include "mdk/global.h"
#include <QVideoRendererControl>
using namespace MDK_NS;

class MediaPlayerControl;
class QOpenGLFramebufferObject;
class RendererControl : public QVideoRendererControl
{
    Q_OBJECT
public:
    RendererControl(MediaPlayerControl* player, QObject *parent = nullptr);
    QAbstractVideoSurface *surface() const override;
    void setSurface(QAbstractVideoSurface *surface) override;

    void setSource();
public Q_SLOTS:
    void onFrameAvailable();
private:
    QAbstractVideoSurface* surface_ = nullptr;
    MediaPlayerControl* mpc_ = nullptr;
    QOpenGLFramebufferObject *fbo_ = nullptr;

    // video_w/h_ is from MediaInfo, which may be incorrect. A better value is from VideoFrame but it's not a public class now.
    int video_w_ = 0;
    int video_h_ = 0;
    MediaStatus status_;
};