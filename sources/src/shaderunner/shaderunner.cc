/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "shaderunner/shaderunner.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <iterator>
#include <vector>

#include <boost/numeric/conversion/cast.hpp>
#include <GL/glew.h>


#define SR_GLSL_VERSION "#version 330 core\n"
#define SR_SL_ENTRY_POINT(entry_point) "#define SR_ENTRY_POINT " entry_point "\n"
#define SR_SL_DUMMY_FKERNEL "void imageMain(inout vec4 frag_color, vec2 frag_coord) { frag_color = vec4(1.0 - float(gl_PrimitiveID), 0.0, 1.0, 1.0); } \n"


using StdClock = std::chrono::high_resolution_clock;

namespace sr {

// =============================================================================

namespace glerror {
std::string const &GetGLErrorString(GLenum const _error);
bool PrintShaderError(std::ostream& _ostream, GLuint const _shader);
bool PrintProgramError(std::ostream& _ostream, GLuint const _program);
GLenum PrintError(std::ostream& _ostream);
bool ClearGLError();
} //namespace glerror

// =============================================================================

template <typename Deleter>
struct GLHandle
{
	explicit GLHandle(GLuint _handle) : handle(_handle) {}
	GLHandle(GLHandle const &) = delete;
	GLHandle(GLHandle &&_v) : handle(_v.handle) { _v.handle = 0u; }
	~GLHandle()	{ _delete(); }
	GLHandle &operator=(GLHandle const &_v) = delete;
	GLHandle &operator=(GLHandle &&_v){ reset(_v.handle); _v.handle = 0u; return *this;}
	operator GLuint() const { return handle; }
	operator bool() const { return handle != 0u; }
	void reset(GLuint _h){ _delete(); handle = _h; }
private:
	void _delete() { if (handle) Deleter{}(handle); }
	GLuint handle = 0u;
};

struct GLProgramDeleter
{
	void operator()(GLuint _program)
	{
		std::cout << "gl program deleted" << std::endl;
		glDeleteProgram(_program);
	}
};
struct GLShaderDeleter
{
	void operator()(GLuint _shader)
	{
		std::cout << "gl shader deleted" << std::endl;
		glDeleteShader(_shader);
	}
};

using GLProgramPtr = GLHandle<GLProgramDeleter>;
using GLShaderPtr = GLHandle<GLShaderDeleter>;

using ShaderSources_t = std::vector<char const *>;
using ShaderBinaries_t = std::vector<GLuint>;

GLShaderPtr CompileFragmentKernel(ShaderSources_t const &_kernel_sources);
GLShaderPtr CompileShader(GLenum _type, ShaderSources_t &_sources);
GLProgramPtr LinkProgram(ShaderBinaries_t const &_binaries);

// =============================================================================



struct RenderContext::Impl_
{
public:
	Impl_();
	~Impl_();

public:
	GLProgramPtr shader_program;
	GLShaderPtr cached_vshader;
	GLShaderPtr cached_fshader;
	GLuint dummy_vao;
};

RenderContext::Impl_::Impl_() :
	shader_program{ 0u },
	cached_vshader{ 0u },
	cached_fshader{ 0u },
	dummy_vao{ 0u }
{
	{
		static ShaderSources_t vertex_sources{
			SR_GLSL_VERSION,
			#include "shaders/fullscreen_tri.vert.h"
		};
		cached_vshader = CompileShader(GL_VERTEX_SHADER, vertex_sources);

		static ShaderSources_t const default_fkernel{
			SR_SL_DUMMY_FKERNEL
		};
		cached_fshader = CompileFragmentKernel(default_fkernel);

		assert(cached_vshader);
		assert(cached_fshader);
		ShaderBinaries_t const shader_binaries{
			cached_vshader,
			cached_fshader
		};
		shader_program = LinkProgram(shader_binaries);
		assert(shader_program);
	}

	{
		glGenVertexArrays(1, &dummy_vao);
	}
}

RenderContext::Impl_::~Impl_()
{
	glDeleteVertexArrays(1, &dummy_vao);
}


// =============================================================================

GLShaderPtr CompileFragmentKernel(ShaderSources_t const &_kernel_sources)
{
	static ShaderSources_t const kKernelPrefix{
		SR_GLSL_VERSION
	};
	static ShaderSources_t const kKernelSuffix{
		SR_SL_ENTRY_POINT("imageMain"),
		#include "shaders/entry_point.frag.h"
	};

	ShaderSources_t shader_sources{};
	shader_sources.reserve(kKernelPrefix.size() + _kernel_sources.size() + kKernelSuffix.size());
	std::copy(kKernelPrefix.cbegin(), kKernelPrefix.cend(), std::back_inserter(shader_sources));
	std::copy(_kernel_sources.cbegin(), _kernel_sources.cend(), std::back_inserter(shader_sources));
	std::copy(kKernelSuffix.cbegin(), kKernelSuffix.cend(), std::back_inserter(shader_sources));

	return CompileShader(GL_FRAGMENT_SHADER, shader_sources);
}

GLShaderPtr CompileShader(GLenum _type, ShaderSources_t &_sources)
{
	GLsizei const source_count = boost::numeric_cast<GLsizei>(_sources.size());
	GLShaderPtr result{ glCreateShader(_type) };
	glShaderSource(result, source_count, _sources.data(), NULL);
	glCompileShader(result);
	if (glerror::PrintShaderError(std::cout, result))
	{
		result.reset(0u);
	}
	return result;
}

GLProgramPtr LinkProgram(ShaderBinaries_t const &_binaries)
{
	GLProgramPtr result{ glCreateProgram() };
	std::for_each(_binaries.cbegin(), _binaries.cend(), [&result](GLuint _shader) {
		glAttachShader(result, _shader);
	});
	glLinkProgram(result);
	if (glerror::PrintProgramError(std::cout, result))
	{
		result.reset(0u);
	}
	return result;
}


// =============================================================================

RenderContext::RenderContext() :
	impl_(new RenderContext::Impl_())
{}

RenderContext::~RenderContext()
{ delete impl_; }

bool
RenderContext::RenderFrame() const
{
	static StdClock::time_point last_render_time = StdClock::now();
	StdClock::duration delta_time = StdClock::now() - last_render_time;
	std::cout << std::chrono::duration_cast<std::chrono::duration<float>>(delta_time).count() << std::endl;

	bool start_over = true;

	assert(!glerror::ClearGLError());

	static GLfloat const clear_color[]{ 0.5f, 0.5f, 0.5f, 1.f };
	glClearBufferfv(GL_COLOR, 0, clear_color);

	glUseProgram(impl_->shader_program);
	glBindVertexArray(impl_->dummy_vao);

	glDrawArrays(GL_TRIANGLES, 0, 3);
	start_over = start_over && (glerror::PrintError(std::cout) == GL_NO_ERROR);

	glBindVertexArray(0u);
	glUseProgram(0u);

#ifdef SR_SINGLE_BUFFERING
	glFlush();
#endif

	last_render_time += delta_time;
	return start_over;
}


bool
RenderContext::LoadFragmentKernel(char const *_sources)
{
	ShaderSources_t const kernel_sources{ _sources };
	GLShaderPtr compiled_fshader = CompileFragmentKernel(kernel_sources);
	if (!compiled_fshader) return false;

	assert(impl_->cached_vshader);
	ShaderBinaries_t const shader_binaries{
		impl_->cached_vshader,
		compiled_fshader
	};
	GLProgramPtr linked_program = LinkProgram(shader_binaries);
	if (!linked_program) return false;

	impl_->cached_fshader = std::move(compiled_fshader);
	impl_->shader_program = std::move(linked_program);
	return true;
}


// =============================================================================

namespace glerror {

std::string const &GetGLErrorString(GLenum const _error)
{
	static std::string const no_error{"None"};
	static std::string const invalid_enum{"Invalid enum"};
	static std::string const invalid_value{"Invalid value"};
	static std::string const invalid_op{"Invalid operation"};
	static std::string const invalid_fb_op{"Invalid framebuffer operation"};
	static std::string const out_of_memory{"Out of memory"};
	static std::string const stack_underflow{"Stack underflow"};
	static std::string const stack_overflow{"Stack overflow"};
	static std::string const unknown{"Unknown error"};

	switch(_error)
	{
	case GL_NO_ERROR:
		return no_error;
		break;
	case GL_INVALID_ENUM:
		return invalid_enum;
		break;
	case GL_INVALID_VALUE:
		return invalid_value;
		break;
	case GL_INVALID_OPERATION:
		return invalid_op;
		break;
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		return invalid_fb_op;
		break;
	case GL_OUT_OF_MEMORY:
		return out_of_memory;
		break;
	case GL_STACK_UNDERFLOW:
		return stack_underflow;
		break;
	case GL_STACK_OVERFLOW:
		return stack_overflow;
		break;
	default:
		return unknown;
		break;
	}
}

bool PrintShaderError(std::ostream& _ostream, GLuint const _shader)
{
	bool result = true;
	GLint success = GL_FALSE;
	glGetShaderiv(_shader, GL_COMPILE_STATUS, &success);
	result = success != GL_TRUE;
	if (result)
	{
		_ostream << "Shader compile error : " << std::endl;
		{ // TODO: make sure char[] allocation is safe enough
			GLsizei log_size = 0;
			glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &log_size);
			assert(log_size != 0);
			char* const info_log = new char[log_size];
			glGetShaderInfoLog(_shader, log_size, NULL, info_log);
			_ostream << info_log << std::endl;
			delete[] info_log;
		}
	}
	return result;
}

bool PrintProgramError(std::ostream& _ostream, GLuint const _program)
{
	bool result = true;
	GLint success = GL_FALSE;
	glGetProgramiv(_program, GL_LINK_STATUS, &success);
	result = success != GL_TRUE;
	if (result)
	{
		_ostream << "Shader link error : " << std::endl;
		{
			GLsizei log_size = 0;
			glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &log_size);
			assert(log_size != 0);
			char* const info_log = new char[log_size];
			glGetShaderInfoLog(_program, log_size, NULL, info_log);
			_ostream << info_log << std::endl;
			delete[] info_log;
		}
	}
	return result;
}

GLenum PrintError(std::ostream& _ostream)
{
	GLenum const error_code = glGetError();
	if (error_code != GL_NO_ERROR)
		_ostream << "OpenGL error : " << GetGLErrorString(error_code).c_str() << std::endl;
	return error_code;
}

bool ClearGLError()
{
	bool const result = (glGetError() != GL_NO_ERROR);
	while(glGetError() != GL_NO_ERROR);
	return result;
}

} //namespace glerror

} //namespace sr
