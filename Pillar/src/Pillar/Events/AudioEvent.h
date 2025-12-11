#pragma once

#include "Event.h"
#include <sstream>

namespace Pillar {

    /**
     * @brief Types of audio events
     */
    enum class AudioEventType
    {
        SoundStarted,
        SoundFinished,
        SoundPaused,
        SoundResumed,
        SoundStopped
    };

    /**
     * @brief Event fired when audio playback state changes
     * 
     * Optional feature for tracking audio playback events.
     * Can be used for triggering gameplay logic based on audio cues.
     */
    class PIL_API AudioPlaybackEvent : public Event
    {
    public:
        AudioPlaybackEvent(AudioEventType type, uint32_t sourceId)
            : m_Type(type), m_SourceID(sourceId) {}
        
        AudioEventType GetAudioEventType() const { return m_Type; }
        uint32_t GetSourceID() const { return m_SourceID; }
        
        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "AudioPlaybackEvent: " << GetAudioEventTypeName() 
               << " (Source ID: " << m_SourceID << ")";
            return ss.str();
        }
        
        EVENT_CLASS_TYPE(AudioPlayback)
        EVENT_CLASS_CATEGORY(EventCategoryAudio)
        
    private:
        const char* GetAudioEventTypeName() const
        {
            switch (m_Type)
            {
                case AudioEventType::SoundStarted:  return "SoundStarted";
                case AudioEventType::SoundFinished: return "SoundFinished";
                case AudioEventType::SoundPaused:   return "SoundPaused";
                case AudioEventType::SoundResumed:  return "SoundResumed";
                case AudioEventType::SoundStopped:  return "SoundStopped";
                default: return "Unknown";
            }
        }
        
        AudioEventType m_Type;
        uint32_t m_SourceID;
    };

    /**
     * @brief Event fired when audio engine is initialized or shut down
     */
    class PIL_API AudioEngineEvent : public Event
    {
    public:
        AudioEngineEvent(bool initialized)
            : m_Initialized(initialized) {}
        
        bool IsInitialized() const { return m_Initialized; }
        
        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "AudioEngineEvent: " << (m_Initialized ? "Initialized" : "Shutdown");
            return ss.str();
        }
        
        EVENT_CLASS_TYPE(AudioEngine)
        EVENT_CLASS_CATEGORY(EventCategoryAudio)
        
    private:
        bool m_Initialized;
    };

} // namespace Pillar
