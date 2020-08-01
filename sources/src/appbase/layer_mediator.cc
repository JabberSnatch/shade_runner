/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. You can do whatever you want with this
 * stuff. If we meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "appbase/layer_mediator.h"

#include <cstring>

#include <imgui.h>

#include "oglbase/framebuffer.h"
#include "shaderunner/shaderunner.h"
#include "uibase/gizmo_layer.h"
#include "utility/file.h"

#include <iostream>

namespace {

static uibase::Vec3_t const kGizmoColorOff{ 0.f, 1.f, 0.f };
static uibase::Vec3_t const kGizmoColorOn{ 1.f, 0.f, 0.f };

float AspectRatio(uibase::Vec2i_t const& _screen_size);

std::unique_ptr<oglbase::Framebuffer> MakeGizmoLayerFramebuffer(uibase::Vec2i_t const& _screen_size);
uibase::Mat4_t MakeGizmoLayerProjection(uibase::Vec2i_t const& _screen_size);
uibase::Mat4_t MakeCameraMatrix(uibase::Vec3_t const& _p, uibase::Vec3_t const& _t, uibase::Vec3_t const& _u);

} // namespace

namespace appbase {

static constexpr int kTransfoCount = 4;

LayerMediator::LayerMediator(uibase::Vec2i_t const& _screen_size, unsigned _flags)
{
    state_.screen_size = _screen_size;
    std::fill(std::begin(state_.key_down), std::end(state_.key_down), false);
    std::fill(std::begin(state_.key_map), std::end(state_.key_map), (eKey)0);

    if (_flags & LayerFlag::kShaderunner)
    {
        sr_layer_ = std::make_unique<sr::RenderContext>();
        sr_layer_->SetResolution(state_.screen_size[0], state_.screen_size[1]);
        sr_layer_->projection_matrix = MakeGizmoLayerProjection(state_.screen_size);
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

        for (int i = 0; i < kTransfoCount; ++i)
        {
            gizmo_layer_->gizmos_.push_back(
                uibase::GizmoDesc{ uibase::eGizmoType::kTransform,
                        uibase::Vec3_t{ 0.f, 0.f + (float)i * 0.01f, -3.f },
                        kGizmoColorOff }
            );
        }
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

    if (sr_layer_ && gizmo_layer_)
    {
        for (std::uint32_t i = 0;
             i < sr_layer_->kGizmoCountMax && i < gizmo_layer_->gizmos_.size();
             ++i)
        {
            std::cout << gizmo_layer_->gizmos_[i].position_[0] << " " << gizmo_layer_->gizmos_[i].position_[1] << " " << gizmo_layer_->gizmos_[i].position_[2] << std::endl;

            std::memcpy(&(sr_layer_->gizmo_positions[i]),
                        &gizmo_layer_->gizmos_[i].position_[0],
                        sizeof(float)*3);
        }

        sr_layer_->gizmo_count = (int)gizmo_layer_->gizmos_.size();
    }

    back_state_ = state_;
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
        sr_layer_->projection_matrix = MakeGizmoLayerProjection(state_.screen_size);
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
                std::uint32_t const subgizmo_index = uibase::UnpackSubgizmoIndex(state_.select_gizmo);
                std::cout << "move transfo" << std::endl;
                std::cout << "delta " << delta[0] << " " << delta[1] << std::endl;

                uibase::Vec2_t const deltaf = uibase::vec2_itof(delta);
                uibase::Vec3_t selected_axis{ 0.f, 0.f, 0.f };
                selected_axis[subgizmo_index] = 1.f;

                uibase::Vec4_t const WS_gizmo_position{
                    gizmo.position_[0],
                    gizmo.position_[1],
                    gizmo.position_[2],
                    1.f };
                uibase::Vec4_t CS_gizmo_position =
                    uibase::mat4_vec4_mul(sr_layer_->projection_matrix, WS_gizmo_position);
                CS_gizmo_position[0] /= CS_gizmo_position[3];
                CS_gizmo_position[1] /= CS_gizmo_position[3];
                CS_gizmo_position[2] /= CS_gizmo_position[3];

                uibase::Vec4_t const WS_axis_offset{
                    gizmo.position_[0] + selected_axis[0],
                    gizmo.position_[1] + selected_axis[1],
                    gizmo.position_[2] + selected_axis[2],
                    1.f };
                uibase::Vec4_t CS_axis_offset =
                    uibase::mat4_vec4_mul(sr_layer_->projection_matrix, WS_axis_offset);
                CS_axis_offset[0] /= CS_axis_offset[3];
                CS_axis_offset[1] /= CS_axis_offset[3];
                CS_axis_offset[2] /= CS_axis_offset[3];

                uibase::Vec3_t CS_axis_direction = uibase::vec3_sub(*(uibase::Vec3_t*)(&CS_axis_offset),
                                                                    *(uibase::Vec3_t*)(&CS_gizmo_position));

                CS_axis_direction[1] *= AspectRatio(state_.screen_size);

                uibase::Vec3_t const result = uibase::vec3_add(
                    gizmo.position_,
                    uibase::vec3_float_mul(
                        uibase::vec3_add(
                            uibase::vec3_float_mul(selected_axis, CS_axis_direction[0]*deltaf[0]),
                            uibase::vec3_float_mul(selected_axis, CS_axis_direction[1]*deltaf[1])),
                        0.1f));

                std::memcpy(&gizmo.position_[0], &result[0], 3*sizeof(float));

                std::uint32_t const gizmo_index = uibase::UnpackGizmoIndex(state_.select_gizmo);
                if (gizmo_index < sr_layer_->kGizmoCountMax)
                    std::memcpy(&(sr_layer_->gizmo_positions[gizmo_index-1]),
                                &gizmo.position_[0],
                                sizeof(float)*3);
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
LayerMediator::RunFrame(float _dt)
{
    bool result = false;

    static constexpr float kCameraAngleSpeed = 25.f;
    if (state_.key_down[appbase::kUp])
        state_.camera_rotation[0] += kCameraAngleSpeed * _dt;
    if (state_.key_down[appbase::kDown])
        state_.camera_rotation[0] -= kCameraAngleSpeed * _dt;
    if (state_.key_down[appbase::kLeft])
        state_.camera_rotation[1] += kCameraAngleSpeed * _dt;
    if (state_.key_down[appbase::kRight])
        state_.camera_rotation[1] -= kCameraAngleSpeed * _dt;

    if (state_.camera_rotation[0] > 360.f)
        state_.camera_rotation[0] -= 360.f;
    if (state_.camera_rotation[0] < 0.f)
        state_.camera_rotation[0] += 360.f;
    if (state_.camera_rotation[1] > 360.f)
        state_.camera_rotation[1] -= 360.f;
    if (state_.camera_rotation[1] < 0.f)
        state_.camera_rotation[1] += 360.f;

    float camspeed = 1.f;
    if (state_.mod_down & appbase::kShift)
        camspeed *= 100.f;
    uibase::Vec3_t t{ 0.f, 0.f, 0.f };
    if (state_.key_down['w'])
        t[2] -= camspeed * _dt;
    if (state_.key_down['s'])
        t[2] += camspeed * _dt;
    if (state_.key_down['d'])
        t[0] += camspeed * _dt;
    if (state_.key_down['a'])
        t[0] -= camspeed * _dt;
    if (state_.key_down['q'])
        t[1] += camspeed * _dt;
    if (state_.key_down['e'])
        t[1] -= camspeed * _dt;

    uibase::Mat4_t camrot = uibase::mat4_rot(state_.camera_rotation);
    uibase::Vec3_t lt = uibase::vec3_from_vec4(
        uibase::mat4_vec4_mul(camrot, uibase::vec4_from_vec3(t, 0.f)));

    state_.camera_position = uibase::vec3_add(lt, state_.camera_position);

    uibase::Vec3_t camforward = uibase::vec3_from_vec4(
        uibase::mat4_vec4_mul(
            camrot,
            uibase::Vec4_t{0.f, 0.f, -1.f, 0.f})
    );
    uibase::Vec3_t camup = uibase::vec3_from_vec4(
        uibase::mat4_vec4_mul(
            camrot,
            uibase::Vec4_t{0.f, 1.f, 0.f, 0.f})
    );
    uibase::Vec3_t camtarget = uibase::vec3_add(state_.camera_position, camforward);
    uibase::Mat4_t cammat = MakeCameraMatrix(state_.camera_position,
                                             camtarget,
                                             camup);

    if (gizmo_layer_)
        framebuffer_->Bind();

    if (sr_layer_)
    {
        sr_layer_->projection_matrix = uibase::mat4_mul(
            MakeGizmoLayerProjection(state_.screen_size),
            cammat
        );
        result = sr_layer_->RenderFrame();
    }

    if (gizmo_layer_)
    {
        gizmo_layer_->projection_ = uibase::mat4_mul(
            MakeGizmoLayerProjection(state_.screen_size),
            cammat
        );

        if (state_.enable_gizmos)
            gizmo_layer_->RenderFrame();
        else
            gizmo_layer_->ClearIDBuffer();

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

    back_state_ = state_;
    return result;
}

} // namespace appbase

namespace {

float AspectRatio(uibase::Vec2i_t const& _screen_size)
{
    return static_cast<float>(_screen_size[1]) / static_cast<float>(_screen_size[0]);
}

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

uibase::Mat4_t
MakeGizmoLayerProjection(uibase::Vec2i_t const& _screen_size)
{
    float aspect_ratio = AspectRatio(_screen_size);
    return uibase::perspective(0.01f, 1000.f, 3.1415926534f*0.5f, aspect_ratio);
}

uibase::Mat4_t
MakeCameraMatrix(uibase::Vec3_t const& _p, uibase::Vec3_t const& _t, uibase::Vec3_t const& _u)
{
    uibase::Vec3_t const f = uibase::vec3_normalise(uibase::vec3_sub(_p, _t));
    uibase::Vec3_t const r = uibase::vec3_normalise(uibase::vec3_cross(_u, f));
    uibase::Vec3_t const u = uibase::vec3_cross(f, r);

    uibase::Vec3_t const p = uibase::vec3_float_mul(_p, -1.f);
    uibase::Vec3_t const t{ uibase::vec3_dot(p, r), uibase::vec3_dot(p, u), uibase::vec3_dot(p, f) };

    return uibase::mat4_col(uibase::Vec4_t{r[0], u[0], f[0], 0.f},
                            uibase::Vec4_t{r[1], u[1], f[1], 0.f},
                            uibase::Vec4_t{r[2], u[2], f[2], 0.f},
                            uibase::vec3_float_concat(t, 1.f));
}

} // namespace
