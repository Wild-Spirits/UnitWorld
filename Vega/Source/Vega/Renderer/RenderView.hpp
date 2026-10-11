#pragma once

#include <array>

#include <glm/glm.hpp>

namespace Vega
{

    /**
     * @brief Frustum planes in world space. Plane is (normal.xyz, distance), normal points inside the frustum,
     * so a point is inside when dot(normal, point) + distance >= 0 for all planes.
     */
    struct Frustum
    {
        enum PlaneIndex : size_t
        {
            kLeft = 0,
            kRight,
            kBottom,
            kTop,
            kNear,
            kFar,
            kCount,
        };

        std::array<glm::vec4, kCount> Planes;

        // NOTE: Projection must use Vulkan [0, 1] depth range (perspectiveRH_ZO / orthoRH_ZO)
        static Frustum FromViewProjection(const glm::mat4& _ViewProjection);

        bool IsSphereInside(const glm::vec3& _Center, float _Radius) const;
    };

    /**
     * @brief Everything the renderer needs to draw the scene from some point of view.
     *
     * Built from the editor camera, a CameraComponent or (later) a light for shadow maps. Render code depends only on
     * this struct, not on the source of the view.
     */
    struct RenderView
    {
        glm::mat4 View { 1.0f };
        glm::mat4 Projection { 1.0f };
        glm::mat4 ViewProjection { 1.0f };

        glm::vec3 Position { 0.0f };
        float Near = 0.0f;
        float Far = 0.0f;

        glm::uvec2 ViewportSize { 0 };

        Frustum ViewFrustum;
    };

}    // namespace Vega
