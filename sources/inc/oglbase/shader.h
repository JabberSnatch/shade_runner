/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma once
#ifndef __YS_OGL_SHADER_HPP__
#define __YS_OGL_SHADER_HPP__

#include <vector>
#include <string>

#include <GL/glew.h>

#include "oglbase/handle.h"

namespace oglbase {


using ShaderSources_t = std::vector<char const *>;
using ShaderBinaries_t = std::vector<GLuint>;

ShaderPtr CompileShader(GLenum _type, ShaderSources_t const&_sources, std::string *o_log = nullptr);

ProgramPtr LinkProgram(ShaderBinaries_t const &_binaries);

} // namespace oglbase

#endif // __YS_OGL_SHADER_HPP__

