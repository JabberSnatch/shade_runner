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

#include <vector>

#include "uibase/mat.h"

#include "utility/callback.h"

#include "oglbase/handle.h"
#include "oglbase/shader.h"

namespace uibase {

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

inline std::uint32_t UnpackGizmoIndex(std::uint32_t _payload) { return _payload & 0xffffffu; }
inline std::uint32_t UnpackSubgizmoIndex(std::uint32_t _payload) { return (_payload >> 24) & 0xffu; }

struct GizmoProgram
{
    GizmoProgram(oglbase::ShaderSources_t const& _geom_sources);

    void Draw(GizmoDesc const &_desc, unsigned _id, Matrix_t const& _projection) const;

    oglbase::ProgramPtr shader_program_;
    oglbase::VAOPtr dummy_vao_;
};

struct GizmoLayer
{
    GizmoLayer(Matrix_t const& _projection);

    void ClearIDBuffer() const;
    void RenderFrame() const;

    GizmoDesc& GetGizmo(std::uint32_t _gizmo_index);
    utility::Callback<GizmoDesc const&> Gizmos_onMove;
    std::vector<GizmoDesc> gizmos_;

    Matrix_t projection_;
    GizmoProgram box_program_;
    GizmoProgram transfo_program_;
};


} // namespace uibase


#endif // __YS_GIZMOLAYER_HPP__
