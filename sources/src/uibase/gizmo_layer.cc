/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. You can do whatever you want with this
 * stuff. If we meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "uibase/gizmo_layer.h"

#include <cassert>
#include <iostream>

#include "oglbase/error.h"
#include "oglbase/shader.h"


namespace {

#define SHADER_VERSION "#version 330 core\n"

static oglbase::ShaderSources_t const kBoxGeom{
    SHADER_VERSION,
    #include "./shaders/box.geom.h"
};

static oglbase::ShaderSources_t const kTransfoGeom{
    SHADER_VERSION,
    #include "./shaders/transfo.geom.h"
};

static oglbase::ShaderSources_t const kGizmoVert{
    SHADER_VERSION,
    #include "./shaders/uPosition.vert.h"
};

static oglbase::ShaderSources_t const kGizmoFrag{
    SHADER_VERSION,
    #include "./shaders/gizmo.frag.h"
};

static GLfloat const kGizmoClearColor[]{ 0.f, 0.f, 0.f, 0.f };

} // namespace


namespace uibase {

GizmoProgram::GizmoProgram(oglbase::ShaderSources_t const& _geom_sources) :
    shader_program_{ 0u },
    dummy_vao_{ 0u }
{
    glGenVertexArrays(1, dummy_vao_.get());

    std::array<oglbase::ShaderPtr, 3> const shaders{
        oglbase::CompileShader(GL_VERTEX_SHADER, kGizmoVert),
        oglbase::CompileShader(GL_GEOMETRY_SHADER, _geom_sources),
        oglbase::CompileShader(GL_FRAGMENT_SHADER, kGizmoFrag)
    };
    oglbase::ShaderBinaries_t const binaries{ shaders[0], shaders[1], shaders[2] };
    shader_program_ = oglbase::LinkProgram(binaries);
    std::cout << oglbase::enum_string(glGetError()) << std::endl;
    assert(shader_program_);
}

void
GizmoProgram::Draw(GizmoDesc const &_desc, unsigned _id, Mat4_t const& _projection) const
{
    glUseProgram(shader_program_);
    {
        int const projection_loc = glGetUniformLocation(shader_program_, "uProjectionMat");
        if (projection_loc >= 0)
            glUniformMatrix4fv(projection_loc, 1, GL_FALSE, &_projection[0]);
    }

    {
        int const position_loc = glGetUniformLocation(shader_program_, "uPosition");
        if (position_loc >= 0)
            glUniform3fv(position_loc, 1, &_desc.position_[0]);
    }

    {
        int const color_loc = glGetUniformLocation(shader_program_, "uGizmoColor");
        if (color_loc >= 0)
            glUniform3fv(color_loc, 1, &_desc.color_[0]);

        int const id_loc = glGetUniformLocation(shader_program_, "uGizmoID");
        static_assert(sizeof(GLuint) == sizeof(unsigned), "");
        glUniform1ui(id_loc, *(GLuint*)&_id);
    }

	glBindVertexArray(dummy_vao_);
	glDrawArrays(GL_POINTS, 0, 1);
	glBindVertexArray(0u);

    glUseProgram(0u);
}


GizmoLayer::GizmoLayer(Mat4_t const& _projection) :
    projection_{ _projection },
    gizmos_{},
    box_program_{ kBoxGeom },
    transfo_program_{ kTransfoGeom }
{}


GizmoDesc&
GizmoLayer::GetGizmo(std::uint32_t _gizmo_index)
{
    const std::uint32_t gizmo_index = uibase::UnpackGizmoIndex(_gizmo_index);
    return gizmos_[gizmo_index-1];
}


void
GizmoLayer::ClearIDBuffer() const
{
    glClearBufferfv(GL_COLOR, 1, &kGizmoClearColor[0]);
}


void
GizmoLayer::RenderFrame() const
{
    ClearIDBuffer();

    glEnable(GL_DEPTH_TEST);
    static GLfloat const clear_depth{ 1.f };
    glClearBufferfv(GL_DEPTH, 0, &clear_depth);

    for (unsigned i = 0; i < unsigned(gizmos_.size()); ++i)
    {
        GizmoDesc const& gizmo_desc = gizmos_[i];

        GizmoProgram const* program{};
        switch(gizmo_desc.type_)
        {
        case eGizmoType::kBox: program = &box_program_; break;
        case eGizmoType::kTransform: program = &transfo_program_; break;
        default:
            break;
        }

        program->Draw(gizmo_desc, i+1, projection_);
    }
}


} // namespace uibase
