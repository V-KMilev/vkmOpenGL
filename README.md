# vkmOpenGL

OpenGL objects and state for a renderer that makes no `gl*` call of its own.
Every GL object - program, texture, buffer, vertex array, framebuffer, fence - is
a class here, edited by name through direct state access; pipeline state goes
through `Context`'s cache; every call after `load()` is under `VKM_OPENGL_CHECK`.

## Conventions

- **One context, from one thread.** Every call is the context's thread's, and nothing is called before
  `load()` answers that the current context is the version `core/version.h` states.
- **Every GL object is a class that owns its name:** made in its constructor or its `create()`, and
  anew where a resize, a reload or a stream's growth needs one, so a caller asks for it rather than
  keeping it; deleted in its destructor, never copied; one that moves takes its name with it, and the
  object it leaves deletes nothing. A `Fence` alone is made in use: each `place()` makes its sync
  object, and one that has passed lets it go.
- **A bind that changes nothing goes no further.** The library remembers what it bound - pipeline
  state in `Context`, texture units, the vertex array bound and each array's feeds, the draw-indirect
  buffer - while it alone binds, or what binds behind it puts the last binding back.
- **A program is a folder** of stages - `vertex.shader`, `fragment.shader` and an optional
  `geometry.shader`, or a `compute.shader` - each read with the prelude and the program's defines
  before it; `#include` inlines a file once.

## A draw

```cpp
using namespace Vkm::OpenGL;

const Loaded loaded = load();             // a context is current, from a window or from EGL
if (!loaded.ok) {
    std::fprintf(stderr, "%s\n", loaded.reason.c_str());
    return;
}

Shader sky("shaders/sky", {"STARS"});     // STARS is defined in each of the folder's stages
Texture2DParams params;
params.width  = 256;
params.height = 256;
Texture2D noise("noise", params);
VertexArray screen;

sky.bind();
noise.bindSlot(0);
screen.bind();
screen.drawArrays(GL_TRIANGLES, 0, 3);    // the vertex stage makes the triangle from gl_VertexID
```

`docs/how-it-works.md` says what the library remembers between calls, and why.

## Requirements

CMake 3.25 or above, a C++17 compiler and a driver of the OpenGL that
`vkm_opengl/core/version.h` states, `MAJOR_VERSION`.`MINOR_VERSION` core.

## Use

Add it as a submodule and `add_subdirectory` it. It links OpenGL, glew, glm and
`vkm_log`, and vendors only glew: the consuming project adds the `glm` and
`vkm_log` targets first, as it uses them itself, and the build stops with the
reason when one is missing. glew's repository holds none of the sources it
generates, so the first configure downloads glew's 2.3.0 release into
`modules/glew` (`modules/CMakeLists.txt`), and a consumer that has a `glew`
target already is linked to that instead. It needs no window library: the consumer makes a
context current, from a window or from EGL, and calls `Vkm::OpenGL::load()`
before the first call; it answers whether OpenGL loaded and why not.

```
git submodule add https://github.com/V-KMilev/vkmOpenGL.git modules/vkmOpenGL
```

The library is `vkm_opengl`, shared. Headers are included by the library's folder:
`#include "vkm_opengl/texture/texture.h"`; everything is in `Vkm::OpenGL`.

| Option                   | Default | What                                                                                                                                                                                 |
|--------------------------|---------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `VKM_OPENGL_ASSERTS`     | `OFF`   | The library's `VKM_ASSERT`s run whatever the build type (`-UNDEBUG` on its own sources)                                                                                              |
| `VKM_OPENGL_CHECKS`      | `OFF`   | `VKM_OPENGL_CHECK` reads GL's error queue after every call, and stops the program on an error: a stall per call. A generator expression decides per configuration: `$<CONFIG:Debug>` |
| `VKM_OPENGL_BUILD_TESTS` | `OFF`   | Builds `vkm_opengl_tests`: each test a CTest of its own, labeled `vkm_opengl`                                                                                                        |

A consumer that sets neither flag gets no checks, and the asserts only where its build type does not
define `NDEBUG`.

## What it holds

| Folder    | What                                                                                                                                                                       |
|-----------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `core`    | `load()`, `Context` (state cache, limits, barriers), `Object`, `VKM_OPENGL_CHECK`, debug output, the version it needs                                                      |
| `buffer`  | `Buffer` and its vertex, uniform and storage kinds; `StreamRing`, `DeviceStream`, `Fence`, `ReadbackBuffer`                                                                |
| `vertex`  | `VertexArray` (attribute formats, bindings, draws)                                                                                                                         |
| `texture` | `Texture2D`, `Texture2DArray`, `Texture3D`, `TextureCube`, `TextureCubeArray`, `MipChainTexture`, `Sampler`; the cube's faces in GL's order (`CUBE_FACES`, `cubeFaceView`) |
| `target`  | `FrameBuffer`, `RenderBuffer`                                                                                                                                              |
| `shader`  | `Shader`, `ComputeShader`, the source preprocessor, parallel compiling, hot reload                                                                                         |

Each folder is under `src/vkm_opengl/`.

## Tests

`tests/` holds the library's own suite, built with `VKM_OPENGL_BUILD_TESTS`: each test is
`TEST("what works")` in `tests/<topic>.cpp`, a CTest named `vkm_opengl: <what works>` and run in a
process of its own, so `ctest -L vkm_opengl` runs them in the consumer's tree, with `-C <config>` under
a multi-config generator. It covers what runs without a context - the preprocessor, the cube's
faces, the indirect draw's record, `load()` with none current, a precondition stopping - so it
runs on a machine with no GPU. What needs a context is in `tests/gpu/`, a binary of its own built where
EGL is: each test makes a context through EGL's device platform, with no display, is labeled `gpu` as
well, and is skipped, saying why, on a machine with no device (or fails where `VKM_GPU_REQUIRED=1`).
Beside them, CTests of the library as a consumer builds it: its includes, its CMake, and its debug
flags.

## Version

2.0.0, set in `CMakeLists.txt`'s `project()`. Each release is tagged `vX.Y.Z` on `master` and listed
in [CHANGELOG.md](CHANGELOG.md); work lands on `vkm/dev` first.

## In vkmEngine

The engine pins it at `modules/vkmOpenGL`, and its OpenGL backend links it. A change is a commit here,
then an engine commit that moves the pin.

## License

vkmOpenGL is source-available under the [vkmEngine License](LICENSE), the engine's own, which holds
for each vkm library used alone. [CONTRIBUTING.md](CONTRIBUTING.md) says how a change is shared.
