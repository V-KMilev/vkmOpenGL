#include "vkm_opengl/core/context.h"

#include "vkm_opengl/core/error_handle.h"

namespace Vkm::OpenGL {

void Context::setClearColor(const glm::vec4& color) {
    m_clearColor = color;
}

void Context::clear(bool color, bool depth, float depthValue) const {
    const glm::vec4& rgba = m_clearColor;

    // A color clear applies the stored clear color first - setClearColor
    // alone never touches GL, so set + clear always clears with the caller's
    // color instead of whatever glClearColor state was left behind.
    if (color) VKM_OPENGL_CHECK(glClearColor(rgba[0], rgba[1], rgba[2], rgba[3]));
    if (depth) VKM_OPENGL_CHECK(glClearDepthf(depthValue));

    GLbitfield mask = 0;
    if (color) mask |= GL_COLOR_BUFFER_BIT;
    if (depth) mask |= GL_DEPTH_BUFFER_BIT;

    VKM_OPENGL_CHECK(glClear(mask));
}

void Context::setViewport(int32_t x, int32_t y, int32_t width, int32_t height) const {
    VKM_OPENGL_CHECK(glViewport(x, y, width, height));
}

void Context::enableScissor(bool enable) {
    if (m_state.scissorEnabled == enable) return;
    m_state.scissorEnabled = enable;

    if (enable) { VKM_OPENGL_CHECK(glEnable(GL_SCISSOR_TEST)); }
    else        { VKM_OPENGL_CHECK(glDisable(GL_SCISSOR_TEST)); }
}

void Context::setScissor(int32_t x, int32_t y, int32_t width, int32_t height) const {
    VKM_OPENGL_CHECK(glScissor(x, y, width, height));
}


void Context::setDepthTest(bool enable) {
    if (m_state.depth.enabled == enable) return;
    m_state.depth.enabled = enable;

    if (enable) { VKM_OPENGL_CHECK(glEnable(GL_DEPTH_TEST)); }
    else        { VKM_OPENGL_CHECK(glDisable(GL_DEPTH_TEST)); }
}

void Context::setDepthFunc(GLenum func) {
    if (m_state.depth.func == func) return;
    m_state.depth.func = func;
    VKM_OPENGL_CHECK(glDepthFunc(func));
}

void Context::setDepthWrite(bool enable) {
    if (m_state.depth.writeEnabled == enable) return;
    m_state.depth.writeEnabled = enable;
    VKM_OPENGL_CHECK(glDepthMask(enable ? GL_TRUE : GL_FALSE));
}


void Context::setFaceCulling(bool enable) {
    if (m_state.cull.enabled == enable) return;
    m_state.cull.enabled = enable;

    if (enable) { VKM_OPENGL_CHECK(glEnable(GL_CULL_FACE)); }
    else        { VKM_OPENGL_CHECK(glDisable(GL_CULL_FACE)); }
}

void Context::setCullFace(GLenum face) {
    if (m_state.cull.cullFace == face) return;
    m_state.cull.cullFace = face;
    VKM_OPENGL_CHECK(glCullFace(face));
}

void Context::setFrontFace(GLenum winding) {
    if (m_state.cull.frontFace == winding) return;
    m_state.cull.frontFace = winding;
    VKM_OPENGL_CHECK(glFrontFace(winding));
}


void Context::setBlending(bool enable) {
    if (m_state.blend.enabled == enable) return;
    m_state.blend.enabled = enable;

    if (enable) { VKM_OPENGL_CHECK(glEnable(GL_BLEND)); }
    else        { VKM_OPENGL_CHECK(glDisable(GL_BLEND)); }
}

void Context::setBlendFunc(GLenum src, GLenum dst) {
    if (m_state.blend.srcFactor == src && m_state.blend.dstFactor == dst) return;
    m_state.blend.srcFactor = src;
    m_state.blend.dstFactor = dst;
    VKM_OPENGL_CHECK(glBlendFunc(src, dst));
}

void Context::setColorMask(bool r, bool g, bool b, bool a) {
    auto& c = m_state.colorMask;
    if (!m_state.colorMaskSplit && c.r == r && c.g == g && c.b == b && c.a == a) return;
    c.r = r; c.g = g; c.b = b; c.a = a;
    m_state.colorMaskSplit = false;
    VKM_OPENGL_CHECK(glColorMask(
        r ? GL_TRUE : GL_FALSE,
        g ? GL_TRUE : GL_FALSE,
        b ? GL_TRUE : GL_FALSE,
        a ? GL_TRUE : GL_FALSE
    ));
}

void Context::setColorMask(uint32_t drawBuffer, bool r, bool g, bool b, bool a) {
    m_state.colorMaskSplit = true;
    VKM_OPENGL_CHECK(glColorMaski(
        drawBuffer,
        r ? GL_TRUE : GL_FALSE,
        g ? GL_TRUE : GL_FALSE,
        b ? GL_TRUE : GL_FALSE,
        a ? GL_TRUE : GL_FALSE
    ));
}

namespace {

// Turn @p capability on or off when @p cached says otherwise, and remember it.
void toggle(bool& cached, GLenum capability, bool enable) {
    if (cached == enable) return;
    cached = enable;
    if (enable) { VKM_OPENGL_CHECK(glEnable(capability)); }
    else        { VKM_OPENGL_CHECK(glDisable(capability)); }
}

} // namespace

void Context::setAlphaToCoverage(bool enable) {
    toggle(m_state.alphaToCoverage, GL_SAMPLE_ALPHA_TO_COVERAGE, enable);
}

void Context::setPolygonOffset(GLenum mode, bool enable, float factor, float units) {
    if (enable && (m_state.offsetFactor != factor || m_state.offsetUnits != units)) {
        m_state.offsetFactor = factor;
        m_state.offsetUnits  = units;
        VKM_OPENGL_CHECK(glPolygonOffset(factor, units));
    }
    switch (mode) {
        case GL_FILL:  toggle(m_state.offsetFill, GL_POLYGON_OFFSET_FILL, enable);   break;
        case GL_LINE:  toggle(m_state.offsetLine, GL_POLYGON_OFFSET_LINE, enable);   break;
        case GL_POINT: toggle(m_state.offsetPoint, GL_POLYGON_OFFSET_POINT, enable); break;
        default: break;
    }
}

void Context::setDepthClamp(bool enable) {
    toggle(m_state.depthClamp, GL_DEPTH_CLAMP, enable);
}

void Context::setClipControl(GLenum origin, GLenum depth) const {
    VKM_OPENGL_CHECK(glClipControl(origin, depth));
}

void Context::setPixelAlignment(int32_t alignment) const {
    VKM_OPENGL_CHECK(glPixelStorei(GL_PACK_ALIGNMENT, alignment));
    VKM_OPENGL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, alignment));
}

