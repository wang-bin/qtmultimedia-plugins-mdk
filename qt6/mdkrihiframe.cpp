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
        return plane == 0 && ctx_ ? ctx_->texture.get() : nullptr;
    }

private:
    std::shared_ptr<MDKRhiContext> ctx_;
};

} // namespace

void MDKRhiContext::reset()
{
    apiBound = false;
    rp.reset();
    rt.reset();
    texture.reset();
    rhi = nullptr;
    size = {};
}

bool MDKRhiContext::ensure(QRhi &newRhi, QSize frameSize)
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
    return bindRenderAPI();
}

bool MDKRhiContext::bindRenderAPI()
{
    if (!player || !rhi || !texture)
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
        player->setRenderAPI(&ra);
        player->scale(1.0f, -1.0f);
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
        player->setRenderAPI(&ra);
        player->scale(1.0f, 1.0f);
        break;
    }
#endif
#if defined(Q_OS_WIN)
    case QRhi::D3D11: {
        D3D11RenderAPI ra{};
        ra.rtv = reinterpret_cast<ID3D11DeviceChild *>(quintptr(texture->nativeTexture().object));
        player->setRenderAPI(&ra);
        player->scale(1.0f, 1.0f);
        break;
    }
    case QRhi::D3D12: {
        const auto *d3dnat = static_cast<const QRhiD3D12NativeHandles *>(nat);
        D3D12RenderAPI ra{};
        ra.cmdQueue = reinterpret_cast<ID3D12CommandQueue *>(d3dnat->commandQueue);
        ra.rt = reinterpret_cast<ID3D12Resource *>(quintptr(texture->nativeTexture().object));
        player->setRenderAPI(&ra);
        player->scale(1.0f, 1.0f);
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
            auto *self = static_cast<MDKRhiContext *>(opaque);
            *w = self->size.width();
            *h = self->size.height();
            *fmt = VK_FORMAT_R8G8B8A8_UNORM;
            *layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            return 1;
        };
        // Offscreen: MDK owns the command buffer when currentCommandBuffer is null.
        player->setRenderAPI(&ra);
        player->scale(1.0f, 1.0f);
        break;
    }
#endif
    default:
        return false;
    }

    player->setVideoSurfaceSize(size.width(), size.height());
    apiBound = true;
    return true;
}

MDKRhiVideoBuffer::MDKRhiVideoBuffer(std::shared_ptr<MDKRhiContext> ctx, QSize size)
    : QHwVideoBuffer(QVideoFrame::RhiTextureHandle, ctx ? ctx->rhi : nullptr)
    , ctx_(std::move(ctx))
    , size_(size)
{
}

MDKRhiVideoBuffer::~MDKRhiVideoBuffer() = default;

QAbstractVideoBuffer::MapData MDKRhiVideoBuffer::map(QVideoFrame::MapMode)
{
    return {};
}

QVideoFrameTexturesUPtr MDKRhiVideoBuffer::mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr &)
{
    if (!ctx_ || !ctx_->player || size_.isEmpty())
        return {};

    std::lock_guard<std::mutex> lock(ctx_->mutex);
    if (!ctx_->ensure(rhi, size_))
        return {};

    // QHwVideoBuffer::rhi() should match the sink RHI used for mapping.
    m_rhi = &rhi;

    if (!rendered_) {
        if (!ctx_->apiBound && !ctx_->bindRenderAPI())
            return {};
        ctx_->player->renderVideo();
        rendered_ = true;
    }

    return std::make_unique<MDKFrameTextures>(ctx_);
}
