#pragma once

#include "Pillar/Renderer/Framebuffer.h"

namespace Pillar {

    class OpenGLFramebuffer : public Framebuffer
    {
    public:
        OpenGLFramebuffer(const FramebufferSpecification& spec);
        virtual ~OpenGLFramebuffer();

        void Invalidate();  // Recreate framebuffer with current specification

        virtual void Bind() override;
        virtual void Unbind() override;
        virtual void Resize(uint32_t width, uint32_t height) override;

        virtual uint32_t GetColorAttachmentRendererID() const override { return m_ColorAttachment; }
        virtual uint32_t GetDepthAttachmentRendererID() const override { return m_DepthAttachment; }
        
        virtual const FramebufferSpecification& GetSpecification() const override { return m_Specification; }
        
        virtual uint32_t GetWidth() const override { return m_Specification.Width; }
        virtual uint32_t GetHeight() const override { return m_Specification.Height; }

    private:
        uint32_t m_RendererID = 0;
        uint32_t m_ColorAttachment = 0;
        uint32_t m_DepthAttachment = 0;
        FramebufferSpecification m_Specification;
    };

}
