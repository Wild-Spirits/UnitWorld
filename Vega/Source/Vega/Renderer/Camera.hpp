#pragma once

#include "RenderView.hpp"

#include <glm/glm.hpp>

namespace Vega
{

    /**
     * @brief Projection parameters without placement in the world.
     *
     * Shared by the editor camera, CameraComponent and (later) shadow casting lights.
     */
    struct CameraProjection
    {
        enum class Type
        {
            kPerspective,
            kOrthographic,
        };

        Type ProjectionType = Type::kPerspective;

        // Perspective
        float FovYDeg = 45.0f;
        // Orthographic: visible height in world units
        float OrthoSize = 10.0f;

        float Near = 0.1f;
        float Far = 1000.0f;

        // NOTE: Vulkan depth range [0, 1]. Y is not flipped: the Vulkan viewport is already flipped
        glm::mat4 GetProjection(float _AspectRatio) const;
    };

    /**
     * @brief Builds a render view for a camera placed by _CameraWorld (camera looks along its local -Z).
     *
     * Scale of _CameraWorld is ignored, only position and rotation are used.
     */
    RenderView MakeRenderView(const CameraProjection& _Projection, const glm::mat4& _CameraWorld,
                              const glm::uvec2& _ViewportSize);

    float GetAspectRatio(const glm::uvec2& _ViewportSize);

}    // namespace Vega
