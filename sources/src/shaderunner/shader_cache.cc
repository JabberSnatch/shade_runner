/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "shaderunner/shader_cache.h"


#define SR_SL_ENTRY_POINT(entry_point) "#define SR_ENTRY_POINT " entry_point "\n"
#define SR_VERT_ENTRY_POINT "vertexMain"
#define SR_FRAG_ENTRY_POINT "imageMain"


namespace sr {


oglbase::ShaderSources_t const &
KernelSuffix(ShaderStage _stage)
{
	switch (_stage)
	{
	case ShaderStage::kVertex:
	{
		static oglbase::ShaderSources_t const kKernelSuffix{
			"\n",
			SR_SL_ENTRY_POINT(SR_VERT_ENTRY_POINT),
			#include "shaders/entry_point.vert.h"
		};
		return kKernelSuffix;
	}
	case ShaderStage::kFragment:
	{
		static oglbase::ShaderSources_t const kKernelSuffix{
			"\n",
			SR_SL_ENTRY_POINT(SR_FRAG_ENTRY_POINT),
			#include "shaders/entry_point.frag.h"
		};
		return kKernelSuffix;
	}
	default:
	{
		static oglbase::ShaderSources_t const kEmptySuffix{};
		return kEmptySuffix;
	}
	}
}

oglbase::ShaderSources_t const &
DefaultKernel(ShaderStage _stage)
{
	switch (_stage)
	{
	case ShaderStage::kVertex:
	{
		static oglbase::ShaderSources_t const kDefaultKernel{
			"const vec2 kTriVertices[] = vec2[3](vec2(-1.0, 3.0), vec2(-1.0, -1.0), vec2(3.0, -1.0)); void " SR_VERT_ENTRY_POINT "(inout vec4 vert_position) { vert_position = vec4(kTriVertices[gl_VertexID], 0.0, 1.0); }\n"
		};
		return kDefaultKernel;
	}
	case ShaderStage::kFragment:
	{
		static oglbase::ShaderSources_t const kDefaultKernel{
			"void " SR_FRAG_ENTRY_POINT "(inout vec4 frag_color, vec2 frag_coord) { frag_color = vec4(1.0 - float(gl_PrimitiveID), 0.0, 1.0, 1.0); } \n"
		};
		return kDefaultKernel;
	}
	default:
	{
		static oglbase::ShaderSources_t const kEmptySuffix{};
		return kEmptySuffix;
	}
	}
}

GLenum
ShaderStageToGLenum(ShaderStage _stage)
{
	switch (_stage)
	{
	case ShaderStage::kVertex: return GL_VERTEX_SHADER;
	case ShaderStage::kFragment: return GL_FRAGMENT_SHADER;
	default: return static_cast<GLenum>(0u);
	}
}


oglbase::ShaderBinaries_t
ShaderCache::select(std::set<ShaderStage> const &_stages) const
{
	oglbase::ShaderBinaries_t result{};
	std::transform(_stages.cbegin(), _stages.cend(), std::back_inserter(result),
				   [this](ShaderStage const _stage) {
					   return static_cast<GLuint>(cached_shaders_[static_cast<std::size_t>(_stage)]);
				   });
	return result;
}

ShaderCache::operator oglbase::ShaderBinaries_t() const
{
	return oglbase::ShaderBinaries_t(cached_shaders_.cbegin(),
									 cached_shaders_.cend());
}

oglbase::ShaderPtr&
ShaderCache::operator[](ShaderStage _stage)
{
	return cached_shaders_[static_cast<std::size_t>(_stage)];
}

oglbase::ShaderPtr const&
ShaderCache::operator[](ShaderStage _stage) const
{
	return cached_shaders_[static_cast<std::size_t>(_stage)];
}


} // namespace sr
