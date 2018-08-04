/*
 * Copyright (C) 2018 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#include "videowidgetcontrol.h"
#include "mediaplayercontrol.h"
#include <QOpenGLWidget>
#include <QCoreApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

class VideoWidget : public QOpenGLWidget, public QOpenGLFunctions
{
public:
    VideoWidget(QWidget *parent = nullptr) : QOpenGLWidget(parent) {}

    void setSource(Player* player) {
        player_ = player;
        player_->setRenderCallback([this]{ update(); });
    }

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();
        auto ctx = context();
        connect(context(), &QOpenGLContext::aboutToBeDestroyed, [ctx]{
            QOffscreenSurface s;
            s.create();
            ctx->makeCurrent(&s);
            Player::foreignGLContextDestroyed();
            ctx->doneCurrent();
        });
    }

    void resizeGL(int w, int h) override {
        if (!player_)
            return;
        player_->setVideoSurfaceSize(w, h);
    }

    void paintGL() override {
        if (!player_)
            return;
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        player_->renderVideo();
    }
private:
    Player* player_ = nullptr;
};

VideoWidgetControl::VideoWidgetControl(QObject* parent)
    : QVideoWidgetControl(parent)
    , vw_(new VideoWidget())
{
}

VideoWidgetControl::~VideoWidgetControl()
{
    delete vw_;
}

bool VideoWidgetControl::isFullScreen() const
{
    return fs_;
}

void VideoWidgetControl::setFullScreen(bool fullScreen)
{
    fs_ = fullScreen;
}

Qt::AspectRatioMode VideoWidgetControl::aspectRatioMode() const
{
    return Qt::IgnoreAspectRatio;
}

void VideoWidgetControl::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    am_ = mode;
    // mpc_ is set internally & never null
    float a;
    switch (mode) {
    case Qt::IgnoreAspectRatio:
        a = IgnoreAspectRatio;
        break;
    case Qt::KeepAspectRatioByExpanding:
        a = KeepAspectRatioCrop;
        break;
    default:
        a = KeepAspectRatio;
        break;
    }
    mpc_->player()->setAspectRatio(a);
}

int VideoWidgetControl::brightness() const
{
    return 0;
}

void VideoWidgetControl::setBrightness(int brightness)
{
}

int VideoWidgetControl::contrast() const
{
    return 0;
}

void VideoWidgetControl::setContrast(int contrast)
{
}

int VideoWidgetControl::hue() const {
    return 0;
}

void VideoWidgetControl::setHue(int hue)
{
}

int VideoWidgetControl::saturation() const
{
    return 0;
}

void VideoWidgetControl::setSaturation(int saturation)
{
}

QWidget* VideoWidgetControl::videoWidget()
{
    return vw_;
}

void VideoWidgetControl::setSource(MediaPlayerControl* player)
{
    mpc_ = player;
    vw_->setSource(mpc_->player());
}
