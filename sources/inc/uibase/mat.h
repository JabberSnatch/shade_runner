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
using Mat4_t = std::array<float, 16>;

inline Mat4_t perspective(float n, float f, float alpha, float how)
{
    const float inv_tan_half_alpha = 1.f / std::tan(alpha * .5f);
    const float inv_fmn = 1.f / (f-n);
    return Mat4_t{
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
vec3_from_vec4(Vec4_t const& _o) {
    return Vec3_t{ _o[0], _o[1], _o[2] };
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

inline Vec3_t
vec3_cwise_mul(Vec3_t const& _lhs, Vec3_t const& _rhs) {
    return Vec3_t{ _lhs[0]*_rhs[0], _lhs[1]*_rhs[1], _lhs[2]*_rhs[2] };
}

inline float
vec3_dot(Vec3_t const& _lhs, Vec3_t const& _rhs) {
    return _lhs[0]*_rhs[0] + _lhs[1]*_rhs[1] + _lhs[2]*_rhs[2];
}

inline Vec3_t
vec3_cross(Vec3_t const& _lhs, Vec3_t const& _rhs) {
    return Vec3_t{
        _lhs[1]*_rhs[2] - _lhs[2]*_rhs[1],
        _lhs[2]*_rhs[0] - _lhs[0]*_rhs[2],
        _lhs[0]*_rhs[1] - _lhs[1]*_rhs[0]
    };
}

inline Vec3_t
vec3_normalise(Vec3_t const& _op) {
    return vec3_float_mul(_op, 1.f/sqrt(vec3_dot(_op, _op)));
}

inline Vec4_t
vec3_float_concat(Vec3_t const& _lhs, float const& _rhs)
{
    return Vec4_t{ _lhs[0], _lhs[1], _lhs[2], _rhs };
}

inline Vec2_t
vec2_itof(Vec2i_t const& _in) {
    return Vec2_t{ (float)_in[0], (float)_in[1] };
}

inline Vec4_t
vec4_from_vec3(Vec3_t const& _lhs, float _rhs) {
    return Vec4_t{ _lhs[0], _lhs[1], _lhs[2], _rhs };
}

inline float
vec4_dot(Vec4_t const& _lhs, Vec4_t const& _rhs)
{
    return _lhs[0]*_rhs[0] + _lhs[1]*_rhs[1] + _lhs[2]*_rhs[2] + _lhs[3]*_rhs[3];
}

inline Mat4_t
mat4_col(Vec4_t const& _c0, Vec4_t const& _c1, Vec4_t const& _c2, Vec4_t const& _c3)
{
    return Mat4_t{
        _c0[0], _c0[1], _c0[2], _c0[3],
        _c1[0], _c1[1], _c1[2], _c1[3],
        _c2[0], _c2[1], _c2[2], _c2[3],
        _c3[0], _c3[1], _c3[2], _c3[3]
        };
}

inline Vec4_t
mat4_vec4_mul(Mat4_t const& _lhs, Vec4_t const& _rhs)
{
    return Vec4_t{
        _lhs[0]*_rhs[0] + _lhs[4]*_rhs[1] + _lhs[8]*_rhs[2] + _lhs[12]*_rhs[3],
        _lhs[1]*_rhs[0] + _lhs[5]*_rhs[1] + _lhs[9]*_rhs[2] + _lhs[13]*_rhs[3],
        _lhs[2]*_rhs[0] + _lhs[6]*_rhs[1] + _lhs[10]*_rhs[2] + _lhs[14]*_rhs[3],
        _lhs[3]*_rhs[0] + _lhs[7]*_rhs[1] + _lhs[11]*_rhs[2] + _lhs[15]*_rhs[3]
    };
}

inline Mat4_t
mat4_transpose(Mat4_t const& _op)
{
    return Mat4_t{
        _op[0], _op[4], _op[8], _op[12],
        _op[1], _op[5], _op[9], _op[13],
        _op[2], _op[6], _op[10], _op[14],
        _op[3], _op[7], _op[11], _op[15]
    };
}

inline Mat4_t
mat4_mul(Mat4_t const& _lhs, Mat4_t const& _rhs)
{
    Mat4_t const lt = mat4_transpose(_lhs);
    Vec4_t const& _lc0 = *(Vec4_t*)&lt;
    Vec4_t const& _lc1 = *(((Vec4_t*)&lt) + 1);
    Vec4_t const& _lc2 = *(((Vec4_t*)&lt) + 2);
    Vec4_t const& _lc3 = *(((Vec4_t*)&lt) + 3);

    Vec4_t const& _rc0 = *(Vec4_t*)&_rhs;
    Vec4_t const& _rc1 = *(((Vec4_t*)&_rhs) + 1);
    Vec4_t const& _rc2 = *(((Vec4_t*)&_rhs) + 2);
    Vec4_t const& _rc3 = *(((Vec4_t*)&_rhs) + 3);

    return mat4_col(
        Vec4_t{vec4_dot(_lc0, _rc0), vec4_dot(_lc1, _rc0), vec4_dot(_lc2, _rc0), vec4_dot(_lc3, _rc0)},
        Vec4_t{vec4_dot(_lc0, _rc1), vec4_dot(_lc1, _rc1), vec4_dot(_lc2, _rc1), vec4_dot(_lc3, _rc1)},
        Vec4_t{vec4_dot(_lc0, _rc2), vec4_dot(_lc1, _rc2), vec4_dot(_lc2, _rc2), vec4_dot(_lc3, _rc2)},
        Vec4_t{vec4_dot(_lc0, _rc3), vec4_dot(_lc1, _rc3), vec4_dot(_lc2, _rc3), vec4_dot(_lc3, _rc3)}
    );
}

inline Mat4_t
mat4_rot(Vec3_t const& _eulerDeg)
{
    Vec3_t er = vec3_float_mul(_eulerDeg, 3.1415926536f/180.f);

    Mat4_t x{
        1.f, 0.f, 0.f, 0.f,
        0.f, std::cos(er[0]), std::sin(er[0]), 0.f,
        0.f, -std::sin(er[0]), std::cos(er[0]), 0.f,
        0.f, 0.f, 0.f, 1.f
    };
    Mat4_t y{
        std::cos(er[1]), 0.f, -std::sin(er[1]), 0.f,
        0.f, 1.f, 0.f, 0.f,
        std::sin(er[1]), 0.f, std::cos(er[1]), 0.f,
        0.f, 0.f, 0.f, 1.f
    };
    Mat4_t z{
        std::cos(er[2]), std::sin(er[2]), 0.f, 0.f,
        -std::sin(er[2]), std::cos(er[2]), 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };

    return mat4_mul(y, mat4_mul(x, z));
}

} // namespace uibase

#endif

