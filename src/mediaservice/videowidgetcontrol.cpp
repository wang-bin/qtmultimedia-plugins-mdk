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

int VideoWidgetControl::hue() const
{
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
