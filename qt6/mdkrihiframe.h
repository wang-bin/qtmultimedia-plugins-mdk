/*
 * Copyright (C) 2026 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#pragma once

#include <QtMultimedia/private/qhwvideobuffer_p.h>

#include <rhi/qrhi.h>

#include <memory>
#include <mutex>

#include <QSize>

#include "mdk/global.h"

MDK_NS_BEGIN
class Player;
MDK_NS_END

/*!
 * Shared RHI render target for MDK → QVideoSink zero-copy frames.
 * Texture resources are created/used from the QRhi thread via mapTextures().
 */
struct MDKRhiContext
{
    MDK_NS::Player *player = nullptr;
    std::mutex mutex;
    QRhi *rhi = nullptr;
    QSize size;
    std::unique_ptr<QRhiTexture> texture;
    std::unique_ptr<QRhiTextureRenderTarget> rt;
    std::unique_ptr<QRhiRenderPassDescriptor> rp;
    bool apiBound = false;

    bool ensure(QRhi &rhi, QSize frameSize);
    bool bindRenderAPI();
    void reset();
};

class MDKRhiVideoBuffer final : public QHwVideoBuffer
{
public:
    MDKRhiVideoBuffer(std::shared_ptr<MDKRhiContext> ctx, QSize size);
    ~MDKRhiVideoBuffer() override;

    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override {}

    QVideoFrameTexturesUPtr mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr &oldTextures) override;

private:
    std::shared_ptr<MDKRhiContext> ctx_;
    QSize size_;
    bool rendered_ = false;
};
