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
#include <cstring>
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

#define SR_SL_PROJMAT_UNIFORM "iProjMat"

#define SR_SL_GIZMOS_UNIFORM "iGizmos"
#define SR_SL_GIZMOS_MAX "16"
#define SR_SL_GIZMO_COUNT_UNIFORM "iGizmoCount"

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
//#define SR_GEOMETRY_RENDERING
#ifdef SR_GEOMETRY_RENDERING
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
#endif

namespace sr {

using Resolution_t = std::array<float, 2>;

// =============================================================================

struct RenderContext::Impl_
{
    static std::pair<oglbase::ShaderPtr, ErrorLogContainer>
    CompileKernel(ShaderStage _stage, oglbase::ShaderSources_t const &_kernel_sources);

    Impl_(RenderContext &_context);

    RenderContext &context_;

    utility::Clock exec_time_;

    static constexpr float kKernelsUpdatePeriod = 1.f;
    void KernelsUpdate();
    std::unordered_map<ShaderStage, utility::File> kernel_files_;

    Resolution_t resolution_;

    std::set<ShaderStage> active_stages_;
    ShaderCache shader_cache_;
    oglbase::ProgramPtr shader_program_;

    UniformContainer uniforms_;

    oglbase::VAOPtr dummy_vao_;

    // GEOMETRY RENDERING EXPERIMENTS
#ifdef SR_GEOMETRY_RENDERING
    int point_count_;
    oglbase::VAOPtr vao_;
    oglbase::BufferPtr vertex_buffer_;
#endif
};

RenderContext::Impl_::Impl_(RenderContext &_context) :
    context_{ _context },
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
    uniforms_{},
    dummy_vao_{ 0u }

#ifdef SR_GEOMETRY_RENDERING
    ,point_count_{ 0 },
    vao_{ 0u },
    vertex_buffer_{ 0u }
#endif
{
    {
        glGenVertexArrays(1, dummy_vao_.get());
    }

    {
        for (ShaderStage stage : active_stages_)
        {
            shader_cache_[stage] = CompileKernel(stage, DefaultKernel(stage)).first;
            assert(shader_cache_[stage]);
        }

        oglbase::ShaderBinaries_t const shader_binaries =
            shader_cache_.select(active_stages_);
        shader_program_ = oglbase::LinkProgram(shader_binaries);
        assert(shader_program_);
    }

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

        shader_cache_[ShaderStage::kVertex] = CompileKernel(ShaderStage::kVertex, kProcessingVKernel()).first;
        shader_cache_[ShaderStage::kGeometry] = CompileKernel(ShaderStage::kGeometry, kProcessingGKernel()).first;
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
                                     [this, &updated_shaders](KernelFile &_kernel_file) {
        bool const kernel_file_available = _kernel_file.second.Exists();
        if (kernel_file_available && _kernel_file.second.HasChanged())
        {
            std::cout << "Kernel file changed, building.." << std::endl;
            std::pair<oglbase::ShaderPtr, ErrorLogContainer> comp_result =
                CompileKernel(_kernel_file.first, { _kernel_file.second.ReadAll().c_str() });
            context_.onFKernelCompileFinished(_kernel_file.second.path(), comp_result.second);
            if (!comp_result.first)
            {
                std::cout << "Shader compilation failed" << std::endl;
                return false;
            }

            updated_shaders[_kernel_file.first] = std::move(comp_result.first);
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


std::pair<oglbase::ShaderPtr, ErrorLogContainer>
RenderContext::Impl_::CompileKernel(ShaderStage _stage, oglbase::ShaderSources_t const &_kernel_sources)
{
    static oglbase::ShaderSources_t const kKernelPrefix{
        SR_GLSL_VERSION,
        "uniform float " SR_SL_TIME_UNIFORM ";\n",
        "uniform vec2 " SR_SL_RESOLUTION_UNIFORM ";\n",

        "uniform mat4 " SR_SL_PROJMAT_UNIFORM ";\n",

        "uniform vec3 " SR_SL_GIZMOS_UNIFORM "[" SR_SL_GIZMOS_MAX "];\n",
        "uniform int " SR_SL_GIZMO_COUNT_UNIFORM ";\n",
    };
    oglbase::ShaderSources_t const &kernel_suffix = KernelSuffix(_stage);

    oglbase::ShaderSources_t shader_sources{};
    shader_sources.reserve(kKernelPrefix.size() + _kernel_sources.size() + kernel_suffix.size());
    std::copy(kKernelPrefix.cbegin(), kKernelPrefix.cend(), std::back_inserter(shader_sources));
    std::copy(_kernel_sources.cbegin(), _kernel_sources.cend(), std::back_inserter(shader_sources));
    std::copy(kernel_suffix.cbegin(), kernel_suffix.cend(), std::back_inserter(shader_sources));


    std::string error_msg;
    ErrorLogContainer errorlog;
    oglbase::ShaderPtr shader = oglbase::CompileShader(ShaderStageToGLenum(_stage), shader_sources, &error_msg);
    if (!shader)
    {
        while (!error_msg.empty())
        {
            std::string head = [](std::string &_in){
                std::size_t index = _in.find('\n');
                std::string head = _in.substr(0, index);
                _in = _in.substr(index+1);
                return head;
            }(error_msg);

            int line_index = 0;
            std::string message = head;
            {
                std::size_t const linenum_begin = head.find(':') + 1;
                std::size_t const linenum_end = head.find(':', linenum_begin) + 1;
                std::size_t const paren_begin = head.find('(', linenum_begin);
                std::size_t const message_begin = head.find(':', linenum_end);

                if (linenum_begin != ~0ull &&
                    linenum_end != ~0ull &&
                    paren_begin != ~0ull &&
                    message_begin != ~0ull)
                {
                    message = head.substr(message_begin+2);
                    line_index = std::stoi(head.substr(linenum_begin, paren_begin-linenum_begin)) - 3;
                }
            }

            errorlog.emplace_back(std::make_pair(line_index, message));
        }
    }
    return std::make_pair(std::move(shader), std::move(errorlog));
}


// =============================================================================

RenderContext::RenderContext() :
    impl_(new RenderContext::Impl_(*this))
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

    glDisable(GL_MULTISAMPLE);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);

    static GLfloat const clear_color[]{ 0.5f, 0.5f, 0.5f, 1.f };
    glClearBufferfv(GL_COLOR, 0, clear_color);

    glUseProgram(impl_->shader_program_);

    {
        int const time_loc = glGetUniformLocation(impl_->shader_program_, SR_SL_TIME_UNIFORM);
        if (time_loc >= 0)
            glUniform1f(time_loc, elapsed_time);

        int const resolution_loc = glGetUniformLocation(impl_->shader_program_, SR_SL_RESOLUTION_UNIFORM);
        if (resolution_loc >= 0)
            glUniform2fv(resolution_loc, 1, &(impl_->resolution_[0]));

        int const projmat_loc = glGetUniformLocation(impl_->shader_program_, SR_SL_PROJMAT_UNIFORM);
        if (projmat_loc >= 0)
            glUniformMatrix4fv(projmat_loc, 1, GL_FALSE, &projection_matrix[0]);

        int const gizmos_loc = glGetUniformLocation(impl_->shader_program_, SR_SL_GIZMOS_UNIFORM);
        if (gizmos_loc >= 0)
        {
            int const gizmocount_loc = glGetUniformLocation(impl_->shader_program_, SR_SL_GIZMO_COUNT_UNIFORM);
            if (gizmocount_loc >= 0)
                glUniform1i(gizmocount_loc, (GLint)gizmo_count);

            for (unsigned i = 0; i < kGizmoCountMax; ++i)
                glUniform3fv(gizmos_loc + i, 1, &gizmo_positions[i][0]);
        }
    }

    for (std::pair<std::string, float> const& uniform : impl_->uniforms_)
    {
        int const location = glGetUniformLocation(impl_->shader_program_, uniform.first.c_str());
        if (location >= 0)
            glUniform1f(location, uniform.second);
    }

#ifdef SR_GEOMETRY_RENDERING
    glBindVertexArray(impl_->vao_);
    glDrawArrays(GL_POINTS, 0, impl_->point_count_);
    glBindVertexArray(0u);
#else
    glBindVertexArray(impl_->dummy_vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
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


void
RenderContext::SetUniforms(UniformContainer const&_uniforms)
{
    impl_->uniforms_ = _uniforms;
}

UniformContainer const&
RenderContext::GetUniforms() const
{
    return impl_->uniforms_;
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

extern "C"
{

    void* srCreateContext()
    {
        return new sr::RenderContext();
    }

    void srDeleteContext(void* context)
    {
        delete (sr::RenderContext*)context;
    }

    bool srRenderFrame(void* context, FrameDesc const* desc)
    {
        return ((sr::RenderContext*)context)->RenderFrame();
    }

    void srWatchKernelFile(void* context, std::uint32_t stage, char const* path)
    {
        ((sr::RenderContext*)context)->WatchKernelFile((sr::ShaderStage)stage, path);
    }

    char const* srGetKernelPath(void* context, std::uint32_t stage)
    {
        return ((sr::RenderContext*)context)->GetKernelPath((sr::ShaderStage)stage).c_str();
    }

}

