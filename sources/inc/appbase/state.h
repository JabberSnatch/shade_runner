/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. You can do whatever you want with this
 * stuff. If we meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma once
#ifndef __YS_STATE_HPP__
#define __YS_STATE_HPP__

#include <array>
#include <cstdint>

#include "uibase/mat.h"

namespace appbase {

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

struct State {
    uibase::Vec2i_t screen_size{ 0, 0 };
    bool mouse_down{ false };
    uibase::Vec2i_t mouse_pos{ -1, -1 };
    uibase::Vec2i_t mouse_delta{ 0, 0 };
    std::array<bool, 256> key_down;

    std::uint32_t mod_down;

    std::array<eKey, 256> key_map;

    bool enable_gizmos = false;
    std::uint32_t hover_gizmo = 0u;
    std::uint32_t select_gizmo = 0u;

    uibase::Vec3_t camera_position{0.f, 0.f, 0.f};
    uibase::Vec3_t camera_rotation{0.f, 0.f, 0.f};
    bool camera_enable_mouse_control = false;
};

} // namespace appbase


#endif // __YS_STATE_HPP__
