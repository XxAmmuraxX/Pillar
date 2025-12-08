#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace Pillar {

    /**
     * @brief Structure containing decoded WAV audio data.
     */
    struct WavData
    {
        std::vector<char> Data;     // Raw audio sample data
        int SampleRate = 0;         // Sample rate in Hz (e.g., 44100)
        int Channels = 0;           // Number of channels (1 = mono, 2 = stereo)
        int BitsPerSample = 0;      // Bits per sample (8 or 16)
        float Duration = 0.0f;      // Duration in seconds
    };

    /**
     * @brief Utility class for loading WAV audio files.
     * 
     * Supports uncompressed PCM WAV files with:
     * - 8-bit or 16-bit samples
     * - Mono or stereo channels
     * - Any sample rate
     */
    class WavLoader
    {
    public:
        /**
         * @brief Load a WAV file from disk.
         * @param filepath Path to the WAV file.
         * @param outData Output structure to fill with audio data.
         * @return true if loading succeeded, false otherwise.
         */
        static bool Load(const std::string& filepath, WavData& outData);

        /**
         * @brief Load WAV data from memory.
         * @param data Pointer to WAV file data in memory.
         * @param size Size of the data in bytes.
         * @param outData Output structure to fill with audio data.
         * @return true if loading succeeded, false otherwise.
         */
        static bool LoadFromMemory(const char* data, size_t size, WavData& outData);

    private:
        // WAV file format structures
        #pragma pack(push, 1)
        struct RIFFHeader
        {
            char ChunkID[4];        // "RIFF"
            uint32_t ChunkSize;     // File size - 8
            char Format[4];         // "WAVE"
        };

        struct FmtChunk
        {
            char SubchunkID[4];     // "fmt "
            uint32_t SubchunkSize;  // 16 for PCM
            uint16_t AudioFormat;   // 1 for PCM
            uint16_t NumChannels;   // 1 = mono, 2 = stereo
            uint32_t SampleRate;    // 44100, etc.
            uint32_t ByteRate;      // SampleRate * NumChannels * BitsPerSample/8
            uint16_t BlockAlign;    // NumChannels * BitsPerSample/8
            uint16_t BitsPerSample; // 8 or 16
        };

        struct DataChunkHeader
        {
            char SubchunkID[4];     // "data"
            uint32_t SubchunkSize;  // Number of bytes in data
        };
        #pragma pack(pop)

        static bool ParseHeader(const char* data, size_t size, WavData& outData, size_t& dataOffset, size_t& dataSize);
    };

}
