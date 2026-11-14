#include "vkm_opengl/texture/texture_3d.h"

#include "vkm_log/assert.h"

#include "vkm_opengl/core/error_handle.h"

namespace Vkm::OpenGL {

Texture3D::Texture3D(const std::string& name, const Texture3DParams& params)
    : Object(GL_TEXTURE_3D, GL_TEXTURE, 0)
    , m_name(name)
    , m_params(params) {
    VKM_OPENGL_CHECK(glCreateTextures(GL_TEXTURE_3D, 1, &m_id));
    VKM_ASSERT(m_id != 0);
    setLabel(m_name.c_str());
    VKM_OPENGL_CHECK(glTextureStorage3D(
        m_id,
        1,
        m_params.internalFormat,
        static_cast<GLsizei>(m_params.width),
        static_cast<GLsizei>(m_params.height),
        static_cast<GLsizei>(m_params.depth)
    ));

    const auto wrap = static_cast<GLint>(toGLenum(m_params.wrap));
    VKM_OPENGL_CHECK(glTextureParameteri(m_id, GL_TEXTURE_WRAP_S, wrap));
    VKM_OPENGL_CHECK(glTextureParameteri(m_id, GL_TEXTURE_WRAP_T, wrap));
    VKM_OPENGL_CHECK(glTextureParameteri(m_id, GL_TEXTURE_WRAP_R, wrap));
    const auto minFilter = static_cast<GLint>(toGLenum(m_params.minFilter));
    const auto magFilter = static_cast<GLint>(toGLenum(m_params.magFilter));
    VKM_OPENGL_CHECK(glTextureParameteri(m_id, GL_TEXTURE_MIN_FILTER, minFilter));
    VKM_OPENGL_CHECK(glTextureParameteri(m_id, GL_TEXTURE_MAG_FILTER, magFilter));
}

Texture3D::~Texture3D() {
    release();
}

Texture3D& Texture3D::operator=(Texture3D && other) noexcept {
    if (this != &other) {
        release();
        // Through the base: Object owns the id and the two fields beside it.
        Object::operator=(std::move(other));
        m_name   = std::move(other.m_name);
        m_params = other.m_params;
    }
    return *this;
}

void Texture3D::release() noexcept {
    if (m_id == 0) return;
    deleteTextures(1, &m_id);
    m_id = 0;
}

void Texture3D::bindSlot(uint32_t slot) const {
    bindTextureUnit(slot, m_id);
}

void Texture3D::bindImage(uint32_t unit, GLenum access) const {
    // Layered = GL_TRUE exposes the whole volume as an image3D to the shader.
    VKM_OPENGL_CHECK(glBindImageTexture(unit, m_id, 0, GL_TRUE, 0, access, m_params.internalFormat));
}

void Texture3D::upload(const void* pixels) const {
    VKM_OPENGL_CHECK(glTextureSubImage3D(
        m_id,
        0,
        0,
        0,
        0,
        static_cast<GLsizei>(m_params.width),
        static_cast<GLsizei>(m_params.height),
        static_cast<GLsizei>(m_params.depth),
        m_params.format,
        m_params.type,
        pixels
    ));
}

} // namespace Vkm::OpenGL
