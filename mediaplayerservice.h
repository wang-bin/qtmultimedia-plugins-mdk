/*
 * Copyright (C) 2018 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#pragma once
#include <QMediaService>

class MediaPlayerControl;
class VideoWindowControl;

class MediaPlayerService : public QMediaService {
    Q_OBJECT
public:
    explicit MediaPlayerService(QObject* parent = nullptr);
    QMediaControl* requestControl(const char* name) override;
    void releaseControl(QMediaControl* control) override;
private:
    MediaPlayerControl* mpc_ = nullptr;
};
