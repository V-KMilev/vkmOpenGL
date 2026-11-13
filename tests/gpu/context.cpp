#include "gpu/context.h"

#include <cstdlib>
#include <cstring>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/glew.h>

#include "vkm_opengl/core/load.h"
#include "vkm_opengl/core/version.h"

#include "harness/check.h"
#include "harness/scratch.h"

namespace Vkm::Test {

namespace {

/// Make a context current on the first EGL device that gives one; empty when one did, else why not.
std::string makeContext() {
    const auto queryDevices = reinterpret_cast<PFNEGLQUERYDEVICESEXTPROC>(
        eglGetProcAddress("eglQueryDevicesEXT")
    );
    const auto platformDisplay = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
        eglGetProcAddress("eglGetPlatformDisplayEXT")
    );
    if (!queryDevices || !platformDisplay) return "EGL_EXT_platform_device is not available";

    // Every device, not the first: a software rasterizer listed beside the GPU may give no core context.
    EGLDeviceEXT devices[8];
    EGLint count = 0;
    if (!queryDevices(8, devices, &count) || count <= 0) return "EGL reports no devices";

    std::string why = "no EGL device gives an OpenGL " + std::to_string(OpenGL::MAJOR_VERSION) + "."
        + std::to_string(OpenGL::MINOR_VERSION) + " core context";
    for (EGLint i = 0; i < count; ++i) {
        const EGLDisplay display = platformDisplay(EGL_PLATFORM_DEVICE_EXT, devices[i], nullptr);
        EGLint major = 0;
        EGLint minor = 0;
        if (display == EGL_NO_DISPLAY || !eglInitialize(display, &major, &minor)) continue;
        if (!eglBindAPI(EGL_OPENGL_API)) {
            eglTerminate(display);
            continue;
        }
        const EGLint attributes[] = {
            EGL_CONTEXT_MAJOR_VERSION,       OpenGL::MAJOR_VERSION,
            EGL_CONTEXT_MINOR_VERSION,       OpenGL::MINOR_VERSION,
            EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
            EGL_NONE,
        };
        const EGLContext context = eglCreateContext(display, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, attributes);
        if (context == EGL_NO_CONTEXT) {
            eglTerminate(display);
            continue;
        }
        if (!eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context)) {
            eglDestroyContext(display, context);
            eglTerminate(display);
            continue;
        }
        const OpenGL::Loaded loaded = OpenGL::load();
        if (loaded.ok) return {};
        why = loaded.reason;
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(display, context);
        eglTerminate(display);
    }
    return why;
}

} // namespace

void missing(const std::string& why) {
    const char* required = std::getenv("VKM_GPU_REQUIRED");
    if (!required || std::strcmp(required, "1") != 0) skip(why);
    fail(__FILE__, __LINE__, "VKM_GPU_REQUIRED=1, and " + why);
    throw Stop{};
}

void requireContext() {
    // The process's one context, kept until it ends: each test runs in a process of its own.
    static const std::string s_missing = makeContext();
    if (!s_missing.empty()) missing(s_missing);
}

void dropContext() {
    eglMakeCurrent(eglGetCurrentDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void requireDefaultFramebuffer(int width, int height) {
    requireContext();
    const EGLDisplay display = eglGetCurrentDisplay();
    const EGLint config[] = {
        EGL_SURFACE_TYPE,    EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_NONE,
    };
    EGLConfig chosen = nullptr;
    EGLint    count  = 0;
    if (!eglChooseConfig(display, config, &chosen, 1, &count) || count == 0) {
        missing("the EGL device has no RGBA8 pbuffer config");
    }
    const EGLint size[] = {EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE};
    const EGLSurface surface = eglCreatePbufferSurface(display, chosen, size);
    if (surface == EGL_NO_SURFACE) missing("the EGL device makes no pbuffer");
    if (!eglMakeCurrent(display, surface, surface, eglGetCurrentContext())) {
        missing("the context cannot be made current with a pbuffer");
    }
    // First made current with no surface, the context draws to and reads from no buffer until told.
    glDrawBuffer(GL_BACK);
    glReadBuffer(GL_BACK);
}

} // namespace Vkm::Test
