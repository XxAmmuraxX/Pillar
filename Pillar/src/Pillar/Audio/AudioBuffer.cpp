#include "Pillar/Audio/AudioBuffer.h"
#include "Platform/OpenAL/OpenALBuffer.h"
#include "Platform/OpenAL/OpenALContext.h"
#include "Pillar/Logger.h"

namespace Pillar {

    std::shared_ptr<AudioBuffer> AudioBuffer::Create(const std::string& filepath)
    {
        if (!OpenALContext::IsInitialized())
        {
            PIL_CORE_ERROR("AudioBuffer::Create: Audio engine not initialized");
            return nullptr;
        }

        auto buffer = std::make_shared<OpenALBuffer>(filepath);
        
        if (!buffer->IsLoaded())
        {
            PIL_CORE_ERROR("AudioBuffer::Create: Failed to load audio file: {0}", filepath);
            return nullptr;
        }

        return buffer;
    }

}
