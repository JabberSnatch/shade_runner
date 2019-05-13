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
#include <cassert>
#include <chrono>
#include <iostream>
#include <iterator>
#include <set>
#include <unordered_map>
#include <vector>

#include <boost/numeric/conversion/cast.hpp>
#include <GL/glew.h>

#include "utility/file.h"
#include "utility/clock.h"

#include "oglbase/error.h"
#include "oglbase/handle.h"
#include "oglbase/shader.h"

#define SR_GLSL_VERSION "#version 330 core\n"
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
 * [ ] |- shader_kernel
 * [ ] platform
 * [X] |- main
 */

// GEOMETRY RENDERING EXPERIMENTS
namespace {

static oglbase::ShaderSources_t const& kProcessingVKernel()
{
    static oglbase::ShaderSources_t const result{
        "in vec3 in_position;\n",
        "void vertexMain(inout vec4 vert_position){\n",
        "vert_position = vec4(in_position, 1.0);",
        "}\n"
    };
    return result;
}

static oglbase::ShaderSources_t const& kProcessingGKernel()
{
    static oglbase::ShaderSources_t const result{
        "layout(points) in;\n",
        "layout(triangle_strip, max_vertices = 24) out;\n",
        "const vec4 kBaseExtent = 0.2 * vec4(1.0, 1.0, 1.0, 0.0);\n",
        "void main() {\n",
        "vec4 in_position = gl_in[0].gl_Position;",
        /*
         * Triangle strips make for the following point ordering for each face :
         * 2 ----- 4
         * |\      |
         * |  \    |
         * |    \  |
         * |      \|
         * 1 ----- 3
         */
        "gl_Position = in_position + vec4(1, -1, 1, 0) * kBaseExtent; EmitVertex();",
        "gl_Position = in_position + vec4(-1, -1, 1, 0) * kBaseExtent; EmitVertex();",
        "gl_Position = in_position + vec4(1, -1, -1, 0) * kBaseExtent; EmitVertex();",
        "gl_Position = in_position + vec4(-1, -1, -1, 0) * kBaseExtent; EmitVertex();",
        "EndPrimitive();",

        "gl_Position = in_position + vec4(1, -1, -1, 0) * kBaseExtent; EmitVertex();",
        "gl_Position = in_position + vec4(1, 1, -1, 0) * kBaseExtent; EmitVertex();",
        "gl_Position = in_position + vec4(1, -1, 1, 0) * kBaseExtent; EmitVertex();",
        "gl_Position = in_position + vec4(1, 1, 1, 0) * kBaseExtent; EmitVertex();",
        "EndPrimitive();",

        "gl_Position = in_position + vec4(1, -1, 1, 0) * kBaseExtent; EmitVertex();",
        "gl_Position = in_position + vec4(1, 1, 1, 0) * kBaseExtent; EmitVertex();",
        "gl_Position = in_position + vec4(-1, -1, 1, 0) * kBaseExtent; EmitVertex();",
        "gl_Position = in_position + vec4(-1, 1, 1, 0) * kBaseExtent; EmitVertex();",
        "EndPrimitive();",

        "gl_Position = in_position + vec4(-1, -1, 1, 0) * kBaseExtent; EmitVertex();",
        "gl_Position = in_position + vec4(-1, 1, 1, 0) * kBaseExtent; EmitVertex();",
        "gl_Position = in_position + vec4(-1, -1, -1, 0) * kBaseExtent; EmitVertex();",
        "gl_Position = in_position + vec4(-1, 1, -1, 0) * kBaseExtent; EmitVertex();",
        "EndPrimitive();",

        "gl_Position = in_position + vec4(-1, -1, -1, 0) * kBaseExtent; EmitVertex();",
        "gl_Position = in_position + vec4(-1, 1, -1, 0) * kBaseExtent; EmitVertex();",
        "gl_Position = in_position + vec4(1, -1, -1, 0) * kBaseExtent; EmitVertex();",
        "gl_Position = in_position + vec4(1, 1, -1, 0) * kBaseExtent; EmitVertex();",
        "EndPrimitive();",

        "gl_Position = in_position + vec4(1, 1, -1, 0) * kBaseExtent; EmitVertex();",
        "gl_Position = in_position + vec4(-1, 1, -1, 0) * kBaseExtent; EmitVertex();",
        "gl_Position = in_position + vec4(1, 1, 1, 0) * kBaseExtent; EmitVertex();",
        "gl_Position = in_position + vec4(-1, 1, 1, 0) * kBaseExtent; EmitVertex();",
        "EndPrimitive();",
        "}"
    };
    return result;
}

static std::vector<float> const& kTempPointVector()
{
    static std::vector<float> const result{
        -0.25f, -0.25f, -0.25f,
        0.25f, -0.25f, -0.25f,
        0.25f, 0.25f, -0.25f,
        -0.25f, 0.25f, -0.25f
    };
    return result;
}

} // namespace

