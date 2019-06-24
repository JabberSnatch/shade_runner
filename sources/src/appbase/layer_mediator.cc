/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. You can do whatever you want with this
 * stuff. If we meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "appbase/layer_mediator.h"

#include "oglbase/framebuffer.h"
#include "shaderunner/shaderunner.h"
#include "uibase/gizmo_layer.h"

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

static uibase::Vec3_t const kGizmoColorOff{ 0.f, 1.f, 0.f };
static uibase::Vec3_t const kGizmoColorOn{ 1.f, 0.f, 0.f };

} // namespace

namespace appbase {

LayerMediator::LayerMediator(Vec2i_t const& _screen_size, unsigned _flags)
{
    state_.screen_size = _screen_size;
    if (_flags & LayerFlag::kShaderunner)
    {
        sr_layer_ = std::make_unique<sr::RenderContext>();
        sr_layer_->SetResolution(state_.screen_size[0], state_.screen_size[1]);
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

    return result;
}

} // namespace appbase
