#include "ImGuiLayer.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include "Platform/Desktop/Core/GLFWTitleBar.hpp"
#include "Vega/Core/Application.hpp"
#include "Vega/Events/EventDispatcher.hpp"
#include "Vega/ImGui/Fonts/ImGuiFontDefinesIconsFA.inl"
#include "Vega/ImGui/Fonts/ImGuiFontDefinesIconsFABrands.inl"

#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <glm/glm.hpp>

#include <unordered_map>
#include <utility>

#define USE_CUSTOM_FONT true

namespace Vega
{

    const std::string regFont = "Assets/Fonts/Roboto/Roboto-Regular.ttf";

    // NOTE: Font viewer: https://fontdrop.info/#/?darkmode=true
    const std::string faFontsFolder = "Assets/Fonts/FA/";

    namespace
    {

        // Minimum size of a secondary viewport window: the WM resize must keep the title bar with the window
        // buttons and a bit of content visible. ImGui's own resize minimum (style.WindowMinSize) doesn't apply
        // to WM-driven resizes, so it is enforced by GLFW size limits (WM_NORMAL_HINTS / WM_GETMINMAXINFO).
        constexpr int kViewportMinWidth = 200;
        constexpr int kViewportMinHeight = 100;

        // Secondary ImGui viewports get a resize-only title bar: the caption drag stays with ImGui so that
        // docking back into the main window (or into another viewport) keeps working
        std::unordered_map<GLFWwindow*, Scope<GLFWTitleBar>> s_ViewportTitleBars;

        void (*s_OriginalCreateWindow)(ImGuiViewport*) = nullptr;
        void (*s_OriginalDestroyWindow)(ImGuiViewport*) = nullptr;

        void ViewportCreateWindow(ImGuiViewport* _Viewport)
        {
            // The backend owns PlatformUserData/PlatformHandle and the input callbacks, keep them intact
            s_OriginalCreateWindow(_Viewport);

            if (!Application::Get().GetIsHasCutsomTitleBar())
            {
                return;
            }

            GLFWwindow* window = static_cast<GLFWwindow*>(_Viewport->PlatformHandle);
            if (window == nullptr)
            {
                return;
            }

            glfwSetWindowSizeLimits(window, kViewportMinWidth, kViewportMinHeight, GLFW_DONT_CARE, GLFW_DONT_CARE);

            if (Scope<GLFWTitleBar> titleBar = GLFWTitleBar::Create(window, TitleBarMode::kResizeOnly))
            {
                s_ViewportTitleBars[window] = std::move(titleBar);
            }
        }

        void ViewportDestroyWindow(ImGuiViewport* _Viewport)
        {
            // Destroy the title bar while the platform window is still alive (it restores the wndproc on Windows)
            if (GLFWwindow* window = static_cast<GLFWwindow*>(_Viewport->PlatformHandle))
            {
                s_ViewportTitleBars.erase(window);
            }

            s_OriginalDestroyWindow(_Viewport);
        }

        // A maximized window must not slide around: hand the just-started caption drag to the window manager,
        // which restores the window and keeps dragging it natively (like the main window's caption)
        void HandOffMaximizedWindowMove()
        {
            ImGuiWindow* movingWindow = GImGui->MovingWindow;
            if (movingWindow == nullptr || !movingWindow->ViewportOwned ||
                movingWindow->Viewport == ImGui::GetMainViewport())
            {
                return;
            }

            GLFWwindow* window = static_cast<GLFWwindow*>(movingWindow->Viewport->PlatformHandle);
            if (window == nullptr || glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_FALSE)
            {
                return;
            }

            auto titleBarIt = s_ViewportTitleBars.find(window);
            if (titleBarIt == s_ViewportTitleBars.end())
            {
                return;
            }

            ImGui::StopMouseMovingWindow();
            titleBarIt->second->StartCaptionMove();

            // Consume the click like the resize border driver: no ImGui move until the (synthetic) button release
            ImGui::FocusWindow(movingWindow);
            ImGui::SetActiveID(movingWindow->MoveId, movingWindow);
        }

    }    // namespace

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
        io.ConfigDpiScaleFonts = true;

        // io.ConfigViewportsNoDecoration = false;
        // io.ConfigViewportsNoDefaultParent = true;

        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

        LoadCustomFonts();

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

        // Detached panels move from the caption only: a resize border click must not start an ImGui move
        io.ConfigWindowsMoveFromTitleBarOnly = true;

        ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
        if (platformIO.Platform_CreateWindow != nullptr && platformIO.Platform_DestroyWindow != nullptr)
        {
            s_OriginalCreateWindow = platformIO.Platform_CreateWindow;
            s_OriginalDestroyWindow = platformIO.Platform_DestroyWindow;
            platformIO.Platform_CreateWindow = ViewportCreateWindow;
            platformIO.Platform_DestroyWindow = ViewportDestroyWindow;
        }