namespace sr {

using Resolution_t = std::array<float, 2>;

// =============================================================================

struct RenderContext::Impl_
{
public:
	Impl_();

public:
	utility::Clock exec_time_;

public:
	static constexpr float kKernelsUpdatePeriod = 1.f;
    void KernelsUpdate();
    std::unordered_map<ShaderStage, utility::File> kernel_files_;

public:
	Resolution_t resolution_;

public:
	static oglbase::ShaderPtr CompileKernel(ShaderStage _stage, oglbase::ShaderSources_t const &_kernel_sources);
	std::set<ShaderStage> active_stages_;
	ShaderCache shader_cache_;
	oglbase::ProgramPtr shader_program_;

public:
	oglbase::VAOPtr dummy_vao_;

    // GEOMETRY RENDERING EXPERIMENTS
public:
    int point_count_;
    oglbase::VAOPtr vao_;
    oglbase::BufferPtr vertex_buffer_;
};

RenderContext::Impl_::Impl_() :
	exec_time_{ [this](float const _dt) {
        static auto kernels_timeout = [this, update_counter = 0.f](float _dt) mutable {
            update_counter += _dt;
            if (update_counter > kKernelsUpdatePeriod)
            {
                update_counter = 0.f;
                this->KernelsUpdate();
            }
        };
        kernels_timeout(_dt);
	}},
	resolution_{ 0.f, 0.f },
	active_stages_{ ShaderStage::kVertex, ShaderStage::kFragment },
	shader_cache_{},
	shader_program_{ 0u },
	dummy_vao_{ 0u },

    point_count_{ 0 },
    vao_{ 0u },
    vertex_buffer_{ 0u }
{
	{
		glGenVertexArrays(1, dummy_vao_.get());
	}

	{
		for (ShaderStage stage : active_stages_)
		{
			shader_cache_[stage] = CompileKernel(stage, DefaultKernel(stage));
			assert(shader_cache_[stage]);
		}

		oglbase::ShaderBinaries_t const shader_binaries =
			shader_cache_.select(active_stages_);
		shader_program_ = oglbase::LinkProgram(shader_binaries);
		assert(shader_program_);
	}

    //#define SR_GEOMETRY_RENDERING
#ifdef SR_GEOMETRY_RENDERING
    // GEOMETRY RENDERING EXPERIMENTS
    {
        std::vector<float> const& points = kTempPointVector();
        point_count_ = static_cast<int>(points.size() / 3u);
        glGenVertexArrays(1, vao_.get());
        glGenBuffers(1, vertex_buffer_.get());

        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
        glBufferData(GL_ARRAY_BUFFER, point_count_ * 3 * sizeof(float),
                     points.data(), GL_STATIC_DRAW);

        glBindVertexArray(vao_);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(0));
        glEnableVertexAttribArray(0);
        glBindVertexArray(0u);

        glBindBuffer(GL_ARRAY_BUFFER, 0u);

        shader_cache_[ShaderStage::kVertex] = CompileKernel(ShaderStage::kVertex, kProcessingVKernel());
        shader_cache_[ShaderStage::kGeometry] = CompileKernel(ShaderStage::kGeometry, kProcessingGKernel());
        active_stages_ = std::set<ShaderStage>{ ShaderStage::kVertex,
                                                ShaderStage::kGeometry,
                                                ShaderStage::kFragment };
        shader_program_ = oglbase::LinkProgram(shader_cache_.select(active_stages_));
    }
#endif
}


