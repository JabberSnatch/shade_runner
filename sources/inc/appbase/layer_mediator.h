/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. You can do whatever you want with this
 * stuff. If we meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma once
#ifndef __YS_LAYERMEDIATOR_HPP__
#define __YS_LAYERMEDIATOR_HPP__

#include <cstdint>
#include <memory>

#include "oglbase/framebuffer.h"
#include "shaderunner/shaderunner.h"
#include "uibase/gizmo_layer.h"
#include "appbase/state.h"
#include "appbase/imgui_layer.h"

namespace appbase {

enum LayerFlag : unsigned
{
    kShaderunner = 1u << 0,
    kGizmo = 1u << 1,
    kImgui = 1u << 2,
};

struct LayerMediator
{
    LayerMediator(uibase::Vec2i_t const& _screen_size, unsigned _flags);

    void SpecialKey(eKey _key, std::uint8_t _v);

    void ResizeEvent(uibase::Vec2i_t const& _size);
    void MouseDown(bool _v);
    void MousePos(uibase::Vec2i_t const& _pos);
    void KeyDown(std::uint32_t _key, std::uint32_t _mod, bool _v);
    bool RunFrame(float _dt);

    State state_;
    State back_state_;

    std::unique_ptr<sr::RenderContext> sr_layer_;
    std::unique_ptr<uibase::GizmoLayer> gizmo_layer_;
    std::unique_ptr<appbase::ImGuiLayer> imgui_layer_;
    std::unique_ptr<oglbase::Framebuffer> framebuffer_;
};

} // namespace appbase

#endif // __YS_LAYERMEDIATOR_HPP__
