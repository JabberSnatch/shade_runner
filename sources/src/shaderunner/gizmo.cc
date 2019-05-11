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
    #include "../shaderunner/shaders/box.geom.h"
};

static oglbase::ShaderSources_t const kGizmoVert{
    SHADER_VERSION,
    #include "../shaderunner/shaders/uPosition.vert.h"
};

static oglbase::ShaderSources_t const kGizmoFrag{
    SHADER_VERSION,
    #include "../shaderunner/shaders/white.frag.h"
};


using Matrix_t = std::array<float, 16>;
using Vec3_t = std::array<float, 3>;

Matrix_t perspective(float n, float f, float alpha, float woh)
{
    float inv_tan_half_alpha = 1.f / std::tan(alpha * .5f);
    float inv_fmn = 1.f / (f-n);
    return Matrix_t{
        inv_tan_half_alpha, 0.f, 0.f, 0.f,
        0.f, (1.f/woh)*inv_tan_half_alpha, 0.f, 0.f,
        0.f, 0.f, (-(f+n))*inv_fmn, -1.f,
        0.f, 0.f, -2.f*f*n*inv_fmn, 0.f
    };
}

struct CubeGizmo
{
    CubeGizmo() :
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

    void Draw(Vec3_t const& location, Matrix_t const& projection) const;

    oglbase::ProgramPtr shader_program_;
    oglbase::VAOPtr dummy_vao_;
};

void
CubeGizmo::Draw(Vec3_t const& location, Matrix_t const& projection) const
{
    static GLfloat const clear_depth{ 1.f };
    glClearBufferfv(GL_DEPTH, 0, &clear_depth);

    glUseProgram(shader_program_);
    {
        int const projection_loc = glGetUniformLocation(shader_program_, "uProjectionMat");
        if (projection_loc >= 0)
            glUniformMatrix4fv(projection_loc, 1, GL_FALSE, &projection[0]);
    }
    {
        int const position_loc = glGetUniformLocation(shader_program_, "uPosition");
        if (position_loc >= 0)
            glUniform3fv(position_loc, 1, &location[0]);
    }

	glBindVertexArray(dummy_vao_);
	glDrawArrays(GL_POINTS, 0, 1);
	glBindVertexArray(0u);

    glUseProgram(0u);
}