void
RenderContext::Impl_::KernelsUpdate()
{
    using KernelFile = std::pair<const ShaderStage, utility::File>;
    using UpdatedShadersLUT = std::unordered_map<ShaderStage, oglbase::ShaderPtr>;
    UpdatedShadersLUT updated_shaders;

    bool link_required = std::any_of(std::begin(kernel_files_), std::end(kernel_files_),
                                     [&updated_shaders](KernelFile &_kernel_file) {
        bool const kernel_file_available = _kernel_file.second.Exists();
        if (kernel_file_available && _kernel_file.second.HasChanged())
        {
            std::cout << "Kernel file changed, building.." << std::endl;
            oglbase::ShaderPtr compiled_shader = CompileKernel(_kernel_file.first, { _kernel_file.second.ReadAll().c_str() });
            if (!compiled_shader)
            {
                std::cout << "Shader compilation failed" << std::endl;
                return false;
            }

            updated_shaders[_kernel_file.first] = std::move(compiled_shader);
            return true;
        }
        else if (!kernel_file_available)
        {
            std::cout << "Kernel file is either nonexistent, or not a regular file" << std::endl;
        }
        return false;
    });

    if (link_required)
    {
        oglbase::ProgramPtr linked_program = oglbase::LinkProgram(
            [](std::set<ShaderStage> const& _active_stages,
               UpdatedShadersLUT const& _updated_shaders,
               ShaderCache const& _shader_cache)
            {
                oglbase::ShaderBinaries_t result{};
                for (ShaderStage stage : _active_stages)
                {
                    auto const updated_shader_it = _updated_shaders.find(stage);
                    if (updated_shader_it != std::end(_updated_shaders))
                    {
                        result.emplace_back(updated_shader_it->second);
                    }
                    else
                    {
                        oglbase::ShaderPtr const& cached_shader = _shader_cache[stage];
                        assert(cached_shader);
                        result.emplace_back(cached_shader);
                    }
                }
                assert(result.size() == _active_stages.size());
                return result;
            } (active_stages_, updated_shaders, shader_cache_)
        );
        if (!linked_program)
        {
            std::cout << "Program link failed" << std::endl;
            return;
        }

        std::cout << "Linked updated program" << std::endl;

        for (auto&& updated_shader : updated_shaders)
            shader_cache_[updated_shader.first] = std::move(updated_shader.second);
        shader_program_ = std::move(linked_program);
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
    glEnable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);

	static GLfloat const clear_color[]{ 0.5f, 0.5f, 0.5f, 1.f };
	glClearBufferfv(GL_COLOR, 0, clear_color);

	glUseProgram(impl_->shader_program_);

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

#ifndef SR_GEOMETRY_RENDERING
	glBindVertexArray(impl_->dummy_vao_);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0u);
#else
    // GEOMETRY RENDERING EXPERIMENTS
    glBindVertexArray(impl_->vao_);
    glDrawArrays(GL_POINTS, 0, impl_->point_count_);
    glBindVertexArray(0u);
#endif

	glUseProgram(0u);

#ifdef SR_SINGLE_BUFFERING
	glFlush();
#endif

	start_over = start_over && (glGetError() == GL_NO_ERROR);
	return start_over;
}

void
RenderContext::WatchKernelFile(ShaderStage _stage, char const *_path)
{
    if (impl_->active_stages_.count(_stage) != 0)
    {
        impl_->kernel_files_[_stage] = utility::File{ _path };
        impl_->KernelsUpdate();
    }
}

void
RenderContext::SetResolution(int _width, int _height)
{
	impl_->resolution_ = { boost::numeric_cast<float>(_width),
						   boost::numeric_cast<float>(_height) };
	glViewport(0, 0, _width, _height);
}


std::string const &
RenderContext::GetKernelPath(ShaderStage _stage) const
{
    auto const kernel_file_it = impl_->kernel_files_.find(_stage);
    if (kernel_file_it != std::end(impl_->kernel_files_))
        return kernel_file_it->second.path();
    else
    {
        static std::string const empty{ "" };
        return empty;
    }
}


} //namespace sr
