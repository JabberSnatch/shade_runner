/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma once
#ifndef __YS_SHADERUNNER_HPP__
#define __YS_SHADERUNNER_HPP__

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "shaderunner/shader_cache.h"

#include "utility/callback.h"

namespace sr {

using UniformContainer = std::vector<std::pair<std::string, float>>;
using ErrorLogContainer = std::vector<std::pair<int, std::string>>;

using Mat4_t = std::array<float, 16>;
using Vec3_t = std::array<float, 3>;

class RenderContext
{
public:
    static constexpr unsigned kGizmoCountMax = 16u;
public:
	RenderContext();
	~RenderContext();

	bool RenderFrame();
	void WatchKernelFile(ShaderStage _stage, char const *_path);
	void SetResolution(int _width, int _height);

    void SetUniforms(UniformContainer const&_uniforms);

    UniformContainer const &GetUniforms() const;
	std::string const &GetKernelPath(ShaderStage _stage) const;

    utility::Callback<std::string const&, ErrorLogContainer const&> onFKernelCompileFinished;

    Mat4_t projection_matrix{ 1.f, 0.f, 0.f, 0.f,
                              0.f, 1.f, 0.f, 0.f,
                              0.f, 0.f, 1.f, 0.f,
                              0.f, 0.f, 0.f, 1.f };
    int gizmo_count = 0;
    Vec3_t gizmo_positions[kGizmoCountMax];

private:
	struct Impl_;
	Impl_* impl_;
};

} // namespace sr

extern "C"
{

    typedef void* hContext;
    struct FrameDesc
    {
        float res[2];

        char const** unames;
        float* uvalues;
        int ucount;

        float projection_matrix[16];
        int gizmo_count;
        float gizmo_positions[3*16];
    };

    void* srCreateContext();
    void srDeleteContext(void* context);
    bool srRenderFrame(void* context, FrameDesc const* desc);
    void srWatchKernelFile(void* context, std::uint32_t stage, char const* path);
    char const* srGetKernelPath(void* context, std::uint32_t stage);

}

#endif // __YS_SHADERUNNER_HPP__
