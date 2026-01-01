#pragma once

#include "Pillar/Core.h"
#include <memory>
#include <cstdint>

namespace Pillar {

    struct FramebufferSpecification
    {
        uint32_t Width = 1280;
        uint32_t Height = 720;
        uint32_t Samples = 1;           // MSAA samples (1 = no multisampling)
        bool SwapChainTarget = false;   // If true, renders to screen instead of texture
    };

    class PIL_API Framebuffer
    {
    public:
        virtual ~Framebuffer() = default;

        virtual void Bind() = 0;
        virtual void Unbind() = 0;
        virtual void Resize(uint32_t width, uint32_t height) = 0;

        virtual uint32_t GetColorAttachmentRendererID() const = 0;
        virtual uint32_t GetDepthAttachmentRendererID() const = 0;
        
        virtual const FramebufferSpecification& GetSpecification() const = 0;
        
        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        static std::shared_ptr<Framebuffer> Create(const FramebufferSpecification& spec);
    };

}
