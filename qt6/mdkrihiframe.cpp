/*
 * Copyright (C) 2026 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 */
#include "mdkrihiframe.h"

#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>

#if QT_CONFIG(opengl)
#  include <QtGui/private/qrhigles2_p.h>
#endif
#if defined(Q_OS_WIN)
#  include <d3d11.h>
#  include <d3d12.h>
#endif
#if QT_CONFIG(vulkan) && __has_include(<vulkan/vulkan.h>)
#  include <vulkan/vulkan.h>
#endif

#include "mdk/Player.h"
#include "mdk/RenderAPI.h"

using namespace MDK_NS;

#if !MDK_HAS_QT_RHI_TEXTURE_POOL
namespace {

class MDKFrameTextures final : public QVideoFrameTextures
{
public:
    explicit MDKFrameTextures(std::shared_ptr<MDKRhiContext> ctx)
        : ctx_(std::move(ctx))
    {
    }

    QRhiTexture *texture(uint plane) const override
    {
        return plane == 0 && ctx_ ? ctx_->sharedTarget.texture.get() : nullptr;
    }

private:
    std::shared_ptr<MDKRhiContext> ctx_;
};

} // namespace
#endif

void MDKRhiRenderTarget::reset()
{
    apiBound = false;
    rp.reset();
    rt.reset();
    texture.reset();
    rhi = nullptr;
    size = {};
}

