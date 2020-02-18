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

#include <iostream>

namespace {

static uibase::Vec3_t const kGizmoColorOff{ 0.f, 1.f, 0.f };
static uibase::Vec3_t const kGizmoColorOn{ 1.f, 0.f, 0.f };

std::unique_ptr<oglbase::Framebuffer> MakeGizmoLayerFramebuffer(uibase::Vec2i_t const& _screen_size);
uibase::Matrix_t MakeGizmoLayerProjection(uibase::Vec2i_t const& _screen_size);

} // namespace

namespace appbase {

LayerMediator::LayerMediator(uibase::Vec2i_t const& _screen_size, unsigned _flags)
{
    state_.screen_size = _screen_size;
    std::fill(std::begin(state_.key_down), std::end(state_.key_down), false);
    std::fill(std::begin(state_.key_map), std::end(state_.key_map), (eKey)0);

    if (_flags & LayerFlag::kShaderunner)
    {
        sr_layer_ = std::make_unique<sr::RenderContext>();
        sr_layer_->SetResolution(state_.screen_size[0], state_.screen_size[1]);
    }
    if (_flags & LayerFlag::kGizmo)
    {
        framebuffer_ = MakeGizmoLayerFramebuffer(state_.screen_size);
        gizmo_layer_ = std::make_unique<uibase::GizmoLayer>(MakeGizmoLayerProjection(state_.screen_size));

#if 0
        for (float i = 0.f; i < 10.f; i+=1.f)
            for (float j = 0.f; j < 10.f; j+=1.f)
                for (float k = 0.f; k < 10.f; k+=1.f)
                    gizmo_layer_->gizmos_.push_back(
                        uibase::GizmoDesc{ uibase::eGizmoType::kBox,
                                uibase::Vec3_t{ i - 5.f, j - 5.f, -k },
                                kGizmoColorOff }
                    );
#endif

        gizmo_layer_->gizmos_.push_back(
            uibase::GizmoDesc{ uibase::eGizmoType::kTransform,
                    uibase::Vec3_t{ 0.f, 0.f, -3.f },
                    kGizmoColorOff }
        );
    }
    if (_flags & LayerFlag::kImgui)
    {
        imgui_layer_ = std::make_unique<appbase::ImGuiLayer>();
        imgui_layer_->imgui_context_.SetResolution(state_.screen_size[0], state_.screen_size[1]);
    }

    if (sr_layer_ && imgui_layer_)
    {
        sr_layer_->onFKernelCompileFinished.listeners_.emplace_back(
            [this] (std::string const&_path, sr::ErrorLogContainer const&_errorlog) {
                this->imgui_layer_->onFKernelCompileFinished(_path, _errorlog);
            });

        imgui_layer_->FKernelPath_query.source_ =
            [this] () {
                return this->sr_layer_->GetKernelPath(sr::ShaderStage::kFragment);
            };

        imgui_layer_->FKernelPath_onReturn.listeners_.emplace_back(
            [this] (std::string const&_path) {
                this->sr_layer_->WatchKernelFile(sr::ShaderStage::kFragment, _path.c_str());
            });

        imgui_layer_->Uniforms_query.source_ =
            [this] () {
                return this->sr_layer_->GetUniforms();
            };

        imgui_layer_->Uniforms_onReturn.listeners_.emplace_back(
            [this] (sr::UniformContainer const&_uniforms) {
                this->sr_layer_->SetUniforms(_uniforms);
            });
    }
}

void
LayerMediator::SpecialKey(eKey _key, std::uint8_t _v)
{
    state_.key_map[_v] = _key;
}

void
LayerMediator::ResizeEvent(uibase::Vec2i_t const& _size)
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
        imgui_layer_->imgui_context_.SetResolution(state_.screen_size[0], state_.screen_size[1]);
    }
}

void
LayerMediator::MouseDown(bool _v)
{
    if (state_.mouse_down == _v)
        return;

    state_.mouse_down = _v;

    if (_v) std::cout << "mouse down" << std::endl;
    else std::cout << "mouse up" << std::endl;

    if (gizmo_layer_)
    {
        if (state_.hover_gizmo)
        {
            const std::uint32_t gizmo_index = uibase::UnpackGizmoIndex(state_.hover_gizmo);
            uibase::GizmoDesc& gizmo = gizmo_layer_->gizmos_[gizmo_index-1];

            if (!_v)
            {
                if (gizmo.type_ == uibase::eGizmoType::kBox)
                    gizmo.color_ = kGizmoColorOff;
            }
            else
            {
                if (gizmo.type_ == uibase::eGizmoType::kTransform)
                    state_.select_gizmo = state_.hover_gizmo;
            }
        }
    }

    if (!_v)
    {
        state_.select_gizmo = 0u;
        state_.hover_gizmo = 0u;
    }
}

