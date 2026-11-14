#include "vkm_opengl/texture/sampler.h"

#include "vkm_log/assert.h"

#include "vkm_opengl/core/error_handle.h"

namespace Vkm::OpenGL {

Sampler::Sampler(const Params& params)
    : Object(0, GL_SAMPLER, 0)
{
    VKM_OPENGL_CHECK(glCreateSamplers(1, &m_id));
    VKM_ASSERT(m_id != 0);
    VKM_OPENGL_CHECK(glSamplerParameteri(m_id, GL_TEXTURE_WRAP_S, toGLenum(params.wrapS)));
    VKM_OPENGL_CHECK(glSamplerParameteri(m_id, GL_TEXTURE_WRAP_T, toGLenum(params.wrapT)));
    VKM_OPENGL_CHECK(glSamplerParameteri(m_id, GL_TEXTURE_WRAP_R, toGLenum(params.wrapR)));
    VKM_OPENGL_CHECK(glSamplerParameteri(m_id, GL_TEXTURE_MIN_FILTER, toGLenum(params.minFilter)));
    VKM_OPENGL_CHECK(glSamplerParameteri(m_id, GL_TEXTURE_MAG_FILTER, toGLenum(params.magFilter)));

    // The mode is what turns comparison on; the function only says which way.
    const bool comparing = params.compare != TextureCompare::None;
    const GLint mode     = comparing ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE;
    VKM_OPENGL_CHECK(glSamplerParameteri(m_id, GL_TEXTURE_COMPARE_MODE, mode));
    VKM_OPENGL_CHECK(glSamplerParameteri(m_id, GL_TEXTURE_COMPARE_FUNC, toGLenum(params.compare)));
}

Sampler::~Sampler() {
    if (m_id == 0) return;
    VKM_OPENGL_CHECK(glDeleteSamplers(1, &m_id));
}

Sampler& Sampler::operator=(Sampler && other) noexcept {
    if (this != &other) {
        if (m_id != 0) {
            VKM_OPENGL_CHECK(glDeleteSamplers(1, &m_id));
        }
        Object::operator=(std::move(other));
    }
    return *this;
}

void Sampler::bindSlot(uint32_t slot) const {
    VKM_OPENGL_CHECK(glBindSampler(slot, m_id));
}

} // namespace Vkm::OpenGL
