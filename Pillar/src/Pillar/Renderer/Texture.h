#pragma once

#include "Pillar/Core.h"
#include <string>
#include <memory>

namespace Pillar {

    class PIL_API Texture
    {
    public:
        virtual ~Texture() = default;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual void Bind(uint32_t slot = 0) const = 0;
    };

    class PIL_API Texture2D : public Texture
    {
    public:
        static std::shared_ptr<Texture2D> Create(const std::string& path);
        static std::shared_ptr<Texture2D> Create(uint32_t width, uint32_t height);
    };

}
