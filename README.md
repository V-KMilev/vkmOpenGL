# vkmOpenGL

OpenGL objects and state for a renderer that makes no `gl*` call of its own.
Every GL object - program, texture, buffer, vertex array, framebuffer, fence - is
a class here, edited by name through direct state access; pipeline state goes
through `Context`'s cache; every call is under `VKM_OPENGL_CHECK`.

## Requirements

CMake 3.25 or above, a C++17 compiler and a driver of the OpenGL that
`vkm_opengl/core/version.h` states, `MAJOR_VERSION`.`MINOR_VERSION` core.

## Use

Add it as a submodule and `add_subdirectory` it. It links OpenGL, glew, glm and
`vkm_log`, and vendors only glew: the consuming project adds the `glm` and
`vkm_log` targets first, as it uses them itself, and the build stops with the
reason when one is missing. It needs no window library: the consumer makes a
context current, from a window or from EGL, and calls `Vkm::OpenGL::load()`
before the first call; it answers whether OpenGL loaded and why not.

```
git submodule add https://github.com/V-KMilev/vkmOpenGL.git modules/vkmOpenGL
```

The library is `vkm_opengl`, shared. Headers are included by the library's folder:
`#include "vkm_opengl/texture/texture.h"`; everything is in `Vkm::OpenGL`.

| Option                   | Default | What                                                                                                            |
|--------------------------|---------|-----------------------------------------------------------------------------------------------------------------|
| `VKM_OPENGL_ASSERTS`     | `OFF`   | The library's `VKM_ASSERT`s run whatever the build type (`-UNDEBUG` on its own sources)                         |
| `VKM_OPENGL_CHECKS`      | `OFF`   | `VKM_OPENGL_CHECK` reads GL's error queue after every call, and stops the program on an error: a stall per call |
| `VKM_OPENGL_BUILD_TESTS` | `OFF`   | Builds `vkm_opengl_tests`: each test a CTest of its own, labeled `vkm_opengl`                                   |

Neither debug feature follows `NDEBUG`: a consumer that sets no flag gets a library with neither.

## What it holds

| Folder    | What                                                                                   |
|-----------|----------------------------------------------------------------------------------------|
| `core`    | `Context` (state cache, limits, barriers), `Object`, `VKM_OPENGL_CHECK`, debug output, the version it needs |
| `buffer`  | `Buffer` and its vertex, index, uniform and storage kinds; `StreamRing`, `Fence`, `ReadbackBuffer` |
| `vertex`  | `VertexArray` (attribute formats, bindings, draws), `VertexBufferLayout`                |
| `texture` | `Texture2D`, `Texture2DArray`, `Texture3D`, `TextureCube`, `TextureCubeArray`, `MipChainTexture`, `Sampler`; the cube's faces in GL's order (`CUBE_FACES`, `cubeFaceView`) |
| `target`  | `FrameBuffer`, `RenderBuffer`                                                           |
| `shader`  | `Shader`, `ComputeShader`, the source preprocessor, parallel compiling, hot reload      |

Each folder is under `src/vkm_opengl/`.

## Tests

`tests/` holds the library's own suite, built with `VKM_OPENGL_BUILD_TESTS`: each test is
`TEST("what works")` in `tests/<topic>.cpp`, a CTest named `vkm_opengl: <what works>` and run in a
process of its own, so `ctest -L vkm_opengl` runs them in the consumer's tree, with `-C <config>` under
a multi-config generator. It covers what runs without a context - vertex layouts, the preprocessor, the
cube's faces, the indirect draw's record, `load()` with none current, a precondition stopping - so it
runs on a machine with no GPU. What needs a context is in `tests/gpu/`, a binary of its own built where
EGL is: each test makes a context through EGL's device platform, with no display, is labeled `gpu` as
well, and is skipped, saying why, on a machine with no device (or fails where `VKM_GPU_REQUIRED=1`).
Beside them, CTests of the library as a consumer builds it: its includes, its CMake, and its debug
flags.
