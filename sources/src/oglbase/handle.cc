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

struct VAODeleter
{
	void operator()(GLuint _vao)
	{
		std::cout << "gl vao deleted " << _vao << std::endl;
		glDeleteVertexArrays(1, &_vao);
	}
};

struct BufferDeleter
{
	void operator()(GLuint _buffer)
	{
		std::cout << "gl buffer deleted " << _buffer << std::endl;
		glDeleteBuffers(1, &_buffer);
	}
};

struct FBODeleter
{
    void operator()(GLuint _fbo)
    {
        std::cout << "gl fbo deleted " << _fbo << std::endl;
        glDeleteFramebuffers(1, &_fbo);
    }
};


template struct Handle<ProgramDeleter>;
template struct Handle<ShaderDeleter>;
template struct Handle<TextureDeleter>;
template struct Handle<VAODeleter>;
template struct Handle<BufferDeleter>;
template struct Handle<FBODeleter>;


} // namespace oglbase
