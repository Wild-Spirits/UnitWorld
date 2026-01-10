#pragma once

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

            virtual void OnUpdate(Scene* _Scene) = 0;

            virtual void OnRender(Scene* _Scene) = 0;
        };

    }    // namespace SceneSystems

}    // namespace Vega
