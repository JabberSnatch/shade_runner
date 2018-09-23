/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "shaderunner/shaderunner.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <iterator>
#include <vector>

#include <boost/numeric/conversion/cast.hpp>
#include <GL/glew.h>

#include "utility/file.h"
#include "utility/clock.h"

#include "oglbase/error.h"
#include "oglbase/handle.h"
#include "oglbase/shader.h"

#include "uibase/imguicontext.h"

#define SR_GLSL_VERSION "#version 330 core\n"
#define SR_SL_ENTRY_POINT(entry_point) "#define SR_ENTRY_POINT " entry_point "\n"
#define SR_SL_TIME_UNIFORM "iTime"
#define SR_SL_RESOLUTION_UNIFORM "iResolution"

/* [ DESIGN DRAFT ]
 * [X] utility
 * [X] |- file
 * [X] |- timing -> clock
 * [X] opengl -> oglbase
 * [X] |- errors
 * [X] |- handles
 * [X] |- shaders
 * [ ] |- render
 * [X] GUI (imgui based) -> uibase
 * [ ] |- input state
 * [ ] |- layout
 * [ ] |- draw
 * [X] |- context
 * [X] shaderunner
 * [ ] |- API
 * [ ] |- editor
 * [X] |- context
 * [ ] |- kernel
 * [ ] platform
 * [X] |- main
 */

namespace sr {

using Resolution_t = std::array<float, 2>;

// =============================================================================

struct RenderContext::Impl_
{
public:
	Impl_();
	~Impl_();

public:
	utility::Clock exec_time_;
public:
	utility::File fkernel_file_;
	static constexpr float kFKernelUpdatePeriod = 1.f;
	float fkernel_update_counter_ = 0.f;
	void FKernelUpdate(float const _elapsed_time);
public:
	uibase::ImGuiContext imgui_;
public:
	Resolution_t resolution_;
public:
	enum class ShaderStage { kVertex = 0, kFragment, kCount };
	static oglbase::ShaderSources_t const &KernelSuffix(ShaderStage _stage);
	static GLenum ShaderStageToGLenum(ShaderStage _stage);

