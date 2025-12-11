#pragma once

#include "EditorPanel.h"
#include <vector>
#include <string>

namespace PillarEditor {

    enum class LogLevel
    {
        Trace = 0,
        Info,
        Warn,
        Error
    };

    struct LogMessage
    {
        std::string Message;
        LogLevel Level;
    };

    class ConsolePanel : public EditorPanel
    {
    public:
        ConsolePanel();

        virtual void OnImGuiRender() override;

        static void Log(const std::string& message, LogLevel level = LogLevel::Info);
        static void Clear();

    private:
        static std::vector<LogMessage> s_Messages;
        static const size_t s_MaxMessages = 500;
        
        bool m_AutoScroll = true;
        bool m_ShowTrace = true;
        bool m_ShowInfo = true;
        bool m_ShowWarn = true;
        bool m_ShowError = true;
    };

}
