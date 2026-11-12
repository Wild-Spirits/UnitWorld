#include "ImGuiViewportTitleBar.hpp"

#include "Vega/Core/Application.hpp"
#include "Vega/ImGui/Fonts/ImGuiFontDefinesIconsFA.inl"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>

namespace Vega
{

    void DrawViewportTitleBarButtons(bool* _pOpen, bool* _pIsDetached)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        const bool isDetached = !window->DockIsActive && window->Viewport != ImGui::GetMainViewport();
        if (_pIsDetached != nullptr)
        {
            *_pIsDetached = isDetached;
        }

        if (!isDetached || window->SkipItems || (window->Flags & ImGuiWindowFlags_NoTitleBar) != 0)
        {
            return;
        }

        // Alt+F4 / native close: Begin consumes this only when p_open is passed to it
        if (window->Viewport->PlatformRequestClose && _pOpen != nullptr)
        {
            *_pOpen = false;
        }

        // macOS keeps the native window buttons
        if (Application::Get().GetWindow()->IsTitleBarHasNativeButtons())
        {
            return;
        }

        GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(window->Viewport->PlatformHandle);
        if (glfwWindow == nullptr)
        {
            return;
        }

        const ImRect titleBar = window->TitleBarRect();
        const float buttonWidth = titleBar.GetHeight() * 1.2f;
        const ImVec2 buttonSize = { buttonWidth, titleBar.GetHeight() };
        const bool isMaximized = glfwGetWindowAttrib(glfwWindow, GLFW_MAXIMIZED) == GLFW_TRUE;

        // The buttons are real items (they block the caption drag), placed outside the inner clip rect of Begin
        const ImVec2 cursorPosBackup = ImGui::GetCursorPos();
        ImGui::PushClipRect(titleBar.Min, titleBar.Max, false);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, 0x00000000);

        ImGui::SetCursorPos(ImVec2(titleBar.Max.x - buttonWidth * 3.0f, titleBar.Min.y) - window->Pos);
        if (ImGui::Button(ICON_FA_MINUS, buttonSize))
        {
            glfwIconifyWindow(glfwWindow);
        }
        ImGui::SameLine();
        if (ImGui::Button(isMaximized ? ICON_FA_WINDOW_RESTORE : ICON_FA_WINDOW_MAXIMIZE, buttonSize))
        {
            if (isMaximized)
            {
                glfwRestoreWindow(glfwWindow);
            }
            else
            {
                glfwMaximizeWindow(glfwWindow);
            }
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xff4f4eff);
        if (ImGui::Button(ICON_FA_XMARK, buttonSize))
        {
            if (_pOpen != nullptr)
            {
                *_pOpen = false;
            }
        }
        ImGui::PopStyleColor();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::PopClipRect();
        ImGui::SetCursorPos(cursorPosBackup);

        // The first click of the double click started an ImGui move: cancel it, then maximize/restore
        if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() &&
            titleBar.Contains(ImGui::GetMousePos()))
        {
            ImGui::StopMouseMovingWindow();
            // Consume the click like a NoMove window: otherwise ImGui starts a move at the end of this frame
            // and drags the window back from the maximized state while the button is held
            ImGui::SetActiveID(window->MoveId, window);
            if (isMaximized)
            {
                glfwRestoreWindow(glfwWindow);
            }
            else
            {
                glfwMaximizeWindow(glfwWindow);
            }
        }
    }

}    // namespace Vega
