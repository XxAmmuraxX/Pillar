#include "Pillar/Audio/WavLoader.h"
#include "Pillar/Logger.h"
#include <fstream>
#include <cstring>

namespace Pillar {

    bool WavLoader::Load(const std::string& filepath, WavData& outData)
    {
        // Open file in binary mode
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            PIL_CORE_ERROR("WavLoader: Failed to open file: {0}", filepath);
            return false;
        }

        // Get file size
        size_t fileSize = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        // Read entire file into memory
        std::vector<char> fileData(fileSize);
        if (!file.read(fileData.data(), fileSize))
        {
            PIL_CORE_ERROR("WavLoader: Failed to read file: {0}", filepath);
            return false;
        }

        file.close();

        return LoadFromMemory(fileData.data(), fileSize, outData);
    }

    bool WavLoader::LoadFromMemory(const char* data, size_t size, WavData& outData)
    {
        if (data == nullptr)
        {
            PIL_CORE_ERROR("WavLoader: Data pointer is null");
            return false;
        }

        size_t dataOffset = 0;
        size_t dataSize = 0;

        if (!ParseHeader(data, size, outData, dataOffset, dataSize))
        {
            return false;
        }

        // Copy audio data
        outData.Data.resize(dataSize);
        std::memcpy(outData.Data.data(), data + dataOffset, dataSize);

        // Calculate duration
        if (outData.SampleRate > 0 && outData.Channels > 0 && outData.BitsPerSample > 0)
        {
            int bytesPerSample = outData.BitsPerSample / 8;
            int bytesPerSecond = outData.SampleRate * outData.Channels * bytesPerSample;
            outData.Duration = static_cast<float>(dataSize) / static_cast<float>(bytesPerSecond);
        }

        PIL_CORE_TRACE("WavLoader: Loaded audio - {0}Hz, {1} channels, {2}-bit, {3:.2f}s",
            outData.SampleRate, outData.Channels, outData.BitsPerSample, outData.Duration);

        return true;
    }

    bool WavLoader::ParseHeader(const char* data, size_t size, WavData& outData, size_t& dataOffset, size_t& dataSize)
    {
        // Minimum size check for RIFF header
        if (size < sizeof(RIFFHeader))
        {
            PIL_CORE_ERROR("WavLoader: File too small to contain WAV header");
            return false;
        }

        // Parse RIFF header
        const RIFFHeader* riffHeader = reinterpret_cast<const RIFFHeader*>(data);
        
        if (std::strncmp(riffHeader->ChunkID, "RIFF", 4) != 0)
        {
            PIL_CORE_ERROR("WavLoader: Invalid RIFF header");
            return false;
        }

        if (std::strncmp(riffHeader->Format, "WAVE", 4) != 0)
        {
            PIL_CORE_ERROR("WavLoader: Not a WAVE file");
            return false;
        }

        // Search for fmt and data chunks
        size_t offset = sizeof(RIFFHeader);
        bool foundFmt = false;
        bool foundData = false;

        while (offset + 8 <= size && (!foundFmt || !foundData))
        {
            const char* chunkID = data + offset;
            uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(data + offset + 4);

            if (std::strncmp(chunkID, "fmt ", 4) == 0)
            {
                // Parse fmt chunk
                if (offset + 8 + chunkSize > size)
                {
                    PIL_CORE_ERROR("WavLoader: fmt chunk extends beyond file");
                    return false;
                }

                const FmtChunk* fmtChunk = reinterpret_cast<const FmtChunk*>(data + offset);
                
                // Check for PCM format (AudioFormat == 1)
                if (fmtChunk->AudioFormat != 1)
                {
                    PIL_CORE_ERROR("WavLoader: Only PCM format is supported (found format: {0})", fmtChunk->AudioFormat);
                    return false;
                }

                outData.Channels = fmtChunk->NumChannels;
                outData.SampleRate = fmtChunk->SampleRate;
                outData.BitsPerSample = fmtChunk->BitsPerSample;

                // Validate supported formats
                if (outData.Channels != 1 && outData.Channels != 2)
                {
                    PIL_CORE_ERROR("WavLoader: Only mono and stereo are supported (found {0} channels)", outData.Channels);
                    return false;
                }

                if (outData.BitsPerSample != 8 && outData.BitsPerSample != 16)
                {
                    PIL_CORE_ERROR("WavLoader: Only 8-bit and 16-bit samples are supported (found {0}-bit)", outData.BitsPerSample);
                    return false;
                }

                foundFmt = true;
            }
            else if (std::strncmp(chunkID, "data", 4) == 0)
            {
                // Found data chunk
                dataOffset = offset + 8;
                dataSize = chunkSize;

                if (dataOffset + dataSize > size)
                {
                    PIL_CORE_WARN("WavLoader: data chunk size exceeds file size, truncating");
                    dataSize = size - dataOffset;
                }

                foundData = true;
            }

            // Move to next chunk (chunk size + 8 bytes for header)
            offset += 8 + chunkSize;
            
            // Ensure word alignment
            if (chunkSize % 2 != 0)
            {
                offset += 1;
            }
        }

        if (!foundFmt)
        {
            PIL_CORE_ERROR("WavLoader: fmt chunk not found");
            return false;
        }

        if (!foundData)
        {
            PIL_CORE_ERROR("WavLoader: data chunk not found");
            return false;
        }

        return true;
    }

}
