#include "vkm_opengl/core/object.h"

#include "vkm_opengl/core/error_handle.h"

namespace Vkm::OpenGL {

Object::Object(GLenum target, GLenum identifier, uint32_t id)
    : m_target(target)
    , m_identifier(identifier)
    , m_id(id) {}

Object::Object(Object && other) noexcept
    : m_target(other.m_target)
    , m_identifier(other.m_identifier)
    , m_id(other.m_id) {
    other.m_id = 0;
}

Object& Object::operator=(Object && other) noexcept {
    // Note: derived classes must release their GL handle before delegating
    // to this operator - this base move-assignment simply transfers state.
    if (this != &other) {
        m_target     = other.m_target;
        m_identifier = other.m_identifier;
        m_id         = other.m_id;
        other.m_id   = 0;
    }
    return *this;
}

uint32_t Object::getID() const { return m_id; }

void Object::setLabel(const char* label) {
    VKM_OPENGL_CHECK(glObjectLabel(m_identifier, m_id, -1, label));
}

} // namespace Vkm::OpenGL
