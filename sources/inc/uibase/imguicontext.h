/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma once
#ifndef __YS_IMGUICONTEXT_HPP__
#define __YS_IMGUICONTEXT_HPP__

#include <array>

#include "oglbase/handle.h"

namespace uibase {

using Resolution_t = std::array<float, 2>;


class ImGuiContext
{
public:
	ImGuiContext();
	~ImGuiContext();
public:
	void Render() const;
	void SetResolution(Resolution_t const &_resolution);
public:
	oglbase::TexturePtr font_texture_;
	oglbase::ProgramPtr shader_program_;
};


} // namespace uibase


#endif // __YS_IMGUICONTEXT_HPP__
