#pragma once

#include "Vega/Core/Timestep.hpp"
#include "Vega/Renderer/RenderView.hpp"

namespace Vega
{

    class Scene;

    namespace SceneSystems
    {

        class SceneSystem
        {
        public:
            virtual ~SceneSystem() = default;

            virtual void Destroy() = 0;

            virtual void OnUpdate(Scene* _Scene, Timestep _Timestep) = 0;

            virtual void OnRender(Scene* _Scene, const RenderView& _View) = 0;
        };

    }    // namespace SceneSystems

}    // namespace Vega
