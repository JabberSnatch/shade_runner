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
#include <cstdint>
#include <memory>

#include "oglbase/framebuffer.h"
#include "shaderunner/shaderunner.h"
#include "uibase/gizmo_layer.h"
#include "uibase/imguicontext.h"

namespace appbase {

using Vec2i_t = std::array<int, 2>;

enum LayerFlag : unsigned
{
    kShaderunner = 1u << 0,
    kGizmo = 1u << 1,
    kImgui = 1u << 2,
};

enum eKey
{
    kSpecialBegin = 1u,
    kTab = kSpecialBegin,
    kLeft,
    kRight,
    kUp,
    kDown,
    kPageUp,
    kPageDown,
    kHome,
    kEnd,
    kInsert,
    kDelete,
    kBackspace,
    kEnter,
    kEscape,
    kSpecialEnd,

    kASCIIBegin = 0x20,
    kASCIIEnd = 0x7f,
};

enum fKeyMod
{
    kCtrl = 1u << 0,
    kShift = 1u << 1,
    kAlt = 1u << 2
};

struct LayerMediator
{
    struct State {
        Vec2i_t screen_size{ 0, 0 };
        bool mouse_down{ false };
        Vec2i_t mouse_pos{ -1, -1 };
        std::array<bool, 256> key_down;
        std::uint32_t mod_down;
        int active_gizmo{ 0 };

        std::array<eKey, 256> key_map;
    };

    LayerMediator(Vec2i_t const& _screen_size, unsigned _flags);

    void SpecialKey(eKey _key, std::uint8_t _v);

    void ResizeEvent(Vec2i_t const& _size);
    void MouseDown(bool _v);
    void MousePos(Vec2i_t const& _pos);
    void KeyDown(std::uint32_t _key, std::uint32_t _mod, bool _v);
    bool RunFrame();

    State state_;
    std::unique_ptr<sr::RenderContext> sr_layer_;
    std::unique_ptr<uibase::GizmoLayer> gizmo_layer_;
    std::unique_ptr<uibase::ImGuiContext> imgui_layer_;
    std::unique_ptr<oglbase::Framebuffer> framebuffer_;
};

} // namespace appbase

#endif // __YS_LAYERMEDIATOR_HPP__
