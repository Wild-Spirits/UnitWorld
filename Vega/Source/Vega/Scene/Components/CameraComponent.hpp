#pragma once

#include "Vega/Renderer/Camera.hpp"

namespace Vega::Components
{

    /**
     * @brief Game camera. Placement comes from WorldTransformComponent, the camera looks along local -Z.
     *
     * The game (and Play mode in the editor) renders from the primary camera, see Scene::GetPrimaryCameraView.
     * Use Scene::SetPrimaryCamera to keep only one primary camera.
     */
    struct CameraComponent
    {
        CameraProjection Projection;
        bool IsPrimary = false;
    };

}    // namespace Vega::Components
