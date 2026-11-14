#pragma once

#include "vkm_opengl/core/object.h"
#include "vkm_opengl/texture/texture.h"

namespace Vkm::OpenGL {

/**
 * @brief Sampler object decoupling filter/wrap state from texture storage.
 *
 * A texture carries its own wrap and filter; a Sampler holds them apart and, bound to a
 * texture unit (glBindSampler), overrides them, so one texture is sampled with different filters.
 * Mirrors the Vulkan VkSampler model.
 */
class Sampler : public Object {
    public:
        struct Params {
            TextureWrap      wrapS     = TextureWrap::Repeat;
            TextureWrap      wrapT     = TextureWrap::Repeat;
            TextureWrap      wrapR     = TextureWrap::Repeat;
            TextureMinFilter minFilter = TextureMinFilter::LinearMipmapLinear;
            TextureMagFilter magFilter = TextureMagFilter::Linear;
            TextureCompare   compare   = TextureCompare::None;
        };

        explicit Sampler(const Params& params);
        ~Sampler() override;

        Sampler(const Sampler& other) = delete;
        Sampler& operator=(const Sampler& other) = delete;

        Sampler(Sampler && other) noexcept = default;
        Sampler& operator=(Sampler && other) noexcept;

    public:
        /**
         * @brief Bind this sampler to a texture unit.
         *
         * The sampler's wrap/filter state then overrides whatever the texture
         * bound to the same unit carries, which is the point of the type.
         *
         * @param slot Texture unit index.
         */
        void bindSlot(uint32_t slot) const;
};

} // namespace Vkm::OpenGL
