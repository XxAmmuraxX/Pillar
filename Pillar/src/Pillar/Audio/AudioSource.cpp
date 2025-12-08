#include "Pillar/Audio/AudioSource.h"
#include "Platform/OpenAL/OpenALSource.h"
#include "Platform/OpenAL/OpenALContext.h"
#include "Pillar/Logger.h"

namespace Pillar {

    std::shared_ptr<AudioSource> AudioSource::Create()
    {
        if (!OpenALContext::IsInitialized())
        {
            PIL_CORE_ERROR("AudioSource::Create: Audio engine not initialized");
            return nullptr;
        }

        return std::make_shared<OpenALSource>();
    }

}
