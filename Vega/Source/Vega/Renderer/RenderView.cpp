#include "RenderView.hpp"

namespace Vega
{

    Frustum Frustum::FromViewProjection(const glm::mat4& _ViewProjection)
    {
        // Gribb-Hartmann plane extraction. glm is column-major, so row i is (m[0][i], m[1][i], m[2][i], m[3][i])
        auto row = [&](int _Index) {
            return glm::vec4(_ViewProjection[0][_Index], _ViewProjection[1][_Index], _ViewProjection[2][_Index],
                             _ViewProjection[3][_Index]);
        };

        glm::vec4 row0 = row(0);
        glm::vec4 row1 = row(1);
        glm::vec4 row2 = row(2);
        glm::vec4 row3 = row(3);

        Frustum result;
        result.Planes[kLeft] = row3 + row0;
        result.Planes[kRight] = row3 - row0;
        result.Planes[kBottom] = row3 + row1;
        result.Planes[kTop] = row3 - row1;
        // Depth range [0, 1]: near plane is z >= 0
        result.Planes[kNear] = row2;
        result.Planes[kFar] = row3 - row2;

        for (glm::vec4& plane : result.Planes)
        {
            plane /= glm::length(glm::vec3(plane));
        }

        return result;
    }

    bool Frustum::IsSphereInside(const glm::vec3& _Center, float _Radius) const
    {
        for (const glm::vec4& plane : Planes)
        {
            if (glm::dot(glm::vec3(plane), _Center) + plane.w < -_Radius)
            {
                return false;
            }
        }

        return true;
    }

}    // namespace Vega