        m_ImGuiImpl->SetImGuiGlobalData(ImGui::GetCurrentContext(), &allocFunc, &freeFunc, &userData);
        m_ImGuiImpl->Init();
    }

    void ImGuiLayer::OnDetach()
    {
        // Title bars are destroyed while their GLFW windows are still alive; the renderer shutdown below
        // destroys the platform windows itself
        s_ViewportTitleBars.clear();

        m_ImGuiImpl->Shutdown();
        ImGui_ImplGlfw_Shutdown();

        ImGui::DestroyContext();

        s_OriginalCreateWindow = nullptr;
        s_OriginalDestroyWindow = nullptr;
    }

    void ImGuiLayer::OnUpdate(Timestep _Timestep) { }

    void ImGuiLayer::OnGuiRender() { }

    bool ImGuiLayer::BeginGuiFrame()
    {
        ImGuiIO& io = ImGui::GetIO();

        Application& app = Application::Get();

        // Before the backend sets the ImGui cursor: the resize cursor of a viewport title bar must win
        for (auto& [window, titleBar] : s_ViewportTitleBars)
        {
            // A minimized window has a stale cursor pos, don't let it freeze the cursor state
            if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) == GLFW_FALSE)
            {
                titleBar->OnUpdate();
            }
        }

        if (app.GetIsHasCutsomTitleBar())
        {
            // The platform shows the resize cursor there
            bool isOnResizeBorder = app.GetWindow()->IsCursorOnResizeBorder();
            for (const auto& [window, titleBar] : s_ViewportTitleBars)
            {
                if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) == GLFW_FALSE)
                {
                    isOnResizeBorder |= titleBar->IsCursorOnResizeBorder();
                }
            }

            if (isOnResizeBorder)
            {
                io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
            }
            else
            {
                io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
            }
        }

        m_ImGuiImpl->NewFrame();
        ImGui_ImplGlfw_NewFrame();
        // Before ImGui::NewFrame applies the move: a maximized viewport window is dragged by the WM instead
        HandOffMaximizedWindowMove();
        ImGui::NewFrame();

        // After NewFrame, so the click state is up to date: start a WM resize from a viewport resize border
        if (ImGui::IsMouseClicked(0))
        {
            for (auto& [window, titleBar] : s_ViewportTitleBars)
            {
                // GLFW_HOVERED: the window-local cursor pos alone would also hit overlapped windows
                if (glfwGetWindowAttrib(window, GLFW_HOVERED) == GLFW_FALSE || !titleBar->IsCursorOnResizeBorder())
                {
                    continue;
                }

                titleBar->OnMouseButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);

                // Consume the click like a NoMove window: the top border lies inside the title bar rect,
                // otherwise ImGui would start moving the window on top of the WM resize
                if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(window))
                {
                    if (ImGuiWindow* imWindow = static_cast<ImGuiViewportP*>(viewport)->Window)
                    {
                        ImGui::FocusWindow(imWindow);
                        ImGui::SetActiveID(imWindow->MoveId, imWindow);
                    }
                }
            }
        }

        return true;
    }

    void ImGuiLayer::EndGuiFrame()
    {
        ImGui::Render();
        m_ImGuiImpl->RenderFrame();
    }

    void ImGuiLayer::OnExternalViewportsRender()
    {
        ImGuiIO& io = ImGui::GetIO();

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

        // Title: match the main window caption (see EditorLayer::DrawGuiTitlebar)
        colors[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(0xff1b1b1b);
        colors[ImGuiCol_TitleBgActive] = ImGui::ColorConvertU32ToFloat4(0xff1b1b1b);
        colors[ImGuiCol_TitleBgCollapsed] = ImGui::ColorConvertU32ToFloat4(0xff1b1b1b);
    }

    void ImGuiLayer::LoadCustomFonts()
    {
        ImGuiIO& io = ImGui::GetIO();

        ImFontConfig config;

        ImFont* font = nullptr;

        float fontSize = static_cast<float>(m_FontSize);
        font = io.Fonts->AddFontFromFileTTF(regFont.c_str(), fontSize, &config, io.Fonts->GetGlyphRangesCyrillic());

#if USE_CUSTOM_FONT
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

        static const ImWchar iconRangesFab[] = { ICON_MIN_FAB, ICON_MAX_FAB, 0 };
        std::filesystem::path fabPath =
            std::filesystem::path(faFontsFolder) / std::filesystem::path(FONT_ICON_FILE_NAME_FAB);
        io.Fonts->AddFontFromFileTTF(fabPath.string().c_str(), fontSize, &config, iconRangesFab);
#endif

        io.FontDefault = font;
    }

    uint32_t ImGuiLayer::GetActiveWidgetID() const { return GImGui->ActiveId; }

}    // namespace Vega
