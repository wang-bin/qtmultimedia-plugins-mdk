#pragma once
#include <QMetaDataReaderControl>

class MediaPlayerControl;
class MetaDataReaderControl final: public QMetaDataReaderControl
{
public:
    MetaDataReaderControl(MediaPlayerControl* mpc, QObject *parent = nullptr);

    bool isMetaDataAvailable() const override {return !tag_.isEmpty();}
    QVariant metaData(const QString &key) const override {return tag_.value(key);}
    QStringList availableMetaData() const override {return tag_.keys();}
private:
    void readMetaData();
private:
    MediaPlayerControl* mpc_ = nullptr;
    QVariantMap tag_;
};