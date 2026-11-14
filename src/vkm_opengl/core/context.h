#pragma once

#include <cstdint>
#include <string>

#include <GL/glew.h>
#include <glm/glm.hpp>

namespace Vkm::OpenGL {

/**
 * @brief Depth state configuration for OpenGL context.
 */
struct DepthState {
    bool   enabled      = false;    ///< Enable/disable depth testing.
    GLenum func         = GL_LESS;  ///< Depth comparison function (e.g., GL_LESS, GL_EQUAL).
    bool   writeEnabled = true;     ///< Enable/disable writing to depth buffer.
};

/**
 * @brief Face culling state configuration for OpenGL context.
 */
struct FaceCullState {
    bool   enabled   = false;    ///< Enable/disable face culling.
    GLenum cullFace  = GL_BACK;  ///< Which face to cull (e.g., GL_BACK, GL_FRONT).
    GLenum frontFace = GL_CCW;   ///< Vertex winding order considered front-facing.
};

/**
 * @brief Blending state configuration for OpenGL context.
 */
struct BlendState {
    bool   enabled   = false;    ///< Enable/disable blending.
    GLenum srcFactor = GL_ONE;   ///< Source blend factor.
    GLenum dstFactor = GL_ZERO;  ///< Destination blend factor.
};

/**
 * @brief Per-channel color write mask.
 */
struct ColorMaskState {
    bool r = true;
    bool g = true;
    bool b = true;
    bool a = true;
};

/**
 * @brief Raster (pipeline) state: depth, culling, blending and the rest Context caches.
 */
struct RasterState {
    DepthState     depth;                     ///< Depth state.
    FaceCullState  cull;                      ///< Face culling state.
    BlendState     blend;                     ///< Blending state.
    ColorMaskState colorMask;                 ///< Per-channel color write mask.
    bool           colorMaskSplit = false;    ///< A draw buffer's mask was set apart from colorMask.
    GLenum         polygonMode    = GL_FILL;  ///< Polygon mode (e.g., GL_FILL, GL_LINE).
    bool           scissorEnabled = false;    ///< Enable/disable scissor test.

    bool  alphaToCoverage = false;  ///< Alpha read as multisample coverage.
    bool  depthClamp      = false;  ///< Depth clamped to the range rather than clipped.
    bool  offsetFill      = false;  ///< Polygon offset on filled polygons.
    bool  offsetLine      = false;  ///< On lines.
    bool  offsetPoint     = false;  ///< On points.
    float offsetFactor    = 0.0f;   ///< Polygon offset's slope factor.
    float offsetUnits     = 0.0f;   ///< Its constant, in depth units.
};

/**
 * @brief Core OpenGL context state manager and state change limiter.
 *
 * Caches the raster state it sets and skips a set that changes nothing; the viewport, the
 * scissor rectangle and the set-once state pass straight through.
 */
class Context {
    public:
        Context() = default;
        ~Context() = default;

        Context(const Context& other) = delete;
        Context& operator=(const Context& other) = delete;

        Context(Context && other) = delete;
        Context& operator=(Context && other) = delete;

        /**
         * @brief Set the color used when clearing the framebuffer.
         *
         * Stored only; applied to GL by the next clear() with the color bit set.
         *
         * @param color RGBA color vector.
         */
        void setClearColor(const glm::vec4& color);

        /**
         * @brief Retrieve the current clear color.
         * @return Reference to current RGBA clear color vector.
         */
        const glm::vec4& getClearColor() const { return m_clearColor; }

        /**
         * @brief Clear framebuffer buffers (color/depth).
         * @param color      Clear color buffer if true.
         * @param depth      Clear depth buffer if true.
         * @param depthValue What the depth buffer is cleared to: 1 is GL's far plane, 0 a reversed one's.
         */
        void clear(bool color = true, bool depth = true, float depthValue = 1.0f) const;

        /**
         * @brief Set the viewport region for rendering.
         *
         * Uncached, unlike the toggles and modes below - hence const: code
         * outside this class sets the viewport too (a mip chain sizes it to the
         * level it renders into), so a cache would go stale.
         *
         * @param x      Lower-left corner X coordinate.
         * @param y      Lower-left corner Y coordinate.
         * @param width  Width of the viewport.
         * @param height Height of the viewport.
         */
        void setViewport(int32_t x, int32_t y, int32_t width, int32_t height) const;

        /**
         * @brief Enable or disable scissor testing.
         * @param enable True to enable, false to disable.
         */
        void enableScissor(bool enable);

        /**
         * @brief Set the scissor rectangle.
         * @param x      Lower-left corner X coordinate.
         * @param y      Lower-left corner Y coordinate.
         * @param width  Width of the rectangle.
         * @param height Height of the rectangle.
         */
        void setScissor(int32_t x, int32_t y, int32_t width, int32_t height) const;

        /**
         * @brief Enable or disable depth test.
         * @param enable True to enable, false to disable.
         */
        void setDepthTest(bool enable);

        /**
         * @brief Set the depth comparison function.
         * @param func OpenGL comparison function (e.g., GL_LESS, GL_GREATER).
         */
        void setDepthFunc(GLenum func);

        /**
         * @brief Enable or disable writing to the depth buffer.
         * @param enable True to enable, false to disable.
         */
        void setDepthWrite(bool enable);

        /**
         * @brief Enable or disable face culling.
         * @param enable True to enable, false to disable.
         */
        void setFaceCulling(bool enable);

        /**
         * @brief Set which faces to cull (front/back).
         * @param face OpenGL face enumeration.
         */
        void setCullFace(GLenum face);

