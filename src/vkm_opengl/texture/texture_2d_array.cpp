#include "vkm_opengl/texture/texture_2d_array.h"

#include <algorithm>
#include <utility>

#include "vkm_log/assert.h"

#include "vkm_opengl/core/error_handle.h"

namespace Vkm::OpenGL {

namespace {

// Wrap, filter and anisotropy of @p params on texture @p id.
void applySampling(uint32_t id, const Texture2DArrayParams& params) {
    const auto set = [id](GLenum name, GLenum value) {
        VKM_OPENGL_CHECK(glTextureParameteri(id, name, static_cast<GLint>(value)));
    };
    set(GL_TEXTURE_WRAP_S, toGLenum(params.wrapS));
    set(GL_TEXTURE_WRAP_T, toGLenum(params.wrapT));
    set(GL_TEXTURE_MIN_FILTER, toGLenum(params.minFilter));
    set(GL_TEXTURE_MAG_FILTER, toGLenum(params.magFilter));
    // Before 4.6 the enum exists only with the extension; asking without it is an error.
    const float degree = std::min(std::max(params.maxAnisotropy, 1.0f), maxSupportedAnisotropy());
    if (degree > 1.0f) VKM_OPENGL_CHECK(glTextureParameterf(id, GL_TEXTURE_MAX_ANISOTROPY, degree));
}

} // namespace

Texture2DArray::Texture2DArray(const std::string& name, const Texture2DArrayParams& params)
    : Object(GL_TEXTURE_2D_ARRAY, GL_TEXTURE, 0)
    , m_name(name)
    , m_params(params)
{
    VKM_ASSERT(params.width > 0 && params.height > 0 && params.layers > 0 && params.levels > 0);
    VKM_OPENGL_CHECK(glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_id));
    setLabel(m_name.c_str());
    VKM_OPENGL_CHECK(glTextureStorage3D(
        m_id,
        static_cast<GLsizei>(m_params.levels),
        m_params.internalFormat,
        static_cast<GLsizei>(m_params.width),
        static_cast<GLsizei>(m_params.height),
        static_cast<GLsizei>(m_params.layers)
    ));
    if (m_params.viewFormat == GL_NONE) {
        applySampling(m_id, m_params);
        return;
    }
    // Generated, not created: glTextureView gives the object to a name never bound.
    VKM_OPENGL_CHECK(glGenTextures(1, &m_view));
    VKM_OPENGL_CHECK(glTextureView(
        m_view,
        GL_TEXTURE_2D_ARRAY,
        m_id,
        m_params.viewFormat,
        0,
        m_params.levels,
        0,
        m_params.layers
    ));
    VKM_OPENGL_CHECK(glObjectLabel(GL_TEXTURE, m_view, -1, m_name.c_str()));
    applySampling(m_view, m_params);
}

Texture2DArray::~Texture2DArray() {
    release();
}

Texture2DArray::Texture2DArray(Texture2DArray && other) noexcept
    : Object(std::move(other))
    , m_name(std::move(other.m_name))
    , m_params(other.m_params)
    , m_view(std::exchange(other.m_view, 0u)) {}

Texture2DArray& Texture2DArray::operator=(Texture2DArray && other) noexcept {
    if (this != &other) {
        release();
        Object::operator=(std::move(other));
        m_name   = std::move(other.m_name);
        m_params = other.m_params;
        m_view   = std::exchange(other.m_view, 0u);
    }
    return *this;
}

void Texture2DArray::release() noexcept {
    if (m_view != 0) {
        deleteTextures(1, &m_view);
        m_view = 0;
    }
    if (m_id == 0) return;
    deleteTextures(1, &m_id);
    m_id = 0;
}

void Texture2DArray::bindSlot(uint32_t slot) const {
    bindTextureUnit(slot, m_view != 0 ? m_view : m_id);
}

void Texture2DArray::bindImage(uint32_t unit, GLenum access, uint32_t level) const {
    const auto at = static_cast<GLint>(level);
    VKM_OPENGL_CHECK(glBindImageTexture(unit, m_id, at, GL_TRUE, 0, access, m_params.internalFormat));
}

void Texture2DArray::clear(uint32_t level, GLenum format, GLenum type, const void* value) const {
    VKM_OPENGL_CHECK(glClearTexImage(m_id, static_cast<GLint>(level), format, type, value));
}

void Texture2DArray::copyLayer(const Texture2DArray& from, uint32_t layer, uint32_t level) const {
    const auto width  = static_cast<GLsizei>(std::max(m_params.width >> level, 1u));
    const auto height = static_cast<GLsizei>(std::max(m_params.height >> level, 1u));
    const auto at     = static_cast<GLint>(layer);
    const auto mip    = static_cast<GLint>(level);
    VKM_OPENGL_CHECK(glCopyImageSubData(
        from.m_id,
        GL_TEXTURE_2D_ARRAY,
        mip,
        0,
        0,
        at,
        m_id,
        GL_TEXTURE_2D_ARRAY,
        mip,
        0,
        0,
        at,
        width,
        height,
        1
    ));
}

void Texture2DArray::generateMipmaps() const {
    VKM_OPENGL_CHECK(glGenerateTextureMipmap(m_id));
}

} // namespace Vkm::OpenGL
