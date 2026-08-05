/*
 * Copyright (C) 2026 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#pragma once

#include <QtCore/qglobal.h>

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#  include <QtMultimedia/private/qhwvideobuffer_p.h>
#else
#  include <QtMultimedia/private/qabstractvideobuffer_p.h>
#endif

#include <rhi/qrhi.h>

#include <memory>
#include <mutex>

#include <QSize>

#include "mdk/global.h"

MDK_NS_BEGIN
class Player;
MDK_NS_END

// The public build option is deliberately opt-in. MDK_HAS_QT_RHI_TEXTURE_POOL records whether
// the requested mode is also supported by the Qt private API available at compile time.
#if (MDK_USE_QT_RHI_TEXTURE_POOL+0) \
        && QT_VERSION >= QT_VERSION_CHECK(6, 8, 2) \
        && __has_include(<QtMultimedia/private/qvideoframetexturepool_p.h>)
#  define MDK_HAS_QT_RHI_TEXTURE_POOL 1
#else
#  define MDK_HAS_QT_RHI_TEXTURE_POOL 0
#endif

/*!
 * QRhi resources used as one MDK render target. Creation and rendering happen on the QRhi thread
 * through the Qt version's private video-buffer mapTextures() callback.
 */
struct MDKRhiRenderTarget
{
    QRhi *rhi = nullptr;
    QSize size;
    std::unique_ptr<QRhiTexture> texture;
    std::unique_ptr<QRhiTextureRenderTarget> rt;
    std::unique_ptr<QRhiRenderPassDescriptor> rp;
    bool apiBound = false;

    bool ensure(QRhi &rhi, QSize frameSize);
    bool bindRenderAPI(MDK_NS::Player &player);
    void reset();
};

/*!
 * State shared by all frames from one player. On Qt versions without QVideoFrameTexturePool the
 * render target remains here to preserve the legacy single-texture path.
 */
struct MDKRhiContext
{
    MDK_NS::Player *player = nullptr;
    std::mutex mutex;
#if !MDK_HAS_QT_RHI_TEXTURE_POOL
    MDKRhiRenderTarget sharedTarget;
#endif

    void reset();
};

#if MDK_HAS_QT_RHI_TEXTURE_POOL
/*!
 * Texture wrapper stored in one QVideoFrameTexturePool slot. Ownership can be transferred from
 * oldTextures only after Qt returns that slot for reuse.
 */
class MDKRhiFrameTextures final : public QVideoFrameTextures
{
public:
    explicit MDKRhiFrameTextures(std::unique_ptr<MDKRhiRenderTarget> target);

    QRhiTexture *texture(uint plane) const override;
    bool usesRhi(const QRhi &rhi) const;
    std::unique_ptr<MDKRhiRenderTarget> takeRenderTarget();

private:
    std::unique_ptr<MDKRhiRenderTarget> target_;
};

// Acquire a target for the current Qt texture-pool slot. Exposed internally for deterministic
// resource lifetime tests; callers still own the returned target exclusively.
std::unique_ptr<MDKRhiRenderTarget>
mdkAcquireRhiRenderTarget(QRhi &rhi, QSize frameSize, QVideoFrameTexturesUPtr &oldTextures);
#endif

// QHwVideoBuffer was introduced in Qt 6.8. Older supported Qt releases expose
// the same RHI texture callback on QAbstractVideoBuffer instead.
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
using MDKVideoBufferBase = QHwVideoBuffer;
#else
using MDKVideoBufferBase = QAbstractVideoBuffer;
#endif

class MDKRhiVideoBuffer final : public MDKVideoBufferBase
{
public:
    MDKRhiVideoBuffer(std::shared_ptr<MDKRhiContext> ctx, QSize size);
    ~MDKRhiVideoBuffer() override;

    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override {}
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 2)
    // Qt 6.5 through 6.7.1 still require mapMode() for every
    // QAbstractVideoBuffer; this RHI-only buffer never exposes a CPU mapping.
    QVideoFrame::MapMode mapMode() const override { return QVideoFrame::NotMapped; }
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 2)
    // Qt 6.8.2 added the oldTextures handoff used by QVideoFrameTexturePool.
    QVideoFrameTexturesUPtr mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr &oldTextures) override;
#else
    // Qt versions before 6.8.2 use the earlier pointer-only mapTextures() contract.
    std::unique_ptr<QVideoFrameTextures> mapTextures(QRhi *rhi) override;
#endif

private:
    std::shared_ptr<MDKRhiContext> ctx_;
    QSize size_;
#if !MDK_HAS_QT_RHI_TEXTURE_POOL
    bool rendered_ = false;
#endif
};
