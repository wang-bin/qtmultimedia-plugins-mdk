/*
 * Copyright (C) 2018-2020 Wang Bin - wbsecg1 at gmail.com
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

class VideoWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
    VideoWidget(QWidget *parent = nullptr) : QOpenGLWidget(parent) {}

    void setSource(Player* player) {
        player_ = player;
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

VideoWidgetControl::VideoWidgetControl(MediaPlayerControl* player, QObject* parent)
    : QVideoWidgetControl(parent)
    , vw_(new VideoWidget())
    , mpc_(player)
{
    vw_->setSource(mpc_->player());
    //connect(mpc_, &MediaPlayerControl::frameAvailable, vw_, &VideoWidget::update, Qt::DirectConnection);
    connect(mpc_, SIGNAL(frameAvailable()), vw_, SLOT(update()), Qt::DirectConnection);
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
    return am_;
}

static float fromQt(Qt::AspectRatioMode value)
{
    switch (value) {
    case Qt::IgnoreAspectRatio: return IgnoreAspectRatio;
    case Qt::KeepAspectRatioByExpanding: return KeepAspectRatioCrop;
    case Qt::KeepAspectRatio: return KeepAspectRatio;
    default: return KeepAspectRatio;
    }
}

void VideoWidgetControl::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    am_ = mode;
    // mpc_ is set internally & never null
    mpc_->player()->setAspectRatio(fromQt(mode));
}

void VideoWidgetControl::setBrightness(int brightness)
{
    brightness_ = brightness;
    float v = float(brightness)/100.0f;
    mpc_->player()->set(VideoEffect::Brightness, v);
}

void VideoWidgetControl::setContrast(int contrast)
{
    contrast_ = contrast;
    float v = float(contrast)/100.0f;
    mpc_->player()->set(VideoEffect::Contrast, v);
}

void VideoWidgetControl::setHue(int hue)
{
    hue_ = hue;
    float v = float(hue)/100.0f;
    mpc_->player()->set(VideoEffect::Hue, v);
}

void VideoWidgetControl::setSaturation(int saturation)
{
    saturation_ = saturation;
    float v = float(saturation)/100.0f;
    mpc_->player()->set(VideoEffect::Saturation, v);
}

QWidget* VideoWidgetControl::videoWidget()
{
    return vw_;
}
