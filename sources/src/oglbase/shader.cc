/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "oglbase/shader.h"

#include <algorithm>
#include <iostream>

#include <boost/numeric/conversion/cast.hpp>

#include "oglbase/error.h"

namespace oglbase {

ShaderPtr
CompileShader(GLenum _type, ShaderSources_t &_sources)
{
	GLsizei const source_count = boost::numeric_cast<GLsizei>(_sources.size());
	ShaderPtr result{ glCreateShader(_type) };
	glShaderSource(result, source_count, _sources.data(), NULL);
	glCompileShader(result);
	if (GetShaderStatus<ShaderInfoFuncs, GL_COMPILE_STATUS>(result))
	{
#ifdef SR_GL_DEBUG_CONTEXT
		ForwardShaderLog<ShaderInfoFuncs>(result);
#else
		std::cout << GetShaderLog<ShaderInfoFuncs>(result) << std::endl;
#endif
		result.reset(0u);
	}
	return result;
}

ProgramPtr
LinkProgram(ShaderBinaries_t const &_binaries)
{
	ProgramPtr result{ glCreateProgram() };
	std::for_each(_binaries.cbegin(), _binaries.cend(), [&result](GLuint _shader) {
		glAttachShader(result, _shader);
	});
	glLinkProgram(result);
	if (GetShaderStatus<ProgramInfoFuncs, GL_LINK_STATUS>(result))
	{
#ifdef SR_GL_DEBUG_CONTEXT
		ForwardShaderLog<ProgramInfoFuncs>(result);
#else
		std::cout << GetShaderLog<ProgramInfoFuncs>(result) << std::endl;
#endif
		result.reset(0u);
	}
	return result;
}


} // namespace oglbase
