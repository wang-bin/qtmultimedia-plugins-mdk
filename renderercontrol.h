
/*
 * Copyright (C) 2018 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#pragma once
#include <QVideoRendererControl>

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
};