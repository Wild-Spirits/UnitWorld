#include "OpenGlImGuiImpl.hpp"

#include "Vega/Core/Application.hpp"

#include <backends/imgui_impl_opengl3.h>

// TODO: Remove GLFW dependency
#include <GLFW/glfw3.h>

namespace Vega
{

    void OpenGlImGuiImpl::Init()
    {

        Application& app = Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow()->GetNativeWindow());

        m_CurrentWindowContext = window;

        // Setup Platform/Renderer bindings
        ImGui_ImplOpenGL3_Init("#version 450");
    }

    void OpenGlImGuiImpl::NewFrame() { ImGui_ImplOpenGL3_NewFrame(); }

    void OpenGlImGuiImpl::RenderFrame()
    {
        ImGuiIO& io = ImGui::GetIO();
        Application& app = Application::Get();
        io.DisplaySize = ImVec2((float)app.GetWindow()->GetWidth(), (float)app.GetWindow()->GetHeight());

        // Rendering
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void OpenGlImGuiImpl::Shutdown() { ImGui_ImplOpenGL3_Shutdown(); }

    void OpenGlImGuiImpl::RecreateFontTexture()
    {
        // ImGui_ImplOpenGL3_DestroyFontsTexture();
        // ImGui_ImplOpenGL3_CreateFontsTexture();
    }

    void OpenGlImGuiImpl::BackupCurrentWindowContext() { GLFWwindow* m_CurrentWindowContext = glfwGetCurrentContext(); }

    void OpenGlImGuiImpl::RestoreCurrentWindowContext() { glfwMakeContextCurrent(m_CurrentWindowContext); }

}    // namespace Vega
