#include "FileDialog.h"
#include <Windows.h>
#include <commdlg.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include "Pillar/Application.h"

namespace PillarEditor {

    std::optional<std::string> FileDialog::OpenFile(const char* filter)
    {
        OPENFILENAMEA ofn;
        CHAR szFile[260] = { 0 };
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        
        // Get native window handle from GLFW
        GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(Pillar::Application::Get().GetWindow().GetNativeWindow());
        ofn.hwndOwner = glfwGetWin32Window(glfwWindow);
        
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn) == TRUE)
        {
            return ofn.lpstrFile;
        }
        
        return std::nullopt;
    }

    std::optional<std::string> FileDialog::SaveFile(const char* filter)
    {
        OPENFILENAMEA ofn;
        CHAR szFile[260] = { 0 };
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        
        // Get native window handle from GLFW
        GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(Pillar::Application::Get().GetWindow().GetNativeWindow());
        ofn.hwndOwner = glfwGetWin32Window(glfwWindow);
        
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
        
        // Set default extension
        ofn.lpstrDefExt = "json";

        if (GetSaveFileNameA(&ofn) == TRUE)
        {
            return ofn.lpstrFile;
        }
        
        return std::nullopt;
    }

}
