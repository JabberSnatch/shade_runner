/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "oglbase/error.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>

#include <boost/numeric/conversion/cast.hpp>


namespace oglbase {


char const* enum_string(GLenum const _value)
{
	switch(_value)
	{
	case GL_NO_ERROR: return "None";
	case GL_INVALID_ENUM: return "Invalid enum";
	case GL_INVALID_VALUE: return "Invalid value";
	case GL_INVALID_OPERATION: return "Invalid operation";
	case GL_INVALID_FRAMEBUFFER_OPERATION: return "Invalid framebuffer operation";
	case GL_OUT_OF_MEMORY: return "Out of memory";
	case GL_STACK_UNDERFLOW: return "Stack underflow";
	case GL_STACK_OVERFLOW: return "Stack overflow";

	case GL_DEBUG_SOURCE_API: return "API";
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "Window system";
	case GL_DEBUG_SOURCE_SHADER_COMPILER: return "Shader compiler";
	case GL_DEBUG_SOURCE_THIRD_PARTY: return "Third party";
	case GL_DEBUG_SOURCE_APPLICATION: return "Application";
	case GL_DEBUG_SOURCE_OTHER: return "Other";

	case GL_DEBUG_TYPE_ERROR: return "Error";
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "Deprecated behaviour";
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "Undefined behaviour";
	case GL_DEBUG_TYPE_PORTABILITY: return "Portability";
	case GL_DEBUG_TYPE_PERFORMANCE: return "Perfomrance";
	case GL_DEBUG_TYPE_MARKER: return "Marker";
	case GL_DEBUG_TYPE_PUSH_GROUP: return "Push group";
	case GL_DEBUG_TYPE_POP_GROUP: return "Pop group";
	case GL_DEBUG_TYPE_OTHER: return "Other";

	case GL_DEBUG_SEVERITY_HIGH: return "High";
	case GL_DEBUG_SEVERITY_MEDIUM: return "Medium";
	case GL_DEBUG_SEVERITY_LOW: return "Low";
	case GL_DEBUG_SEVERITY_NOTIFICATION: return "Notification";

	default: return "Unknown enum";
	}
}

// =============================================================================

struct ShaderInfoFuncs
{
	void getiv(GLuint _shader, GLenum _status_flag, GLint* _result) const
	{
		glGetShaderiv(_shader, _status_flag, _result);
	}
	void getInfoLog(GLuint _shader, GLsizei _max_length, GLsizei* _length, GLchar* _info_log) const
	{
		glGetShaderInfoLog(_shader, _max_length, _length, _info_log);
	}
};

struct ProgramInfoFuncs
{
	void getiv(GLuint _shader, GLenum _status_flag, GLint* _result) const
	{
		glGetProgramiv(_shader, _status_flag, _result);
	}
	void getInfoLog(GLuint _shader, GLsizei _max_length, GLsizei* _length, GLchar* _info_log) const
	{
		glGetProgramInfoLog(_shader, _max_length, _length, _info_log);
	}
};

template <typename InfoFuncs, GLenum kStatusEnum>
bool
GetShaderStatus(GLuint const _handle)
{
	GLint success = GL_FALSE;
	InfoFuncs{}.getiv(_handle, kStatusEnum, &success);
	return success != GL_TRUE;
}

template <typename InfoFuncs>
void
ForwardShaderLog(GLuint const _handle)
{
	GLsizei log_size = 0;
	InfoFuncs{}.getiv(_handle, GL_INFO_LOG_LENGTH, &log_size);
	assert(log_size != 0);
	GLint max_debug_message_length;
	glGetIntegerv(GL_MAX_DEBUG_MESSAGE_LENGTH, &max_debug_message_length);
	char* const info_log = new char[log_size];
	InfoFuncs{}.getInfoLog(_handle, log_size, NULL, info_log);
	glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION,
						 GL_DEBUG_TYPE_ERROR,
						 0u,
						 GL_DEBUG_SEVERITY_LOW,
						 std::min(log_size, max_debug_message_length-1), info_log);
	delete[] info_log;
}

template
bool GetShaderStatus<ShaderInfoFuncs, GL_COMPILE_STATUS>(GLuint const);
template
bool GetShaderStatus<ProgramInfoFuncs, GL_LINK_STATUS>(GLuint const);

template
void ForwardShaderLog<oglbase::ShaderInfoFuncs>(GLuint const);
template
void ForwardShaderLog<oglbase::ProgramInfoFuncs>(GLuint const);


GLenum PrintError(std::ostream& _ostream)
{
	GLenum const error_code = glGetError();
	if (error_code != GL_NO_ERROR)
		_ostream << "OpenGL error : " << enum_string(error_code) << std::endl;
	return error_code;
}

bool ClearError()
{
	bool const result = (glGetError() != GL_NO_ERROR);
	while(glGetError() != GL_NO_ERROR);
	return result;
}

// =============================================================================

struct StdCoutPolicy
{
private:
	struct new_line_t {};
public:
	static constexpr new_line_t new_line() { return new_line_t{}; }

	template <typename T>
	StdCoutPolicy& operator<<(T&& _contents)
	{
		std::cout << _contents;
		return *this;
	}

