#pragma once

#include <AL/al.h>
#include <AL/alc.h>

namespace Pillar {

    /**
     * @brief Manages OpenAL device and context initialization.
     * 
     * This class provides static methods for initializing and
     * shutting down the OpenAL audio context.
     */
    class OpenALContext
    {
    public:
        /**
         * @brief Initialize OpenAL device and context.
         * @return true if initialization succeeded, false otherwise.
         */
        static bool Init();

        /**
         * @brief Shutdown OpenAL and release resources.
         */
        static void Shutdown();

        /**
         * @brief Check if OpenAL is initialized.
         * @return true if initialized, false otherwise.
         */
        static bool IsInitialized() { return s_Initialized; }

        /**
         * @brief Get the OpenAL device.
         * @return Pointer to the ALCdevice.
         */
        static ALCdevice* GetDevice() { return s_Device; }

        /**
         * @brief Get the OpenAL context.
         * @return Pointer to the ALCcontext.
         */
        static ALCcontext* GetContext() { return s_Context; }

        /**
         * @brief Check for OpenAL errors and log them.
         * @param operation Description of the operation for error messages.
         * @return true if no error, false if error occurred.
         */
        static bool CheckError(const char* operation);

    private:
        static ALCdevice* s_Device;
        static ALCcontext* s_Context;
        static bool s_Initialized;
    };

}
