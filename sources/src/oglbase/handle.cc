/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "oglbase/handle.h"

#include <iostream>

namespace oglbase {


template <typename Deleter>
void
Handle<Deleter>::_delete()
{
	if (handle)
		Deleter{}(handle);
}


struct ProgramDeleter
{
	void operator()(GLuint _program)
	{
		std::cout << "gl program deleted " << _program << std::endl;
		glDeleteProgram(_program);
	}
};

struct ShaderDeleter
{
	void operator()(GLuint _shader)
	{
		std::cout << "gl shader deleted " << _shader << std::endl;
		glDeleteShader(_shader);
	}
};

struct TextureDeleter
{
	void operator()(GLuint _texture)
	{
		std::cout << "gl texture deleted " << _texture << std::endl;
		glDeleteTextures(1, &_texture);
	}
};


template struct Handle<ProgramDeleter>;
template struct Handle<ShaderDeleter>;
template struct Handle<TextureDeleter>;


} // namespace oglbase
