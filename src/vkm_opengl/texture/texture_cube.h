#pragma once

#include <cstdint>
#include <utility>

#include <GL/glew.h>

#include "vkm_opengl/core/error_handle.h"
#include "vkm_opengl/core/object.h"
#include "vkm_opengl/texture/texture.h"  // bindTextureUnit, deleteTextures

namespace Vkm::OpenGL {

/**
 * @brief RAII cubemap texture wrapper (per-face / per-mip render targets).
 *
 * Owns a GL_TEXTURE_CUBE_MAP: allocate all six faces (+ optional mip chain),
 * generate mips, bind to a sampler slot. A face/mip is drawn into through
 * FrameBuffer::attachLayer, the face as the layer.
 *
 * An Object like every other handle-owning wrapper here, so the ownership
 * rules - non-copyable, movable, a moved-from object left empty - are the base's
 * rather than a fourth hand-written copy of them. That copy is where a move
 * forgets a member, which is a bug this module has had.
 */
class TextureCube : public Object {
    public:
        TextureCube() : Object(GL_TEXTURE_CUBE_MAP, GL_TEXTURE, 0) {}
        ~TextureCube() override { release(); }

        TextureCube(const TextureCube& other) = delete;
        TextureCube& operator=(const TextureCube& other) = delete;

        TextureCube(TextureCube && other) noexcept
            : Object(std::move(other)), m_size(other.m_size), m_mips(other.m_mips) {}

        TextureCube& operator=(TextureCube && other) noexcept {
            if (this != &other) {
                release();
                Object::operator=(std::move(other));
                m_size = other.m_size;
                m_mips = other.m_mips;
            }
            return *this;
        }

    public:
        /**
         * @brief Allocate the cubemap (six faces, `mips` levels) with immutable storage.
         *
         * @param size           Per-face edge length in texels.
         * @param mips           Mip level count (1 = no chain).
         * @param internalFormat Sized, e.g. GL_RGB16F.
         * @param mipFiltering   Trilinear minification when true.
         */
        void create(int size, int mips, GLenum internalFormat, bool mipFiltering) {
            release();
            m_size = size;
            m_mips = mips;

            VKM_OPENGL_CHECK(glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_id));
            VKM_OPENGL_CHECK(glTextureStorage2D(m_id, mips, internalFormat, size, size));
            VKM_OPENGL_CHECK(glTextureParameteri(m_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            VKM_OPENGL_CHECK(glTextureParameteri(m_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
            VKM_OPENGL_CHECK(glTextureParameteri(m_id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
            const GLint minFilter = mipFiltering ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
            VKM_OPENGL_CHECK(glTextureParameteri(m_id, GL_TEXTURE_MIN_FILTER, minFilter));
            VKM_OPENGL_CHECK(glTextureParameteri(m_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        }

        void generateMipmaps() const {
            VKM_OPENGL_CHECK(glGenerateTextureMipmap(m_id));
        }

        void bindSlot(uint32_t slot) const {
            bindTextureUnit(slot, m_id);
        }

        int size() const { return m_size; }
        int mips() const { return m_mips; }

    private:
        void release() noexcept {
            if (m_id == 0) return;
            deleteTextures(1, &m_id);
            m_id = 0;
        }

    private:
        int m_size = 0;
        int m_mips = 1;
};

} // namespace Vkm::OpenGL
