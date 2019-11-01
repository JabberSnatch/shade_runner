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

#include <array>
#include <memory>

#include "oglbase/framebuffer.h"
#include "shaderunner/shaderunner.h"
#include "uibase/gizmo_layer.h"
#include "uibase/imguicontext.h"

namespace appbase {

using Vec2i_t = std::array<int, 2>;

enum LayerFlag : unsigned
{
    kNone = 0u,
    kShaderunner = 1u << 0,
    kGizmo = 1u << 1,
    kImgui = 1u << 2,
};

struct LayerMediator
{
    struct State {
        Vec2i_t screen_size{ 0, 0 };
        bool mouse_down{ false };
        Vec2i_t mouse_pos{ -1, -1 };
        int active_gizmo{ 0 };
    };

    LayerMediator(Vec2i_t const& _screen_size, unsigned _flags);

    void ResizeEvent(Vec2i_t const& _size);
    void MouseDown(bool _v);
    void MousePos(Vec2i_t const& _pos);
    bool RunFrame();

    State state_;
    std::unique_ptr<sr::RenderContext> sr_layer_;
    std::unique_ptr<uibase::GizmoLayer> gizmo_layer_;
    std::unique_ptr<uibase::ImGuiContext> imgui_layer_;
    std::unique_ptr<oglbase::Framebuffer> framebuffer_;
};

} // namespace appbase

#endif // __YS_LAYERMEDIATOR_HPP__
