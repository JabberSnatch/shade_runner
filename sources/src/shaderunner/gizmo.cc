/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. You can do whatever you want with this
 * stuff. If we meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include <cassert>
#include <cmath>

#include "oglbase/error.h"
#include "oglbase/handle.h"
#include "oglbase/shader.h"


#define SHADER_VERSION "#version 330 core\n"

static oglbase::ShaderSources_t const kGizmoGeom{
    SHADER_VERSION,
    #include "./shaders/box.geom.h"
};

static oglbase::ShaderSources_t const kGizmoVert{
    SHADER_VERSION,
    #include "./shaders/uPosition.vert.h"
};

static oglbase::ShaderSources_t const kGizmoFrag{
    SHADER_VERSION,
    #include "./shaders/gizmo.frag.h"
};


using Matrix_t = std::array<float, 16>;
using Vec3_t = std::array<float, 3>;

Matrix_t perspective(float n, float f, float alpha, float how)
{
    float inv_tan_half_alpha = 1.f / std::tan(alpha * .5f);
    float inv_fmn = 1.f / (f-n);
    return Matrix_t{
        inv_tan_half_alpha, 0.f, 0.f, 0.f,
        0.f, (1.f/how)*inv_tan_half_alpha, 0.f, 0.f,
        0.f, 0.f, (-(f+n))*inv_fmn, -1.f,
        0.f, 0.f, -2.f*f*n*inv_fmn, 0.f
    };
}

struct GizmoDesc
{
    Vec3_t position_;
    Vec3_t color_;
};

struct Gizmo
{
    Gizmo() :
        shader_program_{ 0u },
        dummy_vao_{ 0u }
    {
		glGenVertexArrays(1, dummy_vao_.get());

        std::array<oglbase::ShaderPtr, 3> const shaders{
            oglbase::CompileShader(GL_VERTEX_SHADER, kGizmoVert),
            oglbase::CompileShader(GL_GEOMETRY_SHADER, kGizmoGeom),
            oglbase::CompileShader(GL_FRAGMENT_SHADER, kGizmoFrag)
        };
        oglbase::ShaderBinaries_t const binaries{ shaders[0], shaders[1], shaders[2] };
        shader_program_ = oglbase::LinkProgram(binaries);
        std::cout << oglbase::enum_string(glGetError()) << std::endl;
        assert(shader_program_);
    }

    void Draw(GizmoDesc const &_desc, unsigned _id, Matrix_t const& _projection) const;

    oglbase::ProgramPtr shader_program_;
    oglbase::VAOPtr dummy_vao_;
};

void
Gizmo::Draw(GizmoDesc const &_desc, unsigned _id, Matrix_t const& _projection) const
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
        glUniform3fv(color_loc, 1, &_desc.color_[0]);
        int const id_loc = glGetUniformLocation(shader_program_, "uGizmoID");
        static_assert(sizeof(GLint) == sizeof(unsigned), "");
        glUniform1i(id_loc, *(GLint*)&_id);
    }

	glBindVertexArray(dummy_vao_);
	glDrawArrays(GL_POINTS, 0, 1);
	glBindVertexArray(0u);

    glUseProgram(0u);
}


struct GizmoLayer
{
    GizmoLayer(Matrix_t const& _projection) :
        projection_{ _projection }
    {}
    void RenderFrame() const;
    Matrix_t projection_;
    std::vector<GizmoDesc> gizmos_;
    Gizmo renderable_;
};


void
GizmoLayer::RenderFrame() const
{
    static GLfloat const kGizmoClearColor[]{ 0.f, 0.f, 0.f, 0.f };
    glClearBufferfv(GL_COLOR, 1, &kGizmoClearColor[0]);

    glEnable(GL_DEPTH_TEST);
    static GLfloat const clear_depth{ 1.f };
    glClearBufferfv(GL_DEPTH, 0, &clear_depth);

    for (unsigned i = 0; i < unsigned(gizmos_.size()); ++i)
    {
        GizmoDesc const& gizmo_desc = gizmos_[i];
        renderable_.Draw(gizmo_desc, i+1, projection_);
    }
}
