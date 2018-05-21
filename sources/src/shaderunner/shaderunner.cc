/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "shaderunner/shaderunner.h"

#include <iostream>

#include <GL/glew.h>


namespace sr {

int entry_point()
{
	static const GLfloat clear_color[]{ 0.5f, 0.5f, 0.5f, 1.f };
	glClearBufferfv(GL_COLOR, 0, clear_color);
	std::cout << "Hello World!" << std::endl;
	glFlush();
	return 0;
}

} //namespace sr
