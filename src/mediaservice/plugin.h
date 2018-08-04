/*
 * Copyright (C) 2018 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#pragma once
#include <QMediaServiceProviderPlugin>
// TODO: formats
class MDKPlugin : public QMediaServiceProviderPlugin
//        , public QMediaServiceSupportedFormatsInterface
        , public QMediaServiceFeaturesInterface {
    Q_OBJECT
//    Q_INTERFACES(QMediaServiceSupportedFormatsInterface)
    Q_INTERFACES(QMediaServiceFeaturesInterface)
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.mediaserviceproviderfactory/5.0" FILE "plugin.json")
public:
    QMediaService* create(QString const& key) Q_DECL_OVERRIDE;
    void release(QMediaService *service) Q_DECL_OVERRIDE;

    QMediaServiceProviderHint::Features supportedFeatures(const QByteArray &service) const Q_DECL_OVERRIDE;
};
