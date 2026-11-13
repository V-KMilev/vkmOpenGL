#pragma once

#include <cstdint>

#include <GL/glew.h>

namespace Vkm::OpenGL {

/**
 * @brief Base for the things OpenGL identifies by an integer handle.
 *
 * Carries the handle, the object type glObjectLabel needs, and the ownership
 * rules: non-copyable, since a GL handle has a single owner, and movable so a
 * wrapper can be returned by value or stored in a container. A move zeros the
 * source's m_id, which makes the moved-from destructor a no-op.
 *
 * Declares no bind(): a program and a vertex array have no target to select,
 * and a sampler binds to a texture unit, so each class declares the binding
 * call that suits it.
 */
class Object {
    public:
        Object() = delete;

        Object(const Object& other) = delete;
        Object& operator=(const Object& other) = delete;

        Object(Object && other) noexcept;
        Object& operator=(Object && other) noexcept;

        virtual ~Object() = default;

    protected:
        Object(
            GLenum target,
            GLenum identifier,
            uint32_t id
        );

    public:
        /**
         * @brief Get the OpenGL id/name for this object.
         * @return The GLuint identifier for this GL object.
         */
        uint32_t getID() const;

        /**
         * @brief Check whether the GL object holds a valid OpenGL id.
         * @return True if the object is valid, false otherwise.
         */
        bool isValid() const;

        /**
         * @brief Set a debug label for this OpenGL object, if supported.
         * @param label Null-terminated C-string to set as the label.
         */
        void setLabel(const char* label);

    protected:
        GLenum   m_target;
        GLenum   m_identifier;
        uint32_t m_id;
};

} // namespace Vkm::OpenGL
