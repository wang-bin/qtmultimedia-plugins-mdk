#pragma once
// qio: qrc: etc
#include "mdk/MediaIO.h"
#include <QIODevice>
using namespace MDK_NS;

class QMediaIO final : public MediaIO
{
public:
    static void registerOnce();

    QMediaIO(QIODevice* io = nullptr);

    const char* name() const override { return "QIODevice";}
    const std::set<std::string>& protocols() const override {
        static const std::set<std::string> s{"qio", "qrc"};
        return s;
    }

    bool isSeekable() const override { return io_ && !io_->isSequential(); }
    bool isWritable() const override { return io_ && io_->isWritable(); }
    int64_t read(uint8_t *data, int64_t maxSize) override {
        if (!io_)
            return 0;
        return io_->read((char*)data, maxSize);
    }
    int64_t write(const uint8_t *data, int64_t maxSize) override {
        if (!io_)
            return 0;
        return io_->write((const char*)data, maxSize);
    }

    bool seek(int64_t offset, int from) override;
    int64_t position() const override {
        if (!io_)
            return 0;
        return io_->pos();
    }

    int64_t size() const override {
        if (!io_)
            return 0;
        return io_->size();
    }
protected:
    bool onUrlChanged() override;
private:
    bool owns_io_ = true;
    QIODevice* io_ = nullptr;
};