        /**
         * @brief Set which winding (CW/CCW) is considered front-facing.
         * @param winding OpenGL front face winding.
         */
        void setFrontFace(GLenum winding);

        /**
         * @brief Get the currently set front face winding.
         * @return OpenGL front face winding.
         */
        GLenum getFrontFace() const { return m_state.cull.frontFace; }

        /**
         * @brief Enable or disable blending.
         * @param enable True to enable, false to disable.
         */
        void setBlending(bool enable);

        /**
         * @brief Set blending factors for source and destination.
         * @param src Source factor.
         * @param dst Destination factor.
         */
        void setBlendFunc(GLenum src, GLenum dst);

        /**
         * @brief Enable/disable per-channel writes to the color buffer.
         * @param r Enable writes to the red channel.
         * @param g Enable writes to the green channel.
         * @param b Enable writes to the blue channel.
         * @param a Enable writes to the alpha channel.
         */
        void setColorMask(bool r, bool g, bool b, bool a);

        /**
         * @brief Enable/disable per-channel writes to one draw buffer alone.
         *
         * The next setColorMask() then applies whatever it is given, as the buffers no longer
         * agree with what it last set.
         *
         * @param drawBuffer The draw buffer's index.
         * @param r          Enable writes to the red channel.
         * @param g          Enable writes to the green channel.
         * @param b          Enable writes to the blue channel.
         * @param a          Enable writes to the alpha channel.
         */
        void setColorMask(uint32_t drawBuffer, bool r, bool g, bool b, bool a);

        /**
         * @brief Read a fragment's alpha as the share of its samples it covers.
         *
         * Under multisampling a cutout's edge then blends rather than steps.
         *
         * @param enable True to enable, false to disable.
         */
        void setAlphaToCoverage(bool enable);

        /**
         * @brief Offset the depth of polygons rasterized as @p mode.
         *
         * Each fragment's depth moves by factor * its slope + units * the depth's resolution. One
         * offset holds for every mode; @p enable turns it on or off for @p mode alone.
         *
         * @param mode   GL_FILL, GL_LINE or GL_POINT.
         * @param enable True to offset that mode's polygons.
         * @param factor The slope's multiplier.
         * @param units  The constant, in the depth buffer's smallest steps.
         */
        void setPolygonOffset(GLenum mode, bool enable, float factor = 0.0f, float units = 0.0f);

        /**
         * @brief Clamp depth to the depth range rather than clip what lies past the near and far
         *        planes.
         *
         * @param enable True to clamp, false to clip.
         */
        void setDepthClamp(bool enable);

        /**
         * @brief Where clip space's origin and depth range lie (glClipControl).
         *
         * Not cached: set once, as the depth convention the whole application keeps.
         *
         * @param origin GL_LOWER_LEFT or GL_UPPER_LEFT.
         * @param depth  GL_NEGATIVE_ONE_TO_ONE or GL_ZERO_TO_ONE.
         */
        void setClipControl(GLenum origin, GLenum depth) const;

        /**
         * @brief The byte alignment every row of pixels is packed and unpacked at.
         *
         * Not cached: set once. An alignment of 1 keeps a row of any width and channel count tight.
         *
         * @param alignment 1, 2, 4 or 8.
         */
        void setPixelAlignment(int32_t alignment) const;

        /**
         * @brief Filter a cube map across its faces' seams.
         *
         * Not cached: set once.
         *
         * @param enable True to filter across them.
         */
        void setSeamlessCubes(bool enable) const;

        /**
         * @brief Set the polygon rendering mode (fill/line/point), for front and back faces alike: the
         *        core profile has one mode for both.
         * @param mode OpenGL polygon mode enum.
         */
        void setPolygonMode(GLenum mode);

        /**
         * @brief Reset every tracked piece of state to its default.
         *
         * Covers all of it - depth, culling, blending, color mask, scissor,
         * polygon mode and offset, alpha to coverage and depth clamp - so a
         * caller can rely on the name and not have to know which subset was
         * actually reset. The viewport and scissor *rectangles* are not tracked
         * and are left alone; see setViewport().
         */
        void setDefaultState();

        /**
         * @brief Largest 2D texture edge the driver accepts (GL_MAX_TEXTURE_SIZE).
         *
         * Queried once on first use and cached: a texture past it is one the driver refuses to
         * allocate.
         *
         * @return The limit in texels.
         */
        int32_t maxTextureSize() const;

        /**
         * @brief One of the driver's integer limits or state values (glGetIntegerv).
         *
         * Not cached: for the limits a host checks once, as it starts.
         *
         * @param name What is asked, e.g. GL_MAX_TEXTURE_IMAGE_UNITS.
         * @return Its value; 0 when the query failed.
         */
        int32_t integer(GLenum name) const;

        /**
         * @brief GL_VERSION string (e.g. "4.6.0 ..."), empty if unavailable.
         */
        std::string versionString() const;

        /**
         * @brief GL_RENDERER string (the GPU / device), empty if unavailable.
         */
        std::string rendererString() const;

    private:
        glm::vec4   m_clearColor{0.0f, 0.0f, 0.0f, 0.0f};  ///< Matches the GL default.
        RasterState m_state;

        mutable int32_t m_maxTextureSize = -1;  ///< Cached GL_MAX_TEXTURE_SIZE (-1 = not yet queried).
};

/**
 * @brief Order what earlier commands wrote before what later ones read (glMemoryBarrier).
 *
 * @param barriers GL_*_BARRIER_BIT, OR-ed: how the later commands read it.
 */
void memoryBarrier(GLbitfield barriers);

/// Wait until every command issued so far has finished (glFinish).
void finish();

} // namespace Vkm::OpenGL
