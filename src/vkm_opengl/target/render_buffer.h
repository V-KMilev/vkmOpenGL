#pragma once

#include "vkm_opengl/core/object.h"

namespace Vkm::OpenGL {

/**
 * @brief Encapsulates an OpenGL renderbuffer object.
 *
 * RenderBuffer provides creation and storage allocation of OpenGL renderbuffer
 * objects. Renderbuffers are frequently used as attachments for framebuffers.
 */
class RenderBuffer : public Object {
    public:
        RenderBuffer();
        ~RenderBuffer() override;

        RenderBuffer(const RenderBuffer& other) = delete;
        RenderBuffer& operator=(const RenderBuffer& other) = delete;

        RenderBuffer(RenderBuffer && other) noexcept = default;
        RenderBuffer& operator=(RenderBuffer && other) noexcept;

    public:
        /**
         * @brief Allocates storage for the renderbuffer in single-sample (non-multisample) mode.
         * @param internalFormat Internal format for the renderbuffer (e.g., GL_DEPTH24_STENCIL8).
         * @param width Width in pixels.
         * @param height Height in pixels.
         */
        void storage(GLenum internalFormat, int32_t width, int32_t height);

    private:
        /**
         * @brief Delete the renderbuffer and zero m_id.
         *
         * Idempotent, so it is safe on a moved-from renderbuffer or after a previous release().
         */
        void release() noexcept;
};

} // namespace Vkm::OpenGL
