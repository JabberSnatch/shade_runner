/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. You can do whatever you want with this
 * stuff. If we meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "appbase/layer_mediator.h"

#include <imgui.h>

#include "oglbase/framebuffer.h"
#include "shaderunner/shaderunner.h"
#include "uibase/gizmo_layer.h"
#include "utility/file.h"

namespace {

static uibase::Vec3_t const kGizmoColorOff{ 0.f, 1.f, 0.f };
static uibase::Vec3_t const kGizmoColorOn{ 1.f, 0.f, 0.f };

std::unique_ptr<oglbase::Framebuffer> MakeGizmoLayerFramebuffer(appbase::Vec2i_t const& _screen_size);
uibase::Matrix_t MakeGizmoLayerProjection(appbase::Vec2i_t const& _screen_size);

void ImGui_Config(appbase::LayerMediator &_lm);

static std::string error_console_buffer;
void onCompileFailed(std::string const&_path, sr::ErrorLogContainer const&_errorlog)
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
};

} // namespace

namespace appbase {

LayerMediator::LayerMediator(Vec2i_t const& _screen_size, unsigned _flags)
{
    state_.screen_size = _screen_size;
    std::fill(std::begin(state_.key_down), std::end(state_.key_down), false);
    std::fill(std::begin(state_.key_map), std::end(state_.key_map), (eKey)0);

    if (_flags & LayerFlag::kShaderunner)
    {
        sr_layer_ = std::make_unique<sr::RenderContext>();
        sr_layer_->SetResolution(state_.screen_size[0], state_.screen_size[1]);

        sr_layer_->onCompileFailed_callback.listeners_.emplace_back(&onCompileFailed);
    }
    if (_flags & LayerFlag::kGizmo)
    {
        framebuffer_ = MakeGizmoLayerFramebuffer(state_.screen_size);
        gizmo_layer_ = std::make_unique<uibase::GizmoLayer>(MakeGizmoLayerProjection(state_.screen_size));

        for (float i = 0.f; i < 10.f; i+=1.f)
            for (float j = 0.f; j < 10.f; j+=1.f)
                for (float k = 0.f; k < 10.f; k+=1.f)
                    gizmo_layer_->gizmos_.push_back(
                        uibase::GizmoDesc{ uibase::Vec3_t{ i - 5.f, j - 5.f, -k }, kGizmoColorOff });
    }
    if (_flags & LayerFlag::kImgui)
    {
        imgui_layer_ = std::make_unique<uibase::ImGuiContext>();
        imgui_layer_->SetResolution(state_.screen_size[0], state_.screen_size[1]);

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
}

void
LayerMediator::SpecialKey(eKey _key, std::uint8_t _v)
{
    state_.key_map[_v] = _key;
}

void
LayerMediator::ResizeEvent(Vec2i_t const& _size)
{
    if (_size[0] == state_.screen_size[0] && _size[1] == state_.screen_size[1])
        return;

    state_.screen_size = _size;

    if (sr_layer_)
    {
        sr_layer_->SetResolution(state_.screen_size[0], state_.screen_size[1]);
    }

    if (gizmo_layer_)
    {
        framebuffer_ = MakeGizmoLayerFramebuffer(state_.screen_size);
        gizmo_layer_->projection_ = MakeGizmoLayerProjection(state_.screen_size);
    }

    if (imgui_layer_)
    {
        imgui_layer_->SetResolution(state_.screen_size[0], state_.screen_size[1]);
    }
}

void
LayerMediator::MouseDown(bool _v)
{
    if (state_.mouse_down == _v)
        return;

    state_.mouse_down = _v;

    if (gizmo_layer_ && !_v)
    {
        if (state_.active_gizmo)
            gizmo_layer_->gizmos_[state_.active_gizmo-1].color_ = kGizmoColorOff;
        state_.active_gizmo = 0;
    }
}

void
LayerMediator::MousePos(Vec2i_t const& _pos)
{
    if (state_.mouse_pos[0] == _pos[0] && state_.mouse_pos[1] == _pos[1])
        return;

    state_.mouse_pos = _pos;

    if (gizmo_layer_)
    {
        // =====================================================================
        // framebuffer_->ReadPixel() ?
        framebuffer_->Bind();
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        int gizmo_id = 0;
        static_assert(sizeof(int) == sizeof(float), "");
        glReadPixels(state_.mouse_pos[0], state_.mouse_pos[1],
                     1, 1,
                     GL_RED, GL_FLOAT, (GLfloat*)&gizmo_id);
        glReadBuffer(GL_NONE);
        framebuffer_->Unbind();
        // =====================================================================

        if (gizmo_id != state_.active_gizmo)
        {
            if (state_.active_gizmo)
                gizmo_layer_->gizmos_[state_.active_gizmo-1].color_ = kGizmoColorOff;
            state_.active_gizmo = gizmo_id;
            if (state_.active_gizmo)
                gizmo_layer_->gizmos_[state_.active_gizmo-1].color_ = kGizmoColorOn;
        }
    }
}

void
LayerMediator::KeyDown(std::uint32_t _key, std::uint32_t _mod, bool _v)
{
    if (_key < eKey::kASCIIBegin || _key >= eKey::kASCIIEnd)
        _key = state_.key_map[_key & 0xff];

    state_.mod_down = _mod;

    if (state_.key_down[_key] == _v)
        return;
    state_.key_down[_key] = _v;

    if (imgui_layer_)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.KeysDown[_key] = _v;

        if (_v && _key >= eKey::kASCIIBegin && _key < eKey::kASCIIEnd)
            io.AddInputCharacter((ImWchar)_key);
    }
}

bool
LayerMediator::RunFrame()
{
    bool result = false;

    if (gizmo_layer_)
    {
        framebuffer_->Bind();
    }

    if (sr_layer_)
        result = sr_layer_->RenderFrame();

    if (gizmo_layer_)
    {
        gizmo_layer_->RenderFrame();
        framebuffer_->Unbind();

        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_->fbo_);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK);
        glBlitFramebuffer(0, 0, state_.screen_size[0], state_.screen_size[1],
                          0, 0, state_.screen_size[0], state_.screen_size[1],
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    if (imgui_layer_)
    {
        ImGuiIO &io = ImGui::GetIO();

        io.KeyCtrl = state_.mod_down & fKeyMod::kCtrl;
        io.KeyShift = state_.mod_down & fKeyMod::kShift;
        io.KeyAlt = state_.mod_down & fKeyMod::kAlt;
        io.KeySuper = false;

        io.MouseDown[0] = state_.mouse_down;
        io.MousePos = ImVec2(static_cast<float>(state_.mouse_pos[0]),
                             static_cast<float>(state_.screen_size[1] - state_.mouse_pos[1]));

        {
            ImGuiStyle &style = ImGui::GetStyle();
            style.FrameRounding = 0.f;
            style.WindowRounding = 1.f;
            style.ScrollbarRounding = 0.f;
            style.GrabRounding = 2.f;
        }

        ImGui_Config(*this);

        imgui_layer_->Render();
    }

    return result;
}

} // namespace appbase

