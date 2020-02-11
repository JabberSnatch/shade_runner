/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. You can do whatever you want with this
 * stuff. If we meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "appbase/imgui_layer.h"

#include <imgui.h>

#include "utility/file.h"

namespace appbase
{

ImGuiLayer::ImGuiLayer()
{
    ImGuiIO &io = ImGui::GetIO();

    io.KeyMap[ImGuiKey_Tab] = eKey::kTab;
    io.KeyMap[ImGuiKey_LeftArrow] = eKey::kLeft;
    io.KeyMap[ImGuiKey_RightArrow] = eKey::kRight;
    io.KeyMap[ImGuiKey_UpArrow] = eKey::kUp;
    io.KeyMap[ImGuiKey_DownArrow] = eKey::kDown;
    io.KeyMap[ImGuiKey_PageUp] = eKey::kPageUp;
    io.KeyMap[ImGuiKey_PageDown] = eKey::kPageDown;
    io.KeyMap[ImGuiKey_Home] = eKey::kHome;
    io.KeyMap[ImGuiKey_End] = eKey::kEnd;
    io.KeyMap[ImGuiKey_Insert] = eKey::kInsert;
    io.KeyMap[ImGuiKey_Delete] = eKey::kDelete;
    io.KeyMap[ImGuiKey_Backspace] = eKey::kBackspace;
    io.KeyMap[ImGuiKey_Space] = ' ';
    io.KeyMap[ImGuiKey_Enter] = eKey::kEnter;
    io.KeyMap[ImGuiKey_Escape] = eKey::kEscape;
    io.KeyMap[ImGuiKey_A] = 'a';
    io.KeyMap[ImGuiKey_C] = 'c';
    io.KeyMap[ImGuiKey_V] = 'v';
    io.KeyMap[ImGuiKey_X] = 'x';
    io.KeyMap[ImGuiKey_Y] = 'y';
    io.KeyMap[ImGuiKey_Z] = 'z';

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
}

void
ImGuiLayer::RunFrame(State const&_state)
{
    ImGuiIO &io = ImGui::GetIO();

    io.KeyCtrl = _state.mod_down & fKeyMod::kCtrl;
    io.KeyShift = _state.mod_down & fKeyMod::kShift;
    io.KeyAlt = _state.mod_down & fKeyMod::kAlt;
    io.KeySuper = false;

    io.MouseDown[0] = _state.mouse_down;
    io.MousePos = ImVec2(static_cast<float>(_state.mouse_pos[0]),
                         static_cast<float>(_state.screen_size[1] - _state.mouse_pos[1]));

    {
        ImGuiStyle &style = ImGui::GetStyle();
        style.FrameRounding = 0.f;
        style.WindowRounding = 1.f;
        style.ScrollbarRounding = 0.f;
        style.GrabRounding = 2.f;
    }

    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Windows"))
        {
            ImGui::MenuItem("Demo", nullptr, &show_demo_window);
            ImGui::MenuItem("Main", nullptr, &show_main_window);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (false && show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    if (show_main_window)
    {
        ImGui::SetNextWindowPos(ImVec2(350, 20), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Main Window", &show_main_window, 0))
        {
            {
                std::string fkernel_path = FKernelPath_query();
                assert(fkernel_path.size() <= kPathMaxLength);
                std::copy(fkernel_path.cbegin(), fkernel_path.cend(), &fkernel_path_buffer[0]);

                ImGui::PushItemWidth(-1);
                bool const state = ImGui::InputText("IT_fkernel_path",
                                                    fkernel_path_buffer,
                                                    kPathMaxLength,
                                                    ImGuiInputTextFlags_EnterReturnsTrue);
                ImGui::PopItemWidth();

                if (state)
                    FKernelPath_onReturn(std::string(fkernel_path_buffer, kPathMaxLength));
            }

            if (ImGui::CollapsingHeader("Uniforms"))
            {
                sr::UniformContainer uniforms = Uniforms_query();
                if (ImGui::Button("+"))
                {
                    uniforms.emplace_back(std::pair<std::string, float>{"", 0.f});
                } ImGui::SameLine();

                if (ImGui::Button("-"))
                    ;

                { ImGui::PushItemWidth(-100);
                    std::array<char, kUniformMaxLength> buff;

                    for (std::size_t i = 0; i < uniforms.size(); ++i)
                    {
                        std::pair<std::string, float> &uniform = uniforms[i];

                        std::size_t bufflength = (kUniformMaxLength < uniform.first.size())
                            ? kUniformMaxLength
                            : uniform.first.size();

                        std::copy(uniform.first.c_str(),
                                  uniform.first.c_str() + bufflength,
                                  std::begin(buff));

                        std::fill(std::begin(buff) + bufflength,
                                  std::end(buff),
                                  '\0');

                        ImGui::DragFloat(
                            (std::string("DF_uniform") + std::to_string(i)).c_str(),
                            &uniform.second);

                        if (ImGui::InputText(
                                (std::string("IT_uniform") + std::to_string(i)).c_str(),
                                buff.data(),
                                kUniformMaxLength,
                                ImGuiInputTextFlags_EnterReturnsTrue))
                        {
                            uniform.first = std::string(buff.data(), kUniformMaxLength);
                        }
                    }
                } ImGui::PopItemWidth();

                Uniforms_onReturn(uniforms);
            }

            if (ImGui::CollapsingHeader("Compile Errors"))
                ImGui::TextWrapped(error_console_buffer.c_str(), 0);

            if (ImGui::CollapsingHeader("Debug"))
            {
                for (int i = 0; i < 256; ++i)
                {
                    bool b0 = _state.key_down[i];
                    ImGui::Checkbox((std::string("key") + std::to_string(i)).c_str(), &b0);
                    if (i % 16 != 0) ImGui::SameLine();
                }
            }
        }
        ImGui::End();
    }

    ImGui::EndFrame();
    ImGui::Render();

    imgui_context_.Render();
}


void
ImGuiLayer::onFKernelCompileFinished(std::string const&_path, sr::ErrorLogContainer const&_errorlog)
{
    error_console_buffer = "";
    utility::File kernel_file(_path);

    std::string work_data = kernel_file.ReadAll();
    auto it = _errorlog.begin();
    for (int i = 1; it != _errorlog.end(); ++i)
    {
        while (it->first == i)
        {
            std::size_t index = work_data.find('\n');
            std::string line = work_data.substr(0, index);
            index = line.find_first_not_of("\t ");
            line = line.substr(index);
            error_console_buffer += std::to_string(it->first) + " " + line + "\n" + it->second + "\n";
            ++it;
        }

        std::size_t index = work_data.find('\n');
        work_data = work_data.substr(index+1);
    }
}

} // namespace appbase
