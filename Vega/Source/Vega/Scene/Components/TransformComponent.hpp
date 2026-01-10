#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Vega::Components
{

    struct TransformDirtyComponent
    {
    };

    struct TransformComponent
    {
        glm::vec3 Position { 0.0f, 0.0f, 0.0f };
        glm::quat Rotation { 1.0f, 0.0f, 0.0f, 0.0f };
        glm::vec3 Scale { 1.0f, 1.0f, 1.0f };

        glm::mat4 GetTransformMatrix() const
        {
            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), Position);
            glm::mat4 rotationMatrix = glm::toMat4(Rotation);
            glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), Scale);

            return translationMatrix * rotationMatrix * scaleMatrix;
        }
    };

}    // namespace Vega::Components
