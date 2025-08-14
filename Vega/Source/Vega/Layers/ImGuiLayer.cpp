#include "ImGuiLayer.hpp"

#include <fstream>

#include <imgui.h>
#include <imgui_internal.h>

#include "Vega/Core/Application.hpp"
#include "Vega/Core/Inputs.hpp"
#include "Vega/Events/EventDispatcher.hpp"
#include "Vega/ImGui/Fonts/ImGuiFontDefinesIconsFA.inl"
#include "Vega/ImGui/Fonts/ImGuiFontDefinesIconsFABrands.inl"

#include <backends/imgui_impl_glfw.h>
#include <glm/glm.hpp>

#define USE_CUSTOM_FONT true

namespace Vega
{

    const std::string regFont = "Assets/Fonts/Roboto/Roboto-Regular.ttf";

    // NOTE: Font viewer: https://fontdrop.info/#/?darkmode=true
    const std::string faFontsFolder = "Assets/Fonts/FA/";

    ImGuiLayer::ImGuiLayer() : GuiLayer("ImGuiLayer") { }

    void ImGuiLayer::OnAttach(Ref<EventManager> _EventManager)
    {
        Application& app = Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow()->GetNativeWindow());
        Ref<RendererBackend> rendererBackend = app.GetRendererBackend();
        m_ImGuiImpl = rendererBackend->CreateImGuiImpl();

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;    // Enable Keyboard Controls
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;    // Enable Docking

        // TODO: ImGuiConfigFlags_ViewportsEnable
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform Windows

        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

#if USE_CUSTOM_FONT

        SetFontSizeByMonitorScale(app.GetWindow()->GetMonitorScale());
        m_ChangeSize = true;
        ChangeFontSize(false);

#endif

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular
        // ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        SetDarkThemeColors();

        ImGuiMemAllocFunc allocFunc;
        ImGuiMemFreeFunc freeFunc;
        void* userData;
        ImGui::GetAllocatorFunctions(&allocFunc, &freeFunc, &userData);

        switch (rendererBackend->GetAPI())
        {
            case RendererBackendApi::kVulkan: ImGui_ImplGlfw_InitForVulkan(window, true); break;
            case RendererBackendApi::kDirectX: VEGA_CORE_ASSERT(false, "DirectX not supported yet!"); break;
            case RendererBackendApi::kOpenGL: ImGui_ImplGlfw_InitForOpenGL(window, true); break;
            default: VEGA_CORE_ASSERT(false, "Unknown RendererAPI!");
        }

