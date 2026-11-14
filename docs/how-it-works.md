# How vkmOpenGL works

What the headers state as contracts is not repeated here; this page holds the reasons behind them.

## Objects

Every GL object is a class that owns its names: made in its constructor, or in its `create()` for a
texture sized later, and anew where a resize, a reload or a stream's growth needs one, through direct
state access (`glCreate*`) - a texture view through `glGenTextures` - deleted in its destructor, never
copied; one that moves hands its name over, and the source deletes nothing. `Object` holds the name,
the target it binds to and the identifier its label goes by; each class declares the binding call that
suits it, as a program, a vertex array and a sampler bind in no common way. A `Fence` alone is made in
use: each `place()` makes its sync object (`glFenceSync`), and one that has passed lets it go.

The library needs the OpenGL that `core/version.h` states, and nothing older: `load()` reads those two
numbers, refuses an older context and says why, and the preprocessor writes the matching `#version`.
After `load()`, every call is under `VKM_OPENGL_CHECK`.

## What it remembers

A bind of what is bound already costs the driver as much as a new one, so the library remembers what
it bound, and skips a bind that changes nothing:

- **`Context`** caches the raster state it sets (depth, blend, cull, masks, polygon mode and offset).
  The viewport and the scissor rectangle pass straight through, as other code sets them too.
- **Texture units:** every texture binds through `bindTextureUnit` and deletes through
  `deleteTextures`, which forgets it on every unit it held.
- **Buffers:** every buffer deletes through `deleteBuffer`, which forgets it wherever a binding to it
  is remembered (`bindIndirect`, a vertex array's feeds) and counts the deletion: the driver hands a
  deleted name out again, and a binding remembered by that name would read as made.
- **Vertex arrays:** the bound array and each array's feeds are remembered, as the driver revalidates
  a draw after either changes.

This holds for one context, used from one thread, while vkmOpenGL alone binds (or what binds behind
it puts the last binding back).

## Shaders

A stage is read from disk with the prelude before it: `#version`, the constants `setShaderPrelude`
was given, then the program's defines. `#include` inlines a file once per stage, so a later include
of it is skipped and a cycle ends. With `ShaderBase::compileInParallel`, the driver compiles on two
threads of its own (more corrupted one driver's heap when a start found some programs in its disk
cache and compiled others anew); a program is checked at its first use, or by `checkNextShader`, which
checks one finished program a call: on some drivers the asking is where a program is really built,
holding every GL call while it is, so a few a frame keep each frame short.

**Hot reload.** Every `ShaderBase` adds itself to one live list when made, hands its place over when
moved and leaves it when destroyed, so the list is right without a caller opting in.
`reloadChangedShaders` takes the newest write time anywhere under the folder it watches and, when it
moves, rebuilds every live program: per-file dependencies would be finer, but most programs include
the same shared headers, so a precise tracker would rebuild most of them anyway, and the coarse one
cannot miss a dependency. A program that no longer builds keeps the one it had, and the error is
logged. The list is the context thread's alone.

## Per-frame data

`StreamRing` is one persistently mapped buffer cut into a few regions taken in turn: a region opens
on the first write after a frame's end, waiting until the GPU has finished the frame that last used
it, and the frame's end fences it. So the CPU never writes what the GPU still reads, and the driver
never copies a buffer in flight to keep a draw's old contents. `DeviceStream` copies what a ring holds
once a frame into GPU memory, for data many passes read: a ring may sit on the CPU's side of the bus.

## Debug features

Two flags, each off unless the consumer turns it on: `VKM_OPENGL_ASSERTS` keeps the library's
`VKM_ASSERT`s in whatever the build type, where without it they follow `NDEBUG`, and `VKM_OPENGL_CHECKS`
reads GL's error queue after every call, a stall per call on a driver that runs on a thread of its own.
Without the checks, the driver's own debug output reports errors as they happen, once the consumer
turns it on with `enableGLDebugLogging`.
