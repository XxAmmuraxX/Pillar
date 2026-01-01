#pragma once

#include <string>
#include <optional>

namespace PillarEditor {

    class FileDialog
    {
    public:
        // Opens a file dialog for opening a file
        // Returns the selected file path, or empty optional if cancelled
        static std::optional<std::string> OpenFile(const char* filter);
        
        // Opens a file dialog for saving a file
        // Returns the selected file path, or empty optional if cancelled
        static std::optional<std::string> SaveFile(const char* filter);
    };

}
