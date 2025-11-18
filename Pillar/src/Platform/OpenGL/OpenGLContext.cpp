#include "Platform/OpenGL/OpenGLContext.h"
#include "Pillar/Logger.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Pillar {

    OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
        : m_WindowHandle(windowHandle)
    {
        PIL_CORE_ASSERT(windowHandle, "Window handle is null!");
    }

    void OpenGLContext::Init()
    {
        PIL_CORE_INFO("Initializing OpenGL Context");
        
        glfwMakeContextCurrent(m_WindowHandle);
        
        // Initialize GLAD
        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        PIL_CORE_ASSERT(status, "Failed to initialize GLAD!");
        
        PIL_CORE_INFO("OpenGL Context created successfully");
    }

    void OpenGLContext::SwapBuffers()
    {
        glfwSwapBuffers(m_WindowHandle);
    }

}
