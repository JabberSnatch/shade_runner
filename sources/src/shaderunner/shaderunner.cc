/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "shaderunner/shaderunner.h"

#include <array>
#include <cassert>
#include <chrono>
#include <iostream>

#include <boost/numeric/conversion/cast.hpp>
#include <GL/glew.h>


using StdClock = std::chrono::high_resolution_clock;

namespace sr {


std::string const &GetGLErrorString(GLenum const _error);
void CheckGLShaderError(std::ostream& _ostream, GLuint const _shader);
GLenum CheckGLError(std::ostream& _ostream);
bool ClearGLError();


struct RenderContext::Impl_
{
public:
	Impl_();
	~Impl_();
public:
	GLuint shader_program;
};

RenderContext::Impl_::Impl_() :
	shader_program{ glCreateProgram() }
{
	static char const *vertex_sources[]{
		"#version 330 core\n",
		#include "shaders/fullscreen_quad.vert.h"
	};
	static GLsizei const vertex_source_count =
		boost::numeric_cast<GLsizei>(std::size(vertex_sources));
	static char const *fragment_sources[]{
		"#version 330 core\n",
		"out vec4 frag_color; void main() { frag_color = vec4(1.0 - float(gl_PrimitiveID), 0.0, 1.0, 1.0); } "
	};
	static GLsizei const fragment_source_count =
		boost::numeric_cast<GLsizei>(std::size(fragment_sources));

	GLuint vertex_shader{ glCreateShader(GL_VERTEX_SHADER) };
	glShaderSource(vertex_shader, vertex_source_count, vertex_sources, NULL);
	glCompileShader(vertex_shader);
	CheckGLShaderError(std::cout, vertex_shader);

	GLuint fragment_shader{ glCreateShader(GL_FRAGMENT_SHADER) };
	glShaderSource(fragment_shader, fragment_source_count, fragment_sources, NULL);
	glCompileShader(fragment_shader);
	CheckGLShaderError(std::cout, fragment_shader);

	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);
	{
		GLint success;
		glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
		if (!success)
			std::cout << "Shader link error ." << std::endl;
	}
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
}

RenderContext::Impl_::~Impl_()
{
	glDeleteProgram(shader_program);
}


RenderContext::RenderContext() :
	impl_(new RenderContext::Impl_())
{}

RenderContext::~RenderContext()
{ delete impl_; }

int
RenderContext::RenderFrame() const
{
	static StdClock::time_point last_render_time = StdClock::now();
	StdClock::duration delta_time = StdClock::now() - last_render_time;

	assert([]()
	{
		if (ClearGLError())
			std::cout << "Lingering GL error detected" << std::endl;
		return true;
	}());

	static GLfloat const clear_color[]{ 0.5f, 0.5f, 0.5f, 1.f };
	glClearBufferfv(GL_COLOR, 0, clear_color);

	glUseProgram(impl_->shader_program);

	GLuint vao = 0u;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	std::cout << "Hello World!" << std::endl;
	std::cout << std::chrono::duration_cast<std::chrono::duration<float>>(delta_time).count() << std::endl;
	glDrawArrays(GL_TRIANGLES, 0, 6);
	CheckGLError(std::cout);

	glBindVertexArray(0u);
	glDeleteVertexArrays(1, &vao);

	glUseProgram(0u);
#ifdef SR_SINGLE_BUFFERING
	glFlush();
#endif
	last_render_time += delta_time;
	return 0;
}


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

} //namespace sr
