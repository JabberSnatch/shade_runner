/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma once
#ifndef __YS_OGL_HANDLE_HPP__
#define __YS_OGL_HANDLE_HPP__

#include <GL/glew.h>

namespace oglbase {


template <typename Deleter>
struct Handle
{
	Handle() = default;
	explicit Handle(GLuint _handle) : handle(_handle) {}
	Handle(Handle const &) = delete;
	Handle(Handle &&_v) : handle(_v.handle) { _v.handle = 0u; }
	~Handle() { _delete(); }
	Handle &operator=(Handle const &_v) = delete;
	Handle &operator=(Handle &&_v) { reset(_v.handle); _v.handle = 0u; return *this;}
	operator GLuint() const { return handle; }
	operator bool() const { return handle != 0u; }
	void reset(GLuint _h) { _delete(); handle = _h; }
	GLuint* get() { return &handle; }
private:
	void _delete();
	GLuint handle = 0u;
};

struct ProgramDeleter;
struct ShaderDeleter;
struct TextureDeleter;
struct VAODeleter;
struct BufferDeleter;
struct FBODeleter;

using ProgramPtr = Handle<ProgramDeleter>;
using ShaderPtr = Handle<ShaderDeleter>;
using TexturePtr = Handle<TextureDeleter>;
using VAOPtr = Handle<VAODeleter>;
using BufferPtr = Handle<BufferDeleter>;
using FBOPtr = Handle<FBODeleter>;

} // namespace oglbase


#endif // __YS_OGL_HANDLE_HPP__