bool MDKRhiRenderTarget::ensure(QRhi &newRhi, QSize frameSize)
{
    if (rhi == &newRhi && size == frameSize && texture && rt && rp)
        return true;

    reset();
    rhi = &newRhi;
    size = frameSize;

    texture.reset(newRhi.newTexture(QRhiTexture::RGBA8, frameSize, 1,
                                    QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    if (!texture || !texture->create()) {
        reset();
        return false;
    }

    QRhiColorAttachment color0(texture.get());
    rt.reset(newRhi.newTextureRenderTarget({ color0 }));
    if (!rt) {
        reset();
        return false;
    }
    rp.reset(rt->newCompatibleRenderPassDescriptor());
    if (!rp) {
        reset();
        return false;
    }
    rt->setRenderPassDescriptor(rp.get());
    if (!rt->create()) {
        reset();
        return false;
    }

    apiBound = false;
    return true;
}

bool MDKRhiRenderTarget::bindRenderAPI(Player &player)
{
    if (!rhi || !texture)
        return false;

    const QRhiNativeHandles *nat = rhi->nativeHandles();
    if (!nat)
        return false;

    switch (rhi->backend()) {
#if QT_CONFIG(opengl)
    case QRhi::OpenGLES2: {
        auto *glrt = static_cast<QGles2TextureRenderTarget *>(rt.get());
        GLRenderAPI ra{};
        ra.fbo = int(glrt->framebuffer);
        player.setRenderAPI(&ra);
        player.scale(1.0f, -1.0f);
        break;
    }
#endif
#if QT_CONFIG(metal)
    case QRhi::Metal: {
        const auto *mtlnat = static_cast<const QRhiMetalNativeHandles *>(nat);
        MetalRenderAPI ra{};
        ra.device = mtlnat->dev;
        ra.cmdQueue = mtlnat->cmdQueue;
        ra.texture = reinterpret_cast<const void *>(quintptr(texture->nativeTexture().object));
        player.setRenderAPI(&ra);
        player.scale(1.0f, 1.0f);
        break;
    }
#endif
#if defined(Q_OS_WIN)
    case QRhi::D3D11: {
        D3D11RenderAPI ra{};
        ra.rtv = reinterpret_cast<ID3D11DeviceChild *>(quintptr(texture->nativeTexture().object));
        player.setRenderAPI(&ra);
        player.scale(1.0f, 1.0f);
        break;
    }
    case QRhi::D3D12: {
        const auto *d3dnat = static_cast<const QRhiD3D12NativeHandles *>(nat);
        D3D12RenderAPI ra{};
        ra.cmdQueue = reinterpret_cast<ID3D12CommandQueue *>(d3dnat->commandQueue);
        ra.rt = reinterpret_cast<ID3D12Resource *>(quintptr(texture->nativeTexture().object));
        player.setRenderAPI(&ra);
        player.scale(1.0f, 1.0f);
        break;
    }
#endif
#if QT_CONFIG(vulkan) && __has_include(<vulkan/vulkan.h>)
    case QRhi::Vulkan: {
        const auto *vknat = static_cast<const QRhiVulkanNativeHandles *>(nat);
        VulkanRenderAPI ra{};
        ra.device = vknat->dev;
        ra.phy_device = vknat->physDev;
        ra.opaque = this;
        ra.rt = VkImage(texture->nativeTexture().object);
        ra.renderTargetInfo = [](void *opaque, int *w, int *h, VkFormat *fmt, VkImageLayout *layout) {
            auto *self = static_cast<MDKRhiRenderTarget *>(opaque);
            *w = self->size.width();
            *h = self->size.height();
            *fmt = VK_FORMAT_R8G8B8A8_UNORM;
            *layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            return 1;
        };
        // Offscreen: MDK owns the command buffer when currentCommandBuffer is null.
        player.setRenderAPI(&ra);
        player.scale(1.0f, 1.0f);
        break;
    }
#endif
    default:
        return false;
    }

    player.setVideoSurfaceSize(size.width(), size.height());
    apiBound = true;
    return true;
}

void MDKRhiContext::reset()
{
#if !MDK_HAS_QT_RHI_TEXTURE_POOL
    sharedTarget.reset();
#endif
}

#if MDK_HAS_QT_RHI_TEXTURE_POOL
MDKRhiFrameTextures::MDKRhiFrameTextures(std::unique_ptr<MDKRhiRenderTarget> target)
    : target_(std::move(target))
{
}

QRhiTexture *MDKRhiFrameTextures::texture(uint plane) const
{
    return plane == 0 && target_ ? target_->texture.get() : nullptr;
}

bool MDKRhiFrameTextures::usesRhi(const QRhi &rhi) const
{
    return target_ && target_->rhi == &rhi;
}

std::unique_ptr<MDKRhiRenderTarget> MDKRhiFrameTextures::takeRenderTarget()
{
    return std::move(target_);
}

std::unique_ptr<MDKRhiRenderTarget>
mdkAcquireRhiRenderTarget(QRhi &rhi, QSize frameSize, QVideoFrameTexturesUPtr &oldTextures)
{
    std::unique_ptr<MDKRhiRenderTarget> target;
    if (auto *old = dynamic_cast<MDKRhiFrameTextures *>(oldTextures.get())) {
        // oldTextures belongs to the current QRhi frame slot. Reuse its allocation only when it
        // was created by the same QRhi; other slots keep exclusive ownership of their textures.
        if (old->usesRhi(rhi))
            target = old->takeRenderTarget();
    }

    if (!target)
        target = std::make_unique<MDKRhiRenderTarget>();
    if (!target->ensure(rhi, frameSize))
        return {};
    return target;
}
#endif

MDKRhiVideoBuffer::MDKRhiVideoBuffer(std::shared_ptr<MDKRhiContext> ctx, QSize size)
    : QHwVideoBuffer(QVideoFrame::RhiTextureHandle,
#if MDK_HAS_QT_RHI_TEXTURE_POOL
                     nullptr)
#else
                     ctx ? ctx->sharedTarget.rhi : nullptr)
#endif
    , ctx_(std::move(ctx))
    , size_(size)
{
}

MDKRhiVideoBuffer::~MDKRhiVideoBuffer() = default;

QAbstractVideoBuffer::MapData MDKRhiVideoBuffer::map(QVideoFrame::MapMode)
{
    return {};
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 2)
QVideoFrameTexturesUPtr MDKRhiVideoBuffer::mapTextures(QRhi &rhi,
                                                       QVideoFrameTexturesUPtr &oldTextures)
#else
std::unique_ptr<QVideoFrameTextures> MDKRhiVideoBuffer::mapTextures(QRhi *rhi)
#endif
{
    if (!ctx_ || !ctx_->player || size_.isEmpty()
#if QT_VERSION < QT_VERSION_CHECK(6, 8, 2)
            || !rhi
#endif
    )
        return {};

    std::lock_guard<std::mutex> lock(ctx_->mutex);
    // QHwVideoBuffer::rhi() should match the sink RHI used for mapping.
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 2)
    m_rhi = &rhi;
#else
    m_rhi = rhi;
    QRhi &mappedRhi = *rhi;
#endif

#if MDK_HAS_QT_RHI_TEXTURE_POOL
    auto target = mdkAcquireRhiRenderTarget(rhi, size_, oldTextures);
    if (!target)
        return {};

    // The player has one default renderer, while each Qt pool slot has a different target.
    // Rebind before every draw so renderVideo() cannot overwrite another live frame's texture.
    if (!target->bindRenderAPI(*ctx_->player))
        return {};
    ctx_->player->renderVideo();
    return std::make_unique<MDKRhiFrameTextures>(std::move(target));
#else
    // Qt 6.8.0/6.8.1 have no oldTextures argument, so they use the legacy shared target.
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 2)
    QRhi &mappedRhi = rhi;
#endif
    if (!ctx_->sharedTarget.ensure(mappedRhi, size_))
        return {};

    if (!rendered_) {
        if (!ctx_->sharedTarget.apiBound && !ctx_->sharedTarget.bindRenderAPI(*ctx_->player))
            return {};
        ctx_->player->renderVideo();
        rendered_ = true;
    }

    return std::make_unique<MDKFrameTextures>(ctx_);
#endif
}
