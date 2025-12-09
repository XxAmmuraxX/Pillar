#include "ImGuiLayer.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "Pillar/Application.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace Pillar {

    ImGuiLayer::ImGuiLayer()
        : Layer("ImGuiLayer")
    {
    }

    ImGuiLayer::~ImGuiLayer()
    {
    }

    void ImGuiLayer::OnAttach()
    {
        PIL_CORE_INFO("ImGuiLayer::OnAttach");

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        
        // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        
        // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        
        // Enable Multi-Viewport / Platform Windows
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        // Configure ImGui ini file for docking layout persistence
        io.IniFilename = "imgui.ini";

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // When viewports are enabled we tweak WindowRounding/WindowBg 
        // so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        Application& app = Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 410");
    }

    void ImGuiLayer::OnDetach()
    {
        PIL_CORE_INFO("ImGuiLayer::OnDetach");

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void ImGuiLayer::OnUpdate(float deltaTime)
    {
        m_Time += deltaTime;
    }

    void ImGuiLayer::OnImGuiRender()
    {
        // ImGuiLayer no longer creates its own dockspace
        // The application layer (EditorLayer, SceneDemoLayer, etc.) should handle dockspace setup
        // This allows different applications to have different layouts
    }

    void ImGuiLayer::Begin()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::End()
    {
        ImGuiIO& io = ImGui::GetIO();

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
    }

    void ImGuiLayer::OnEvent(Event& event)
    {
        if (m_BlockEvents)
        {
            ImGuiIO& io = ImGui::GetIO();
            event.Handled |= event.IsInCategory(EventCategoryMouse) && io.WantCaptureMouse;
            event.Handled |= event.IsInCategory(EventCategoryKeyboard) && io.WantCaptureKeyboard;
        }
    }

    ImGuiContext* ImGuiLayer::GetImGuiContext()
    {
        return ImGui::GetCurrentContext();
    }

}
