/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. You can do whatever you want with this
 * stuff. If we meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "oglbase/framebuffer.h"

#include <algorithm>
#include <cassert>

#include "oglbase/error.h"

namespace oglbase {

Framebuffer::Framebuffer(GLsizei _width, GLsizei _height, AttachmentDescs const& _attachments, bool _depth_stencil) :
    fbo_{ 0u },
    depth_stencil_{ _depth_stencil }
{
    assert(!_attachments.empty());
    glGenFramebuffers(1, fbo_.get());
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    draw_buffers_.reserve(_attachments.size());
    buffers_.reserve(_attachments.size() + (depth_stencil_ ? 1 : 0));
    std::transform(
        std::begin(_attachments), std::end(_attachments), std::back_inserter(buffers_),
        [&_width, &_height, this](AttachmentDesc const& _desc) {
            TexturePtr result{ 0u };
            glGenTextures(1, result.get());
            glBindTexture(GL_TEXTURE_2D, result);
            glTexStorage2D(GL_TEXTURE_2D, 1, _desc.format, _width, _height);
            glBindTexture(GL_TEXTURE_2D, 0);

            glFramebufferTexture(GL_FRAMEBUFFER, _desc.point, result, 0);

            this->draw_buffers_.emplace_back(_desc.point);
            return result;
        });

    if (depth_stencil_)
    {
        buffers_.emplace_back(0u);
        glGenTextures(1, buffers_.back().get());
        glBindTexture(GL_TEXTURE_2D, buffers_.back());
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, _width, _height);
        glBindTexture(GL_TEXTURE_2D, 0);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, buffers_.back(), 0);
    }

    std::cout << "Framebuffer construction " << enum_string(glCheckFramebufferStatus(GL_FRAMEBUFFER)) << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void
Framebuffer::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glDrawBuffers((GLsizei)draw_buffers_.size(), draw_buffers_.data());
}

void
Framebuffer::Unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace oglbase
