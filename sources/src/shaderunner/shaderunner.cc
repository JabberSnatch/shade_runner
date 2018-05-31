/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "shaderunner/shaderunner.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <vector>

#include <boost/numeric/conversion/cast.hpp>
#include <GL/glew.h>


#define SR_GLSL_VERSION "#version 330 core\n"
#define SR_SL_ENTRY_POINT(entry_point) "#define SR_ENTRY_POINT " entry_point "\n"
#define SR_SL_DUMMYFSOURCES "out vec4 frag_color; void main() { frag_color = vec4(1.0 - float(gl_PrimitiveID), 0.0, 1.0, 1.0); } "


using StdClock = std::chrono::high_resolution_clock;

namespace sr {

// =============================================================================

namespace glerror {
std::string const &GetGLErrorString(GLenum const _error);
void CheckGLShaderError(std::ostream& _ostream, GLuint const _shader);
GLenum CheckGLError(std::ostream& _ostream);
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
	void reset(GLuint _h){ _delete(); handle = _h; }
private:
	void _delete() { if (handle) Deleter{}(handle); }
	GLuint handle = 0u;
};

struct GLProgramDeleter
{
	void operator()(GLuint _program)
	{
		glDeleteProgram(_program);
	}
};
struct GLShaderDeleter
{
	void operator()(GLuint _shader)
	{
		glDeleteShader(_shader);
	}
};

using GLProgramPtr = GLHandle<GLProgramDeleter>;
using GLShaderPtr = GLHandle<GLShaderDeleter>;

using ShaderSources_t = std::vector<char const *>;

GLShaderPtr CompileFragmentKernel(ShaderSources_t &_kernel_sources);
GLShaderPtr CompileShader(GLenum _type, ShaderSources_t &_sources);


// =============================================================================

template <int I = 0>
struct find_str
{
	using StrType = char const *const;
#if 0
	static constexpr int value(StrType (&_array)[], StrType _value)
	{
		static_assert(I < std::size(_array), "elem not found");
		return !strcmp(_array[I], _value) ? I : find_str<I+1>::value(_array, _value);
	}
#endif
	template <int N>
	static constexpr int value(StrType (&_array)[N], StrType _value)
	{
		static_assert(I < N, "elem not found");
		return (&(_array[I]) == &_value) ? I : find_str<I+1>::value(_array, _value);
	}
};

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
		static ShaderSources_t vertex_sources = {
			SR_GLSL_VERSION,
			#include "shaders/fullscreen_quad.vert.h"
		};

		static ShaderSources_t fragment_sources{
			SR_GLSL_VERSION,
			//			SR_SL_DUMMYFSOURCES,

			R"__SR_SS__(
void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	frag_color.xy = frag_coord / vec2(1280, 720);
	frag_color.a = 1.0;
}
			)__SR_SS__"
			,

			SR_SL_ENTRY_POINT("imageMain"),
			#include "shaders/entry_point.frag.h"
		};

		cached_fshader = CompileShader(GL_FRAGMENT_SHADER, fragment_sources);
		cached_vshader = CompileShader(GL_VERTEX_SHADER, vertex_sources);

		shader_program.reset(glCreateProgram());
		glAttachShader(shader_program, cached_vshader);
		glAttachShader(shader_program, cached_fshader);
		glLinkProgram(shader_program);
		{
			GLint success;
			glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
			if (!success)
				std::cout << "Shader link error ." << std::endl;
		}
	}

	{
		glGenVertexArrays(1, &dummy_vao);
	}
}

RenderContext::Impl_::~Impl_()
{
	glDeleteVertexArrays(1, &dummy_vao);
}


GLShaderPtr CompileFragmentKernel(ShaderSources_t &)
{
	static constexpr char const *kKernelLocation = "\0\0";
	static constexpr char const *kKernelWrapper[] = {
			SR_GLSL_VERSION,
			kKernelLocation,
			SR_SL_ENTRY_POINT("imageMain"),
			#include "shaders/entry_point.frag.h"
	};
	//static constexpr int kIndex = find_str<>::value(kKernelWrapper, kKernelLocation);
	//	static_assert(kKernelLocation == kKernelWrapper[kIndex], "find_elem failed");
	//	std::cout << kIndex << std::endl;

	return GLShaderPtr{ 0u };
}

GLShaderPtr CompileShader(GLenum _type, std::vector<char const *> &_sources)
{
	GLsizei const source_count = boost::numeric_cast<GLsizei>(_sources.size());
	GLShaderPtr result{ glCreateShader(_type) };
	glShaderSource(result, source_count, _sources.data(), NULL);
	glCompileShader(result);
	glerror::CheckGLShaderError(std::cout, result);
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

#if 0
	ShaderSources_t yolo{};
	CompileFragmentKernel(yolo);
	start_over = false;
#endif

	assert([]()
	{
		if (glerror::ClearGLError())
			std::cout << "Lingering GL error detected" << std::endl;
		return true;
	}());

	static GLfloat const clear_color[]{ 0.5f, 0.5f, 0.5f, 1.f };
	glClearBufferfv(GL_COLOR, 0, clear_color);

	glUseProgram(impl_->shader_program);
	glBindVertexArray(impl_->dummy_vao);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	start_over = start_over && (glerror::CheckGLError(std::cout) == GL_NO_ERROR);

	glBindVertexArray(0u);
	glUseProgram(0u);

#ifdef SR_SINGLE_BUFFERING
	glFlush();
#endif

	last_render_time += delta_time;
	return start_over;
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

void CheckGLShaderError(std::ostream& _ostream, GLuint const _shader)
{
	GLint success = GL_FALSE;
	glGetShaderiv(_shader, GL_COMPILE_STATUS, &success);
	if (!success)
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
}

GLenum CheckGLError(std::ostream& _ostream)
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