	static oglbase::ShaderPtr CompileKernel(ShaderStage _stage, oglbase::ShaderSources_t const &_kernel_sources);
	bool BuildFKernel(std::string const &_sources);
	oglbase::ProgramPtr shader_program_;
	oglbase::ShaderPtr cached_vshader_;
	oglbase::ShaderPtr cached_fshader_;
public:
	GLuint dummy_vao_;
};

RenderContext::Impl_::Impl_() :
	exec_time_{ [this](float const _dt) {
		FKernelUpdate(_dt);
	}},
	fkernel_file_{},
	imgui_{},
	resolution_{ 0.f, 0.f },
	shader_program_{ 0u },
	cached_vshader_{ 0u },
	cached_fshader_{ 0u },
	dummy_vao_{ 0u }
{
	{
		static oglbase::ShaderSources_t const default_vkernel{
			"const vec2 kTriVertices[] = vec2[3](vec2(-1.0, 3.0), vec2(-1.0, -1.0), vec2(3.0, -1.0)); void vertexMain(inout vec4 vert_position) { vert_position = vec4(kTriVertices[gl_VertexID], 0.0, 1.0); }\n"
		};
		cached_vshader_ = CompileKernel(ShaderStage::kVertex, default_vkernel);

		static oglbase::ShaderSources_t const default_fkernel{
			"void imageMain(inout vec4 frag_color, vec2 frag_coord) { frag_color = vec4(1.0 - float(gl_PrimitiveID), 0.0, 1.0, 1.0); } \n"
		};
		cached_fshader_ = CompileKernel(ShaderStage::kFragment, default_fkernel);

		assert(cached_vshader_);
		assert(cached_fshader_);
		oglbase::ShaderBinaries_t const shader_binaries{
			cached_vshader_,
			cached_fshader_
		};
		shader_program_ = oglbase::LinkProgram(shader_binaries);
		assert(shader_program_);
	}

	{
		glGenVertexArrays(1, &dummy_vao_);
	}
}

RenderContext::Impl_::~Impl_()
{
	glDeleteVertexArrays(1, &dummy_vao_);
}

void
RenderContext::Impl_::FKernelUpdate(float const _increment)
{
	fkernel_update_counter_ += _increment;
	if (fkernel_update_counter_ > kFKernelUpdatePeriod)
	{
#if 0
		std::cout << "Running fkernel update.." << std::endl;
#endif
		fkernel_update_counter_ = 0.f;
		bool const fkernel_file_available = fkernel_file_.Exists();
		if (fkernel_file_available && fkernel_file_.HasChanged())
		{
			std::cout << "fkernel file changed, building.." << std::endl;
			BuildFKernel(fkernel_file_.ReadAll());
		}
		else if (!fkernel_file_available)
		{
			std::cout << "fkernel file is either nonexistent, or not a regular file" << std::endl;
		}
		else
		{
#if 0
			std::cout << "compiled fkernel is up to date" << std::endl;
#endif
		}
	}
}

oglbase::ShaderSources_t const &
RenderContext::Impl_::KernelSuffix(ShaderStage _stage)
{
	switch (_stage)
	{
	case ShaderStage::kVertex:
	{
		static oglbase::ShaderSources_t const kKernelSuffix{
			"\n",
			SR_SL_ENTRY_POINT("vertexMain"),
			#include "shaders/entry_point.vert.h"
		};
		return kKernelSuffix;
	}
	case ShaderStage::kFragment:
	{
		static oglbase::ShaderSources_t const kKernelSuffix{
			"\n",
			SR_SL_ENTRY_POINT("imageMain"),
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

GLenum
RenderContext::Impl_::ShaderStageToGLenum(ShaderStage _stage)
{
	switch (_stage)
	{
	case ShaderStage::kVertex: return GL_VERTEX_SHADER;
	case ShaderStage::kFragment: return GL_FRAGMENT_SHADER;
	default: return static_cast<GLenum>(0u);
	}
}

oglbase::ShaderPtr
RenderContext::Impl_::CompileKernel(ShaderStage _stage, oglbase::ShaderSources_t const &_kernel_sources)
{
	static oglbase::ShaderSources_t const kKernelPrefix{
		SR_GLSL_VERSION,
		"uniform float " SR_SL_TIME_UNIFORM ";\n",
		"uniform vec2 " SR_SL_RESOLUTION_UNIFORM ";\n"
	};
	oglbase::ShaderSources_t const &kernel_suffix = KernelSuffix(_stage);

	oglbase::ShaderSources_t shader_sources{};
	shader_sources.reserve(kKernelPrefix.size() + _kernel_sources.size() + kernel_suffix.size());
	std::copy(kKernelPrefix.cbegin(), kKernelPrefix.cend(), std::back_inserter(shader_sources));
	std::copy(_kernel_sources.cbegin(), _kernel_sources.cend(), std::back_inserter(shader_sources));
	std::copy(kernel_suffix.cbegin(), kernel_suffix.cend(), std::back_inserter(shader_sources));

	return oglbase::CompileShader(ShaderStageToGLenum(_stage), shader_sources);
}

bool
RenderContext::Impl_::BuildFKernel(std::string const &_sources)
{
	oglbase::ShaderSources_t kernel_sources{ _sources.c_str() };
	oglbase::ShaderPtr compiled_fshader = CompileKernel(ShaderStage::kFragment, kernel_sources);
	if (!compiled_fshader) return false;

	assert(cached_vshader_);
	oglbase::ShaderBinaries_t const shader_binaries{
		cached_vshader_,
		compiled_fshader
	};
	oglbase::ProgramPtr linked_program = oglbase::LinkProgram(shader_binaries);
	if (!linked_program) return false;

	cached_fshader_ = std::move(compiled_fshader);
	shader_program_ = std::move(linked_program);
	return true;
}

// =============================================================================

RenderContext::RenderContext() :
	impl_(new RenderContext::Impl_())
{}

RenderContext::~RenderContext()
{ delete impl_; }

bool
RenderContext::RenderFrame()
{
	float const elapsed_time = impl_->exec_time_.read();
	impl_->exec_time_.step();

	bool start_over = true;

	assert(!oglbase::ClearError());

	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	static GLfloat const clear_color[]{ 0.5f, 0.5f, 0.5f, 1.f };
	glClearBufferfv(GL_COLOR, 0, clear_color);

	glUseProgram(impl_->shader_program_);
	glBindVertexArray(impl_->dummy_vao_);

	{
		int const time_loc = glGetUniformLocation(impl_->shader_program_, SR_SL_TIME_UNIFORM);
		if (time_loc >= 0)
		{
			glUniform1f(time_loc, elapsed_time);
		}
	}
	{
		int const resolution_loc = glGetUniformLocation(impl_->shader_program_, SR_SL_RESOLUTION_UNIFORM);
		if (resolution_loc >= 0)
		{
			glUniform2fv(resolution_loc, 1, &(impl_->resolution_[0]));
		}
	}

	glDrawArrays(GL_TRIANGLES, 0, 3);
	start_over = start_over && (glGetError() == GL_NO_ERROR);

	glBindVertexArray(0u);
	glUseProgram(0u);

#ifdef SR_SINGLE_BUFFERING
	glFlush();
#endif

	{
		impl_->imgui_.Render();
	}

	return start_over;
}


void
RenderContext::WatchFKernelFile(char const *_path)
{
	impl_->fkernel_file_ = utility::File{ _path };
	if (impl_->fkernel_file_.Exists())
		impl_->BuildFKernel(impl_->fkernel_file_.ReadAll());
}


void
RenderContext::SetResolution(int _width, int _height)
{
	impl_->resolution_ = { boost::numeric_cast<float>(_width),
						   boost::numeric_cast<float>(_height) };
	impl_->imgui_.SetResolution(impl_->resolution_);
	glViewport(0, 0, _width, _height);
}


std::string const &
RenderContext::GetFKernelPath() const
{
	return impl_->fkernel_file_.path();
}


} //namespace sr
