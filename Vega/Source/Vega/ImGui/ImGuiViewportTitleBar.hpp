#pragma once

namespace Vega
{

    // Draws minimize/maximize/close buttons inside the built-in title bar of a detached (secondary viewport)
    // panel window and handles the caption double click (maximize/restore). The caption drag itself stays with
    // ImGui, so docking keeps working. Call right after ImGui::Begin of the panel, even when Begin returns false.
    // _pOpen receives false when the panel is closed; _pIsDetached (optional) receives the detachment state for
    // the next frame's window flags. Merged floating dock nodes keep the standard ImGui title bar.
    void DrawViewportTitleBarButtons(bool* _pOpen, bool* _pIsDetached = nullptr);

}    // namespace Vega
