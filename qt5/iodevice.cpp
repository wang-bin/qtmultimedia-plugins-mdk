/*
 * Copyright (C) 2018-2019 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#include "iodevice.h"
#ifdef MDK_ABI
#include <QFile>
#include <QtDebug>

void QMediaIO::registerOnce()
{
    MediaIO::registerOnce("QIODevice", []{ return new QMediaIO();});
}

QMediaIO::QMediaIO(QIODevice* io)
    : MediaIO()
    , owns_io_(!!io)
    , io_(io)
{        
}

bool QMediaIO::onUrlChanged()
{
    if (io_)
        io_->close();
    if (owns_io_) {
        delete io_;
        io_ = nullptr;
    }
    if (url().empty())
        return true;
    if (!io_) {
        auto s = QString::fromStdString(url());
        static const std::string kScheme = "qio:";
        if (url().substr(0, kScheme.size()) != kScheme) {
            io_ = new QFile(s);
        } else {
            io_ = (QIODevice*)s.mid(4).toULongLong();
        }
    }
    if (io_->isOpen() && io_->isReadable())
        return true;
    if (!io_->open(accessMode() == MediaIO::AccessMode::Write ? QIODevice::WriteOnly : QIODevice::ReadOnly)) {
        qWarning() << "Failed to open qiodevice: " << io_->errorString();  
        if (owns_io_) {
            delete io_;
            io_ = nullptr;
        }
        return false;
    }
    return true;
}

bool QMediaIO::seek(int64_t offset, int from)
{
    if (!io_)
        return false;
    int p = offset;
    if (from == SEEK_CUR)
        p += io_->pos();
    else if (from == SEEK_END)
        p += size(); // offset < 0
    return io_->seek(p);
}
#endif // MDK_ABI