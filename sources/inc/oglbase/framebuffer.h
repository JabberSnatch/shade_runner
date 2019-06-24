/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. You can do whatever you want with this
 * stuff. If we meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma once

#include <vector>

#include <GL/glew.h>
#include <GL/gl.h>

#include "oglbase/handle.h"

namespace oglbase {


struct Framebuffer
{
    struct AttachmentDesc
    {
        GLenum point;
        GLenum format;
    };
    using AttachmentDescs = std::vector<AttachmentDesc>;

    Framebuffer(GLsizei _width, GLsizei _height,
                AttachmentDescs const& _attachments,
                bool _depth_stencil);
    Framebuffer(Framebuffer const&) = delete;
    Framebuffer& operator=(Framebuffer const&) = delete;

    void Bind() const;
    void Unbind() const;

    FBOPtr fbo_;

private:
    bool depth_stencil_;
    std::vector<GLenum> draw_buffers_;
    std::vector<TexturePtr> buffers_;
};


} // namespace oglbase
