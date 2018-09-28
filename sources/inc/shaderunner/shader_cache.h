/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma once
#ifndef __YS_SHADER_CACHE_HPP__
#define __YS_SHADER_CACHE_HPP__

#include <array>
#include <set>

#include <GL/glew.h>

#include "oglbase/shader.h"

namespace sr {


enum class ShaderStage { kVertex = 0, kFragment, kCount };
oglbase::ShaderSources_t const &KernelSuffix(ShaderStage _stage);
oglbase::ShaderSources_t const &DefaultKernel(ShaderStage _stage);
GLenum ShaderStageToGLenum(ShaderStage _stage);


class ShaderCache
{
public:
	ShaderCache() = default;
public:
	template <ShaderStage ... kStages>
	oglbase::ShaderBinaries_t select() const;
	oglbase::ShaderBinaries_t select(std::set<ShaderStage> const &_stages) const;
	operator oglbase::ShaderBinaries_t() const;

	oglbase::ShaderPtr& operator[](ShaderStage _stage);
	oglbase::ShaderPtr const& operator[](ShaderStage _stage) const;
public:
	using ShadersContainer_t =
		std::array<oglbase::ShaderPtr, static_cast<std::size_t>(ShaderStage::kCount)>;
	ShadersContainer_t cached_shaders_;
};


template <ShaderStage ... kStages>
oglbase::ShaderBinaries_t
ShaderCache::select() const
{
	return oglbase::ShaderBinaries_t{
		cached_shaders_[static_cast<std::size_t>(kStages)]...
	};
}


} // namesapce sr


#endif // __YS_SHADER_CACHE_HPP__
