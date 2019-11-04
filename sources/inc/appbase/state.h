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

namespace appbase {

using Vec2i_t = std::array<int, 2>;

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
    Vec2i_t screen_size{ 0, 0 };
    bool mouse_down{ false };
    Vec2i_t mouse_pos{ -1, -1 };
    std::array<bool, 256> key_down;
    std::uint32_t mod_down;
    int active_gizmo{ 0 };

    std::array<eKey, 256> key_map;
};

} // namespace appbase


#endif // __YS_STATE_HPP__
