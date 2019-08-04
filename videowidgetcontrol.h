/*
 * Copyright (C) 2018 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#pragma once
#include <QVideoWidgetControl>

class MediaPlayerControl;
class VideoWidget;
class VideoWidgetControl : public QVideoWidgetControl {
    Q_OBJECT
public:
    explicit VideoWidgetControl(MediaPlayerControl* player, QObject* parent = nullptr);
    ~VideoWidgetControl() override;

    bool isFullScreen() const override;
    void setFullScreen(bool fullScreen) override;
    Qt::AspectRatioMode aspectRatioMode() const override;
    void setAspectRatioMode(Qt::AspectRatioMode mode) override;
    int brightness() const override;
    void setBrightness(int brightness) override;
    int contrast() const override;
    void setContrast(int contrast) override;
    int hue() const override;
    void setHue(int hue) override;
    int saturation() const override;
    void setSaturation(int saturation) override;
    QWidget* videoWidget() override;
private:
    bool fs_ = false;
    Qt::AspectRatioMode am_ = Qt::KeepAspectRatio;
    VideoWidget* vw_ = nullptr;
    MediaPlayerControl* mpc_ = nullptr;
};
