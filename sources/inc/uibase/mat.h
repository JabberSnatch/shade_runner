/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. You can do whatever you want with this
 * stuff. If we meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma once
#ifndef __YS_MAT_HPP__
#define __YS_MAT_HPP__

#include <array>
#include <cmath>


namespace uibase {

using Vec3_t = std::array<float, 3>;
using Vec2_t = std::array<float, 2>;
using Vec2i_t = std::array<int, 2>;
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

inline Vec2i_t
vec2i_sub(Vec2i_t const& _lhs, Vec2i_t const&_rhs) {
    return Vec2i_t{ _lhs[0] - _rhs[0], _lhs[1] - _rhs[1] };
}

inline Vec2_t
vec2_itof(Vec2i_t const& _in) {
    return Vec2_t{ (float)_in[0], (float)_in[1] };
}

} // namespace uibase

#endif

