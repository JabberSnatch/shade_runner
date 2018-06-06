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
#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <iterator>
#include <vector>

#include <boost/numeric/conversion/cast.hpp>
#include <GL/glew.h>

#include "utility/file.h"


#define SR_GLSL_VERSION "#version 330 core\n"
#define SR_SL_ENTRY_POINT(entry_point) "#define SR_ENTRY_POINT " entry_point "\n"
#define SR_SL_DUMMY_FKERNEL "void imageMain(inout vec4 frag_color, vec2 frag_coord) { frag_color = vec4(1.0 - float(gl_PrimitiveID), 0.0, 1.0, 1.0); } \n"
#define SR_SL_TIME_UNIFORM "iTime"
#define SR_SL_RESOLUTION_UNIFORM "iResolution"

using StdClock = std::chrono::high_resolution_clock;
template <typename Rep, typename Period>
constexpr float StdDurationToSeconds(std::chrono::duration<Rep, Period> const &_d)
{ return std::chrono::duration_cast<std::chrono::duration<float>>(_d).count(); }

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

GLShaderPtr CompileFKernel(ShaderSources_t const &_kernel_sources);
GLShaderPtr CompileShader(GLenum _type, ShaderSources_t &_sources);
GLProgramPtr LinkProgram(ShaderBinaries_t const &_binaries);

// =============================================================================



// =============================================================================

struct RenderContext::Impl_
{
public:
	Impl_();
	~Impl_();

public:
	utility::File fkernel_file_;
public:
	std::array<int, 2> resolution_;
public:
	bool BuildFKernel(std::string const &_sources);
	GLProgramPtr shader_program_;
	GLShaderPtr cached_vshader_;
	GLShaderPtr cached_fshader_;
public:
	GLuint dummy_vao_;
};

RenderContext::Impl_::Impl_() :
	fkernel_file_{},
	shader_program_{ 0u },
	cached_vshader_{ 0u },
	cached_fshader_{ 0u },
	dummy_vao_{ 0u }
{
	{
		static ShaderSources_t vertex_sources{
			SR_GLSL_VERSION,
			#include "shaders/fullscreen_tri.vert.h"
		};
		cached_vshader_ = CompileShader(GL_VERTEX_SHADER, vertex_sources);

		static ShaderSources_t const default_fkernel{
			SR_SL_DUMMY_FKERNEL
		};
		cached_fshader_ = CompileFKernel(default_fkernel);

		assert(cached_vshader_);
		assert(cached_fshader_);
		ShaderBinaries_t const shader_binaries{
			cached_vshader_,
			cached_fshader_
		};
		shader_program_ = LinkProgram(shader_binaries);
		assert(shader_program_);
	}

	{
		glGenVertexArrays(1, &dummy_vao_);
	}
}

RenderContext::Impl_::~Impl_()
{
	glDeleteVertexArrays(1, &dummy_vao_);
}

bool
RenderContext::Impl_::BuildFKernel(std::string const &_sources)
{
	ShaderSources_t kernel_sources{ _sources.c_str() };
	GLShaderPtr compiled_fshader = CompileFKernel(kernel_sources);
	if (!compiled_fshader) return false;

	assert(cached_vshader_);
	ShaderBinaries_t const shader_binaries{
		cached_vshader_,
		compiled_fshader
	};
	GLProgramPtr linked_program = LinkProgram(shader_binaries);
	if (!linked_program) return false;

	cached_fshader_ = std::move(compiled_fshader);
	shader_program_ = std::move(linked_program);
	return true;
}



// =============================================================================

GLShaderPtr CompileFKernel(ShaderSources_t const &_kernel_sources)
{
	static ShaderSources_t const kKernelPrefix{
		SR_GLSL_VERSION,
		"uniform float " SR_SL_TIME_UNIFORM ";\n",
		"uniform ivec2 " SR_SL_RESOLUTION_UNIFORM ";\n"
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
RenderContext::RenderFrame()
{
	static StdClock::time_point prev_render_time = StdClock::now();
	static StdClock::time_point begin_time = prev_render_time;
	StdClock::duration const delta_time = StdClock::now() - prev_render_time;
	float const elapsed_time = StdDurationToSeconds(prev_render_time - begin_time);
#if 0
	std::cout << StdDurationToSeconds(delta_time) << std::endl;
#endif

	{
		constexpr float kFKernelUpdatePeriod = 1.f;
		static StdClock::time_point prev_fkernel_update = StdClock::now();
		static bool prev_frame_had_fkernel_file = false;
		StdClock::duration const elapsed = StdClock::now() - prev_fkernel_update;
		if (StdDurationToSeconds(elapsed) > kFKernelUpdatePeriod)
		{
			std::cout << "Running fkernel update.." << std::endl;
			prev_fkernel_update = StdClock::now();
			bool const fkernel_file_available = impl_->fkernel_file_.Exists();
			if (fkernel_file_available && impl_->fkernel_file_.HasChanged())
			{
				std::cout << "fkernel file changed, building.." << std::endl;
				impl_->BuildFKernel(impl_->fkernel_file_.ReadAll());
			}
			else if (!fkernel_file_available)
			{
				std::cout << "fkernel file is either nonexistent, or not a regular file" << std::endl;
			}
			else
			{
				std::cout << "compiled fkernel is up to date" << std::endl;
			}
		}
	}

	bool start_over = true;

	assert(!glerror::ClearGLError());

	static GLfloat const clear_color[]{ 0.5f, 0.5f, 0.5f, 1.f };
	glClearBufferfv(GL_COLOR, 0, clear_color);

	glUseProgram(impl_->shader_program_);
	glBindVertexArray(impl_->dummy_vao_);

	{
		int const time_loc = glGetUniformLocation(impl_->shader_program_, SR_SL_TIME_UNIFORM);
		if (time_loc >= 0)
		{
			glUniform1f(time_loc, elapsed_time);
		}
		glerror::PrintError(std::cout);
	}
	{
		int const resolution_loc = glGetUniformLocation(impl_->shader_program_, SR_SL_RESOLUTION_UNIFORM);
		if (resolution_loc >= 0)
		{
			glUniform2iv(resolution_loc, 1, &(impl_->resolution_[0]));
		}
	}

	glDrawArrays(GL_TRIANGLES, 0, 3);
	start_over = start_over && (glerror::PrintError(std::cout) == GL_NO_ERROR);

	glBindVertexArray(0u);
	glUseProgram(0u);

#ifdef SR_SINGLE_BUFFERING
	glFlush();
#endif

	prev_render_time += delta_time;
	return start_over;
}


void
RenderContext::WatchFKernelFile(char const *_path)
{
	impl_->fkernel_file_ = utility::File{ _path };
	if (impl_->fkernel_file_.Exists())
		impl_->BuildFKernel(impl_->fkernel_file_.ReadAll());
}


void
RenderContext::SetResolution(int _width, int _height)
{
	impl_->resolution_ = { _width, _height };
	glViewport(0, 0, _width, _height);
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
