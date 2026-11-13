#pragma once

#include <cstdint>
#include <string>

namespace Vkm::OpenGL {

class ShaderBase;

// The live shaders, for hot reload, counting and checking: every ShaderBase registers itself, and
// the list is the context thread's alone. Why reload is coarse: docs/how-it-works.md, "Shaders".

/**
 * @brief Add @p shader to the live set. Called by ShaderBase's constructor.
 */
void registerShader(ShaderBase* shader);

/**
 * @brief Remove @p shader from the live set. Called by ShaderBase's destructor.
 */
void unregisterShader(ShaderBase* shader);

/**
 * @brief Give @p to the place @p from held in the live set.
 *
 * What a move takes is the program, and the place goes with it: @p to is live exactly when
 * @p from was, in its order, and @p from is live no more. So a moved-from shader is never walked.
 *
 * @param from The shader moved from.
 * @param to   The shader moved into.
 */
void handOverShader(ShaderBase* from, ShaderBase* to);

/**
 * @brief How many shaders are live, how many the driver is still compiling, and how many are unchecked.
 */
struct ShaderCount {
    uint32_t live      = 0;
    uint32_t compiling = 0;  ///< Not ShaderBase::ready().
    uint32_t unchecked = 0;  ///< Not ShaderBase::checked(); every one still compiling is too.
};

/**
 * @brief Count the live shaders, asking each whether it is ready without waiting for it.
 *
 * For a host showing progress while ShaderBase::compileInParallel() builds them.
 *
 * @return The counts.
 */
ShaderCount countShaders();

/**
 * @brief Check every live shader's compile and link, waiting for any still building.
 *
 * With ShaderBase::compileInParallel() on, a program is otherwise checked at its first use, which
 * can come long after startup. A host calls this once countShaders() finds none compiling, so a
 * failure surfaces then. A shader already checked costs nothing.
 *
 * @throws std::runtime_error At the first shader that failed to compile or link, having logged it.
 */
void checkShaders();

/**
 * @brief Check one live shader the driver is done with that nothing has checked yet, as
 *        checkShaders() checks them all.
 *
 * On some drivers the asking is where a program is really built, holding every GL call while it
 * is: a host checking a few a frame keeps each frame short where checking all at once is one long
 * frozen frame.
 *
 * @return Whether one was checked.
 * @throws std::runtime_error When it failed to compile or link, having logged it.
 */
bool checkNextShader();

/**
 * @brief Recompile every live shader if anything under @p directory changed.
 *
 * Walks every file under @p directory for the newest write time, on the calling thread. A shader
 * whose source no longer compiles keeps its previous program and logs the error.
 *
 * @param directory Root to watch, e.g. "shaders".
 * @return Number of shaders recompiled; 0 when nothing changed.
 */
uint32_t reloadChangedShaders(const std::string& directory);

} // namespace Vkm::OpenGL
