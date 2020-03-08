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

using Vec4_t = std::array<float, 4>;
using Vec3_t = std::array<float, 3>;
using Vec2_t = std::array<float, 2>;
using Vec2i_t = std::array<int, 2>;
using Matrix_t = std::array<float, 16>;

inline Matrix_t perspective(float n, float f, float alpha, float how)
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

inline Vec3_t
vec3_sub(Vec3_t const& _lhs, Vec3_t const& _rhs) {
    return Vec3_t{ _lhs[0]-_rhs[0], _lhs[1]-_rhs[1], _lhs[2]-_rhs[2] };
}

inline Vec3_t
vec3_add(Vec3_t const& _lhs, Vec3_t const& _rhs) {
    return Vec3_t{ _lhs[0]+_rhs[0], _lhs[1]+_rhs[1], _lhs[2]+_rhs[2] };
}

inline Vec3_t
vec3_float_mul(Vec3_t const& _lhs, float _rhs) {
    return Vec3_t{ _lhs[0]*_rhs, _lhs[1]*_rhs, _lhs[2]*_rhs };
}

inline Vec2_t
vec2_itof(Vec2i_t const& _in) {
    return Vec2_t{ (float)_in[0], (float)_in[1] };
}

inline Vec4_t
mat4_vec4_mul(Matrix_t const& _lhs, Vec4_t const& _rhs)
{
    return Vec4_t{
        _lhs[0]*_rhs[0] + _lhs[4]*_rhs[1] + _lhs[8]*_rhs[2] + _lhs[12]*_rhs[3],
        _lhs[1]*_rhs[0] + _lhs[5]*_rhs[1] + _lhs[9]*_rhs[2] + _lhs[13]*_rhs[3],
        _lhs[2]*_rhs[0] + _lhs[6]*_rhs[1] + _lhs[10]*_rhs[2] + _lhs[14]*_rhs[3],
        _lhs[3]*_rhs[0] + _lhs[7]*_rhs[1] + _lhs[11]*_rhs[2] + _lhs[15]*_rhs[3]
    };
}

} // namespace uibase

#endif

