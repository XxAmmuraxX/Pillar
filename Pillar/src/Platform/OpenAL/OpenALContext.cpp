#include "Platform/OpenAL/OpenALContext.h"
#include "Pillar/Logger.h"

namespace Pillar {

    ALCdevice* OpenALContext::s_Device = nullptr;
    ALCcontext* OpenALContext::s_Context = nullptr;
    bool OpenALContext::s_Initialized = false;

    bool OpenALContext::Init()
    {
        if (s_Initialized)
        {
            PIL_CORE_WARN("OpenALContext: Already initialized");
            return true;
        }

        // Open the default audio device
        s_Device = alcOpenDevice(nullptr);
        if (!s_Device)
        {
            PIL_CORE_ERROR("OpenALContext: Failed to open audio device");
            return false;
        }

        // Log device name
        const char* deviceName = alcGetString(s_Device, ALC_DEVICE_SPECIFIER);
        PIL_CORE_INFO("OpenALContext: Opened audio device: {0}", deviceName ? deviceName : "Unknown");

        // Create audio context
        s_Context = alcCreateContext(s_Device, nullptr);
        if (!s_Context)
        {
            PIL_CORE_ERROR("OpenALContext: Failed to create audio context");
            alcCloseDevice(s_Device);
            s_Device = nullptr;
            return false;
        }

        // Make the context current
        if (!alcMakeContextCurrent(s_Context))
        {
            PIL_CORE_ERROR("OpenALContext: Failed to make context current");
            alcDestroyContext(s_Context);
            alcCloseDevice(s_Device);
            s_Context = nullptr;
            s_Device = nullptr;
            return false;
        }

        // Log OpenAL version info
        const char* vendor = alGetString(AL_VENDOR);
        const char* renderer = alGetString(AL_RENDERER);
        const char* version = alGetString(AL_VERSION);
        PIL_CORE_INFO("OpenALContext: Vendor: {0}", vendor ? vendor : "Unknown");
        PIL_CORE_INFO("OpenALContext: Renderer: {0}", renderer ? renderer : "Unknown");
        PIL_CORE_INFO("OpenALContext: Version: {0}", version ? version : "Unknown");

        // Set default listener properties
        alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
        alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
        
        // Listener orientation: forward and up vectors
        ALfloat orientation[] = { 0.0f, 0.0f, -1.0f,  // Forward (looking into screen)
                                   0.0f, 1.0f, 0.0f }; // Up
        alListenerfv(AL_ORIENTATION, orientation);

        s_Initialized = true;
        PIL_CORE_INFO("OpenALContext: Initialized successfully");
        return true;
    }

    void OpenALContext::Shutdown()
    {
        if (!s_Initialized)
        {
            return;
        }

        // Make no context current
        alcMakeContextCurrent(nullptr);

        // Destroy context
        if (s_Context)
        {
            alcDestroyContext(s_Context);
            s_Context = nullptr;
        }

        // Close device
        if (s_Device)
        {
            alcCloseDevice(s_Device);
            s_Device = nullptr;
        }

        s_Initialized = false;
        PIL_CORE_INFO("OpenALContext: Shutdown complete");
    }

    bool OpenALContext::CheckError(const char* operation)
    {
        ALenum error = alGetError();
        if (error != AL_NO_ERROR)
        {
            const char* errorStr = "Unknown error";
            switch (error)
            {
                case AL_INVALID_NAME:      errorStr = "AL_INVALID_NAME"; break;
                case AL_INVALID_ENUM:      errorStr = "AL_INVALID_ENUM"; break;
                case AL_INVALID_VALUE:     errorStr = "AL_INVALID_VALUE"; break;
                case AL_INVALID_OPERATION: errorStr = "AL_INVALID_OPERATION"; break;
                case AL_OUT_OF_MEMORY:     errorStr = "AL_OUT_OF_MEMORY"; break;
            }
            PIL_CORE_ERROR("OpenAL Error [{0}]: {1}", operation, errorStr);
            return false;
        }
        return true;
    }

}