void
LayerMediator::MousePos(uibase::Vec2i_t const& _pos)
{
    if (state_.mouse_pos[0] == _pos[0] && state_.mouse_pos[1] == _pos[1])
        return;

    const uibase::Vec2i_t delta = uibase::vec2i_sub(_pos, state_.mouse_pos);
    state_.mouse_pos = _pos;

    if (gizmo_layer_)
    {
        // =====================================================================
        // framebuffer_->ReadPixel() ?
        framebuffer_->Bind();
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        std::uint32_t gizmo_payload = 0;
        static_assert(sizeof(int) == sizeof(float), "");
        glReadPixels(state_.mouse_pos[0], state_.mouse_pos[1],
                     1, 1,
                     GL_RED, GL_FLOAT, (GLfloat*)&gizmo_payload);
        glReadBuffer(GL_NONE);
        framebuffer_->Unbind();
        // =====================================================================

        if (gizmo_payload != state_.hover_gizmo)
        {
            // =================================================================
            // HOVER_OUT
            if (state_.hover_gizmo)
            {
                uibase::GizmoDesc& gizmo = gizmo_layer_->GetGizmo(state_.hover_gizmo);
                const std::uint32_t subgizmo_index = uibase::UnpackSubgizmoIndex(state_.hover_gizmo);

                if (gizmo.type_ == uibase::eGizmoType::kBox)
                    gizmo.color_ = kGizmoColorOff;

                if (gizmo.type_ == uibase::eGizmoType::kTransform)
                    std::cout << subgizmo_index << " off" << std::endl;
            }

            state_.hover_gizmo = gizmo_payload;

            // =================================================================
            // HOVER_IN
            if (state_.hover_gizmo)
            {
                uibase::GizmoDesc& gizmo = gizmo_layer_->GetGizmo(state_.hover_gizmo);
                const std::uint32_t subgizmo_index = uibase::UnpackSubgizmoIndex(state_.hover_gizmo);

                if (gizmo.type_ == uibase::eGizmoType::kBox)
                    gizmo.color_ = kGizmoColorOn;

                if (gizmo.type_ == uibase::eGizmoType::kTransform)
                {
                    std::cout << subgizmo_index << " on" << std::endl;
                }
            }
        }

        if (state_.select_gizmo)
        {
            uibase::GizmoDesc& gizmo = gizmo_layer_->GetGizmo(state_.select_gizmo);

            if (gizmo.type_ == uibase::eGizmoType::kTransform)
            {
                const std::uint32_t subgizmo_index = uibase::UnpackSubgizmoIndex(state_.select_gizmo);
                std::cout << "move transfo" << std::endl;
                std::cout << "delta " << delta[0] << " " << delta[1] << std::endl;

                const uibase::Vec2_t deltaf = uibase::vec2_itof(delta);
                if (subgizmo_index == 0u)
                {
                    std::cout << "x" << std::endl;
                    gizmo.position_[0] += deltaf[0] * 0.01f;
                }
                if (subgizmo_index == 1u)
                {
                    std::cout << "y" << std::endl;
                    gizmo.position_[1] += deltaf[1] * 0.01f;
                }
                if (subgizmo_index == 2u)
                {
                    std::cout << "z" << std::endl;
                }
            }
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
        framebuffer_->Bind();

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
        imgui_layer_->RunFrame(state_);

    return result;
}

} // namespace appbase

namespace {

std::unique_ptr<oglbase::Framebuffer>
MakeGizmoLayerFramebuffer(uibase::Vec2i_t const& _screen_size)
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
MakeGizmoLayerProjection(uibase::Vec2i_t const& _screen_size)
{
    float aspect_ratio = static_cast<float>(_screen_size[1]) / static_cast<float>(_screen_size[0]);
    return uibase::perspective(0.01f, 1000.f, 3.1415926534f*0.5f, aspect_ratio);
}


} // namespace
