/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma once
#ifndef __YS_OGL_ERROR_HPP__
#define __YS_OGL_ERROR_HPP__

#include <GL/glew.h>

#include <string>
#include <iostream>


namespace oglbase {


char const* enum_string(GLenum const _value);

// =============================================================================

struct GetShaderivFunc;
struct GetProgramivFunc;

template <typename GetivFunc, GLenum kStatusEnum>
bool GetShaderStatus(GLuint const _handle);
template <typename GetivFunc>
void ForwardShaderLog(GLuint const _handle);

GLenum PrintError(std::ostream& _ostream);
bool ClearError();

// =============================================================================

struct StdCoutPolicy;

template <typename OutputPolicy = StdCoutPolicy>
class DebugMessageControl
{
public:
	static void ProcessDebugMessage(GLenum _source,
									GLenum _type,
									GLuint _id,
									GLenum _severity,
									GLsizei _length,
									GLchar const* const _message,
									void const* const _debugMessageControl);
public:
	DebugMessageControl();
	explicit DebugMessageControl(OutputPolicy&) : DebugMessageControl() {};
	void SetSourceEnabled(GLenum _source, bool _enable) const;
	void SetTypeEnabled(GLenum _type, bool _enable) const;
	void SetMinSeverity(GLenum _severity) const;
private:
	void FilterDebugMessage(GLenum _source,
							GLenum _type,
							GLuint _id,
							GLenum _severity,
							GLsizei _length,
							GLchar const* const _message,
							void const* const _userParam) const;
};


} // namespace oglbase


#endif // __YS_OGL_ERROR_HPP__

