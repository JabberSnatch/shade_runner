/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. You can do whatever you want with this
 * stuff. If we meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma once
#ifndef __YS_IMGUILAYER_HPP__
#define __YS_IMGUILAYER_HPP__

#include "utility/callback.h"
#include "uibase/imguicontext.h"
#include "shaderunner/shaderunner.h"
#include "appbase/state.h"

namespace appbase {

struct ImGuiLayer
{
    ImGuiLayer();
    void RunFrame(State const&_state);

    static constexpr std::size_t kPathMaxLength = 512u;
    static constexpr std::size_t kUniformMaxLength = 32u;

    bool show_demo_window = true;
    bool show_main_window = true;

    char fkernel_path_buffer[kPathMaxLength] = "";
    utility::Query<std::string> FKernelPath_query;
    utility::Callback<std::string const&> FKernelPath_onReturn;

    utility::Query<sr::UniformContainer> Uniforms_query;
    utility::Callback<sr::UniformContainer const&> Uniforms_onReturn;

    std::string error_console_buffer;
    void onFKernelCompileFinished(std::string const&_path, sr::ErrorLogContainer const&_errorlog);

    uibase::ImGuiContext imgui_context_;
};


} // namespace appbase


#endif // __YS_IMGUILAYER_HPP__
