/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. You can do whatever you want with this
 * stuff. If we meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma once
#ifndef __YS_GIZMOLAYER_HPP__
#define __YS_GIZMOLAYER_HPP__

#include <array>
#include <cmath>
#include <vector>

#include "oglbase/handle.h"

namespace uibase {

using Vec3_t = std::array<float, 3>;
using Matrix_t = std::array<float, 16>;

constexpr Matrix_t perspective(float n, float f, float alpha, float how)
{
    const float inv_tan_half_alpha = 1.f / std::tan(alpha * .5f);
    const float inv_fmn = 1.f / (f-n);
    return Matrix_t{
        inv_tan_half_alpha, 0.f, 0.f, 0.f,
        0.f, (1.f/how)*inv_tan_half_alpha, 0.f, 0.f,
        0.f, 0.f, (-(f+n))*inv_fmn, -1.f,
        0.f, 0.f, -2.f*f*n*inv_fmn, 0.f
    };
}


enum class eGizmoType;

struct GizmoDesc
{
    eGizmoType type_;
    Vec3_t position_;

    Vec3_t color_;
};

enum class eGizmoType
{
    kBox,
    kTransform,
};


struct GizmoProgram
{
    GizmoProgram();

    void Draw(GizmoDesc const &_desc, unsigned _id, Matrix_t const& _projection) const;

    oglbase::ProgramPtr shader_program_;
    oglbase::VAOPtr dummy_vao_;
};

struct GizmoLayer
{
    GizmoLayer(Matrix_t const& _projection) :
        projection_{ _projection }
    {}

    void RenderFrame() const;

    Matrix_t projection_;
    std::vector<GizmoDesc> gizmos_;
    GizmoProgram renderable_;
};


} // namespace uibase


#endif // __YS_GIZMOLAYER_HPP__
