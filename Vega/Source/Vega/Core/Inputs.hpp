#pragma once

#include "Vega/Core/KeyCodes.hpp"
#include "Vega/Core/MouseCodes.hpp"

#include <glm/glm.hpp>

namespace LM
{

    class Input
    {
    public:
        static bool IsKeyPressed(KeyCode key);

        static bool IsMouseButtonPressed(MouseCode button);
        static glm::vec2 GetMousePosition();
        static float GetMouseX();
        static float GetMouseY();
    };

}    // namespace LM