	StdCoutPolicy& operator<<(new_line_t)
	{
		std::cout << std::endl;
		return *this;
	}
};


struct DemoFilePolicy
{
	DemoFilePolicy() = default;
	static char const* const new_line() { return "\n"; }
	template <typename T>
	DemoFilePolicy& operator<<(T&& _contents)
	{
		file << _contents;
		return *this;
	}

	std::ofstream file{ "muc.txt", std::ios_base::app | std::ios_base::out };
};


template <typename OutputPolicy>
void
DebugMessageControl<OutputPolicy>::ProcessDebugMessage(
	GLenum _source,
	GLenum _type,
	GLuint _id,
	GLenum _severity,
	GLsizei _length,
	GLchar const* const _message,
	void const* const _debugMessageControl)
{
	auto& messageControl =
		*reinterpret_cast<DebugMessageControl<OutputPolicy> const*>
		(_debugMessageControl);
	messageControl.FilterDebugMessage(_source, _type, _id, _severity, _length, _message, nullptr);
}


template <typename OutputPolicy>
DebugMessageControl<OutputPolicy>::DebugMessageControl()
{
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(oglbase::DebugMessageControl<>::ProcessDebugMessage,
						   this);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	oglbase::PrintError(std::cout);
	SetMinSeverity(GL_DEBUG_SEVERITY_LOW);
}


template <typename OutputPolicy>
void
DebugMessageControl<OutputPolicy>::SetSourceEnabled(GLenum _source, bool _enable) const
{
	assert(
		_source == GL_DEBUG_SOURCE_API ||
		_source == GL_DEBUG_SOURCE_WINDOW_SYSTEM ||
		_source == GL_DEBUG_SOURCE_SHADER_COMPILER ||
		_source == GL_DEBUG_SOURCE_THIRD_PARTY ||
		_source == GL_DEBUG_SOURCE_APPLICATION ||
		_source == GL_DEBUG_SOURCE_OTHER
		);
	glDebugMessageControl(_source, GL_DONT_CARE, GL_DONT_CARE,
						  0, nullptr, _enable ? GL_TRUE : GL_FALSE);
	oglbase::PrintError(std::cout);
}


template <typename OutputPolicy>
void
DebugMessageControl<OutputPolicy>::SetTypeEnabled(GLenum _type, bool _enable) const
{
	assert(
		_type == GL_DEBUG_TYPE_ERROR ||
		_type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR ||
		_type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR ||
		_type == GL_DEBUG_TYPE_PORTABILITY ||
		_type == GL_DEBUG_TYPE_PERFORMANCE ||
		_type == GL_DEBUG_TYPE_MARKER ||
		_type == GL_DEBUG_TYPE_PUSH_GROUP ||
		_type == GL_DEBUG_TYPE_POP_GROUP ||
		_type == GL_DEBUG_TYPE_OTHER
		);
	glDebugMessageControl(GL_DONT_CARE, _type, GL_DONT_CARE,
						  0, nullptr, _enable ? GL_TRUE : GL_FALSE);
	oglbase::PrintError(std::cout);
}

template <typename OutputPolicy>
void
DebugMessageControl<OutputPolicy>::SetMinSeverity(GLenum _severity) const
{
	assert(
		_severity == GL_DEBUG_SEVERITY_HIGH ||
		_severity == GL_DEBUG_SEVERITY_MEDIUM ||
		_severity == GL_DEBUG_SEVERITY_LOW ||
		_severity == GL_DEBUG_SEVERITY_NOTIFICATION
		);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, _severity,
						  0, nullptr, GL_TRUE);
	switch(_severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM,
							  0, nullptr, GL_FALSE);
	case GL_DEBUG_SEVERITY_MEDIUM:
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW,
							  0, nullptr, GL_FALSE);
	case GL_DEBUG_SEVERITY_LOW:
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION,
							  0, nullptr, GL_FALSE);
	default: break;
	}
	switch(_severity)
	{
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW,
							  0, nullptr, GL_TRUE);
	case GL_DEBUG_SEVERITY_LOW:
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM,
							  0, nullptr, GL_TRUE);
	case GL_DEBUG_SEVERITY_MEDIUM:
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH,
							  0, nullptr, GL_TRUE);
	default: break;
	}
	oglbase::PrintError(std::cout);
}

template <typename OutputPolicy>
void
DebugMessageControl<OutputPolicy>::FilterDebugMessage(
	GLenum _source,
	GLenum _type,
	GLuint /*_id*/,
	GLenum _severity,
	GLsizei _length,
	GLchar const* const _message,
	void const* const /*_userParam*/) const
{
	std::string message_str{};
	if (_length > 0)
	{
		message_str = std::string{ _message, boost::numeric_cast<size_t>(_length) };
	}
	else
	{
		message_str = std::string{ _message };
	}

	OutputPolicy{} << "[" << enum_string(_source) << "] "
				   << "[" << enum_string(_type) << "] "
				   << "[" << enum_string(_severity) << "] " << OutputPolicy::new_line()
				   << message_str << OutputPolicy::new_line();
}


template class DebugMessageControl<StdCoutPolicy>;



} // namespace oglbase