        m_ImGuiImpl->SetImGuiGlobalData(ImGui::GetCurrentContext(), &allocFunc, &freeFunc, &userData);
        m_ImGuiImpl->Init();
    }

    void ImGuiLayer::OnDetach()
    {
        m_ImGuiImpl->Shutdown();
        ImGui_ImplGlfw_Shutdown();

        ImGui::DestroyContext();
    }

    // void ImGuiLayer::OnEvent(Event& e)
    //{
    //     EventDispatcher dispatcher(e);
    //     dispatcher.Dispatch<WindowMonitorScaleChangedEvent>([&](WindowMonitorScaleChangedEvent& event) {
    //         SetFontSizeByMonitorScale(event.GetScale());
    //         m_ChangeSize = true;
    //         return false;
    //     });
    //     if (m_BlockEvents)
    //     {
    //         ImGuiIO& io = ImGui::GetIO();
    //         e.Handled |= e.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
    //         e.Handled |= e.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
    //     }
    // }

    void ImGuiLayer::OnUpdate() { }

    void ImGuiLayer::OnGuiRender() { }

    void ImGuiLayer::BeginGuiFrame()
    {
        ChangeFontSize(true);

        m_ImGuiImpl->NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::EndGuiFrame()
    {
        ImGuiIO& io = ImGui::GetIO();

        ImGui::Render();
        m_ImGuiImpl->RenderFrame();

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            m_ImGuiImpl->BackupCurrentWindowContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            m_ImGuiImpl->RestoreCurrentWindowContext();
        }
    }

    void ImGuiLayer::SetDarkThemeColors()
    {
        auto& colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_WindowBg] = ImVec4 { 0.1f, 0.105f, 0.11f, 1.0f };

        // Headers
        colors[ImGuiCol_Header] = ImVec4 { 0.2f, 0.205f, 0.21f, 1.0f };
        colors[ImGuiCol_HeaderHovered] = ImVec4 { 0.3f, 0.305f, 0.31f, 1.0f };
        colors[ImGuiCol_HeaderActive] = ImVec4 { 0.15f, 0.1505f, 0.151f, 1.0f };

        // Buttons
        colors[ImGuiCol_Button] = ImVec4 { 0.2f, 0.205f, 0.21f, 1.0f };
        colors[ImGuiCol_ButtonHovered] = ImVec4 { 0.3f, 0.305f, 0.31f, 1.0f };
        colors[ImGuiCol_ButtonActive] = ImVec4 { 0.15f, 0.1505f, 0.151f, 1.0f };

        // Frame BG
        colors[ImGuiCol_FrameBg] = ImVec4 { 0.2f, 0.205f, 0.21f, 1.0f };
        colors[ImGuiCol_FrameBgHovered] = ImVec4 { 0.3f, 0.305f, 0.31f, 1.0f };
        colors[ImGuiCol_FrameBgActive] = ImVec4 { 0.15f, 0.1505f, 0.151f, 1.0f };

        // Tabs
        colors[ImGuiCol_Tab] = ImVec4 { 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TabHovered] = ImVec4 { 0.38f, 0.3805f, 0.381f, 1.0f };
        colors[ImGuiCol_TabActive] = ImVec4 { 0.28f, 0.2805f, 0.281f, 1.0f };
        colors[ImGuiCol_TabUnfocused] = ImVec4 { 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4 { 0.2f, 0.205f, 0.21f, 1.0f };

        // Title
        colors[ImGuiCol_TitleBg] = ImVec4 { 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TitleBgActive] = ImVec4 { 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4 { 0.15f, 0.1505f, 0.151f, 1.0f };
    }

    void ImGuiLayer::ChangeFontSize(bool _NeedUpdateFontTexture)
    {
#if USE_CUSTOM_FONT
        if (m_ChangeSize)
        {
            ImGuiIO& io = ImGui::GetIO();

            ImFontConfig config;

            bool isFontExists = m_Fonts.contains(m_FontSize);

            ImFont* font = nullptr;

            if (isFontExists)
            {
                font = m_Fonts[m_FontSize];
            }
            else
            {
                float fontSize = static_cast<float>(m_FontSize);
                font = io.Fonts->AddFontFromFileTTF(regFont.c_str(), fontSize, &config,
                                                    io.Fonts->GetGlyphRangesCyrillic());

                config.MergeMode = true;
                config.GlyphMinAdvanceX = fontSize;    // Use if you want to make the icon monospaced
                // config.DstFont = font;

                static const ImWchar iconRangesFa[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
                std::filesystem::path farPath =
                    std::filesystem::path(faFontsFolder) / std::filesystem::path(FONT_ICON_FILE_NAME_FAR);
                std::filesystem::path fasPath =
                    std::filesystem::path(faFontsFolder) / std::filesystem::path(FONT_ICON_FILE_NAME_FAS);
                io.Fonts->AddFontFromFileTTF(farPath.string().c_str(), fontSize, &config, iconRangesFa);
                io.Fonts->AddFontFromFileTTF(fasPath.string().c_str(), fontSize, &config, iconRangesFa);
                // io.Fonts->AddFontFromFileTTF(regFont.c_str(), fontSize, &config, );

                static const ImWchar iconRangesFab[] = { ICON_MIN_FAB, ICON_MAX_FAB, 0 };
                std::filesystem::path fabPath =
                    std::filesystem::path(faFontsFolder) / std::filesystem::path(FONT_ICON_FILE_NAME_FAB);
                io.Fonts->AddFontFromFileTTF(fabPath.string().c_str(), fontSize, &config, iconRangesFab);

                m_Fonts[m_FontSize] = font;
            }
            io.FontDefault = font;

            if (_NeedUpdateFontTexture && !isFontExists)
            {
                io.Fonts->Build();
                m_ImGuiImpl->RecreateFontTexture();
            }

            VEGA_CORE_TRACE("ImGui font size: {}", m_FontSize);

            m_ChangeSize = false;
        }

#endif
    }

    uint32_t ImGuiLayer::GetActiveWidgetID() const { return GImGui->ActiveId; }

}    // namespace Vega
