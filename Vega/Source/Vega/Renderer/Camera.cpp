#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace Vega
{

    glm::mat4 CameraProjection::GetProjection(float _AspectRatio) const
    {
        switch (ProjectionType)
        {
            case Type::kPerspective:
                return glm::perspectiveRH_ZO(glm::radians(FovYDeg), _AspectRatio, Near, Far);
            case Type::kOrthographic: {
                float halfHeight = OrthoSize * 0.5f;
                float halfWidth = halfHeight * _AspectRatio;
                return glm::orthoRH_ZO(-halfWidth, halfWidth, -halfHeight, halfHeight, Near, Far);
            }
        }

        return glm::mat4(1.0f);
    }

    RenderView MakeRenderView(const CameraProjection& _Projection, const glm::mat4& _CameraWorld,
                              const glm::uvec2& _ViewportSize)
    {
        glm::vec3 position = glm::vec3(_CameraWorld[3]);

        // Drop scale: normalized axes give a pure rotation, so the view is a rigid inverse
        glm::mat3 rotation = glm::mat3(glm::normalize(glm::vec3(_CameraWorld[0])),
                                       glm::normalize(glm::vec3(_CameraWorld[1])),
                                       glm::normalize(glm::vec3(_CameraWorld[2])));
        glm::mat3 inverseRotation = glm::transpose(rotation);

        RenderView result;
        result.View = glm::mat4(inverseRotation);
        result.View[3] = glm::vec4(-(inverseRotation * position), 1.0f);
        result.Projection = _Projection.GetProjection(GetAspectRatio(_ViewportSize));
        result.ViewProjection = result.Projection * result.View;
        result.Position = position;
        result.Near = _Projection.Near;
        result.Far = _Projection.Far;
        result.ViewportSize = _ViewportSize;
        result.ViewFrustum = Frustum::FromViewProjection(result.ViewProjection);

        return result;
    }

    float GetAspectRatio(const glm::uvec2& _ViewportSize)
    {
        if (_ViewportSize.x == 0 || _ViewportSize.y == 0)
        {
            return 1.0f;
        }

        return static_cast<float>(_ViewportSize.x) / static_cast<float>(_ViewportSize.y);
    }

}    // namespace Vega