namespace {

std::unique_ptr<oglbase::Framebuffer>
MakeGizmoLayerFramebuffer(appbase::Vec2i_t const& _screen_size)
{
    oglbase::Framebuffer::AttachmentDescs const fbo_attachments{
        { GL_COLOR_ATTACHMENT0, GL_RGBA8 },
        { GL_COLOR_ATTACHMENT1, GL_R32F }
    };

    return std::make_unique<oglbase::Framebuffer>(
        _screen_size[0], _screen_size[1], fbo_attachments, true
    );
}

uibase::Matrix_t
MakeGizmoLayerProjection(appbase::Vec2i_t const& _screen_size)
{
    float aspect_ratio = static_cast<float>(_screen_size[1]) / static_cast<float>(_screen_size[0]);
    return uibase::perspective(0.01f, 1000.f, 3.1415926534f*0.5f, aspect_ratio);
}

void ImGui_Config(appbase::LayerMediator &_lm)
{
    constexpr int kTextBufferSize = 512;

    static bool show_demo_window = true;
    static bool show_main_window = true;

    static char fkernel_path_buffer[kTextBufferSize] = "";
    static char stub1[kTextBufferSize] = "";

    constexpr std::size_t kUniformMaxLength = 32;

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

    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    if (show_main_window)
    {
        ImGui::SetNextWindowPos(ImVec2(350, 20), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Main Window", &show_main_window, 0))
        {
            {
                std::string const &fkernel_path = _lm.sr_layer_->GetKernelPath(sr::ShaderStage::kFragment);
                assert(fkernel_path.size() <= kTextBufferSize);
                std::copy(fkernel_path.cbegin(), fkernel_path.cend(), &fkernel_path_buffer[0]);

                ImGui::PushItemWidth(-1);
                bool const state = ImGui::InputText("IT_fkernel_path",
                                                    fkernel_path_buffer,
                                                    kTextBufferSize,
                                                    ImGuiInputTextFlags_EnterReturnsTrue);
                ImGui::PopItemWidth();

                if (state)
                    _lm.sr_layer_->WatchKernelFile(sr::ShaderStage::kFragment, fkernel_path_buffer);
            }

            if (ImGui::CollapsingHeader("Uniforms"))
            {
                sr::UniformContainer uniforms = _lm.sr_layer_->GetUniforms();
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

                        std::copy(uniform.first.c_str(),
                                  uniform.first.c_str() + std::min(kUniformMaxLength, uniform.first.size()),
                                  std::begin(buff));

                        std::fill(std::begin(buff) + std::min(kUniformMaxLength, uniform.first.size()),
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

                _lm.sr_layer_->SetUniforms(std::move(uniforms));
            }

            if (ImGui::CollapsingHeader("Compile Errors"))
                ImGui::TextWrapped(error_console_buffer.c_str(), 0);

            if (ImGui::CollapsingHeader("Debug"))
            {
                for (int i = 0; i < 256; ++i)
                {
                    ImGui::Checkbox((std::string("key") + std::to_string(i)).c_str(), &_lm.state_.key_down[i]);
                    if (i % 16 != 0) ImGui::SameLine();
                }
            }
        }
        ImGui::End();
    }

    ImGui::EndFrame();
    ImGui::Render();
}

} // namespace