void Context::setSeamlessCubes(bool enable) const {
    if (enable) { VKM_OPENGL_CHECK(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS)); }
    else        { VKM_OPENGL_CHECK(glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS)); }
}

void Context::setPolygonMode(GLenum mode) {
    if (m_state.polygonMode == mode) return;
    m_state.polygonMode = mode;
    VKM_OPENGL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, mode));
}

void Context::setDefaultState() {
    setDepthTest(true);
    setDepthFunc(GL_LEQUAL);
    setDepthWrite(true);

    setFaceCulling(true);
    setCullFace(GL_BACK);
    setFrontFace(GL_CCW);

    setBlending(false);
    setBlendFunc(GL_ONE, GL_ZERO);

    setColorMask(true, true, true, true);
    enableScissor(false);

    setPolygonMode(GL_FILL);

    setAlphaToCoverage(false);
    setPolygonOffset(GL_FILL, false);
    setPolygonOffset(GL_LINE, false);
    setPolygonOffset(GL_POINT, false);
    setDepthClamp(false);
}

int32_t Context::maxTextureSize() const {
    if (m_maxTextureSize < 0) {
        GLint size = 0;
        VKM_OPENGL_CHECK(glGetIntegerv(GL_MAX_TEXTURE_SIZE, &size));
        m_maxTextureSize = size;
    }
    return m_maxTextureSize;
}

int32_t Context::integer(GLenum name) const {
    GLint value = 0;
    VKM_OPENGL_CHECK(glGetIntegerv(name, &value));
    return value;
}

void memoryBarrier(GLbitfield barriers) {
    VKM_OPENGL_CHECK(glMemoryBarrier(barriers));
}

void finish() {
    VKM_OPENGL_CHECK(glFinish());
}

std::string Context::versionString() const {
    const GLubyte* version = nullptr;
    VKM_OPENGL_CHECK(version = glGetString(GL_VERSION));
    return version ? reinterpret_cast<const char*>(version) : std::string();
}

std::string Context::rendererString() const {
    const GLubyte* renderer = nullptr;
    VKM_OPENGL_CHECK(renderer = glGetString(GL_RENDERER));
    return renderer ? reinterpret_cast<const char*>(renderer) : std::string();
}

} // namespace Vkm::OpenGL
