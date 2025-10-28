#include "ImGuiLayer.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "Pillar/Application.h"

#include <glad/glad.h>
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

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();

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
        // Example ImGui window
        static bool show = true;
        ImGui::ShowDemoWindow(&show);
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
        Application& app = Application::Get();
        io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), 
                                (float)app.GetWindow().GetHeight());

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
            event.Handled |= event.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
            event.Handled |= event.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
        }

        // Dispatch to specific handlers for custom processing if needed
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(ImGuiLayer::OnMouseButtonPressed));
        dispatcher.Dispatch<MouseButtonReleasedEvent>(BIND_EVENT_FN(ImGuiLayer::OnMouseButtonReleased));
        dispatcher.Dispatch<MouseMovedEvent>(BIND_EVENT_FN(ImGuiLayer::OnMouseMoved));
        dispatcher.Dispatch<MouseScrolledEvent>(BIND_EVENT_FN(ImGuiLayer::OnMouseScrolled));
        dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(ImGuiLayer::OnKeyPressed));
        dispatcher.Dispatch<KeyReleasedEvent>(BIND_EVENT_FN(ImGuiLayer::OnKeyReleased));
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(ImGuiLayer::OnWindowResize));
    }

    bool ImGuiLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
    {
        // ImGui backend handles this automatically via ImGui_ImplGlfw callbacks
        return false;
    }

    bool ImGuiLayer::OnMouseButtonReleased(MouseButtonReleasedEvent& e)
    {
        // ImGui backend handles this automatically via ImGui_ImplGlfw callbacks
        return false;
    }

    bool ImGuiLayer::OnMouseMoved(MouseMovedEvent& e)
    {
        // ImGui backend handles this automatically via ImGui_ImplGlfw callbacks
        return false;
    }

    bool ImGuiLayer::OnMouseScrolled(MouseScrolledEvent& e)
    {
        // ImGui backend handles this automatically via ImGui_ImplGlfw callbacks
        return false;
    }

    bool ImGuiLayer::OnKeyPressed(KeyPressedEvent& e)
    {
        // ImGui backend handles this automatically via ImGui_ImplGlfw callbacks
        return false;
    }

    bool ImGuiLayer::OnKeyReleased(KeyReleasedEvent& e)
    {
        // ImGui backend handles this automatically via ImGui_ImplGlfw callbacks
        return false;
    }

    bool ImGuiLayer::OnWindowResize(WindowResizeEvent& e)
    {
        // ImGui will handle this automatically in Begin()
        return false;
    }

}