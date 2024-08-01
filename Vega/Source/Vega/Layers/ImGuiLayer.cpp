#include "ImGuiLayer.hpp"

#include <fstream>

#include <imgui.h>
#include <imgui_internal.h>

#include "Vega/Core/Application.hpp"
#include "Vega/Core/Inputs.hpp"
#include "Vega/Events/EventDispatcher.hpp"
#include "Vega/ImGui/Fonts/ImGuiFontDefinesIconsFA.inl"
#include "Vega/ImGui/Fonts/ImGuiFontDefinesIconsFABrands.inl"

#include <glm/glm.hpp>

#define USE_CUSTOM_FONT true

namespace LM
{

    const std::string regFont = "Assets/Fonts/Roboto/Roboto-Regular.ttf";

    // NOTE: Font viewer: https://fontdrop.info/#/?darkmode=true
    const std::string faFontsFolder = "Assets/Fonts/FA/";

    ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer") { }

    void ImGuiLayer::OnAttach(Ref<EventManager> _EventManager)
    {
        m_ImGuiImpl = ImGuiImpl::Create();

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;    // Enable Keyboard Controls
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform Windows
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

#if USE_CUSTOM_FONT

        SetFontSizeByMonitorScale(Application::Get().GetWindow()->GetMonitorScale());
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

        m_ImGuiImpl->Init();
    }

    void ImGuiLayer::OnDetach()
    {
        m_ImGuiImpl->Shutdown();
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

    void ImGuiLayer::OnUpdate()
    {
        ChangeFontSize(true);

        m_ImGuiImpl->NewFrame();

        ImGui::NewFrame();

        if (ImGui::Begin("Test"))
        {
            ImGui::Text("Hello, world!");
            if (ImGui::Button(ICON_FA_FILE_WORD ICON_FA_BRIDGE ICON_FA_LINUX "Button"))
            {
            }
        }
        ImGui::End();

        ImGui::Render();
    }

    void ImGuiLayer::OnRender() { m_ImGuiImpl->RenderFrame(); }

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

                static const ImWchar icon_ranges_fa[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
                std::filesystem::path farPath =
                    std::filesystem::path(faFontsFolder) / std::filesystem::path(FONT_ICON_FILE_NAME_FAR);
                std::filesystem::path fasPath =
                    std::filesystem::path(faFontsFolder) / std::filesystem::path(FONT_ICON_FILE_NAME_FAS);
                io.Fonts->AddFontFromFileTTF(farPath.string().c_str(), fontSize, &config, icon_ranges_fa);
                io.Fonts->AddFontFromFileTTF(fasPath.string().c_str(), fontSize, &config, icon_ranges_fa);

                static const ImWchar icon_ranges_fab[] = { ICON_MIN_FAB, ICON_MAX_FAB, 0 };
                std::filesystem::path fabPath =
                    std::filesystem::path(faFontsFolder) / std::filesystem::path(FONT_ICON_FILE_NAME_FAB);
                io.Fonts->AddFontFromFileTTF(fabPath.string().c_str(), fontSize, &config, icon_ranges_fab);

                m_Fonts[m_FontSize] = font;
            }
            io.FontDefault = font;

            if (_NeedUpdateFontTexture && !isFontExists)
            {
                io.Fonts->Build();
                m_ImGuiImpl->RecreateFontTexture();
            }

            LM_CORE_TRACE("ImGui font size: {}", m_FontSize);

            m_ChangeSize = false;
        }

#endif
    }

    uint32_t ImGuiLayer::GetActiveWidgetID() const { return GImGui->ActiveId; }

}    // namespace LM
