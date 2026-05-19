/*
 * Copyright (C) 2022 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#pragma once
#include <private/qplatformmediaplugin_p.h>

class MDKMediaPlugin final : public QPlatformMediaPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "../mdkmediaservice.json")
public:
    explicit MDKMediaPlugin(QObject *parent = nullptr);
    QPlatformMediaIntegration *create(const QString &key) override;
};
