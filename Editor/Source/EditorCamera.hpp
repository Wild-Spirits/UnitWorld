#pragma once

#include "Vega/Core/Timestep.hpp"
#include "Vega/Renderer/Camera.hpp"
#include "Vega/Renderer/RenderView.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Vega
{

    // Shared by the editor camera settings and the CameraComponent props panel
    void DrawGuiCameraProjection(CameraProjection& _Projection);

    /**
     * @brief Editor-only camera, it is not an entity and is not saved with the scene.
     *
     * Orbit model: the camera looks at FocalPoint from Distance. Fly mode moves the focal point together with the
     * camera, so both modes can be mixed freely.
     *
     * Controls (only when the viewport is hovered, a started drag continues outside of it):
     * - Fly: hold RMB to look around, WASD to move, Q/E down/up, Shift to speed up, wheel changes fly speed;
     * - Orbit: Alt + LMB to rotate around the focal point, MMB (or Alt + MMB) to pan, Alt + RMB or wheel to zoom;
     * - F focuses on the world origin (TODO: focus on the selected entity).
     */
    class EditorCamera
    {
    public:
        // Reads input through ImGui IO, so call it from OnGuiRender
        void OnUpdate(Timestep _Timestep, bool _IsViewportHovered);

        RenderView GetRenderView(const glm::uvec2& _ViewportSize) const;

        void Reset();

        void DrawGuiSettings();

        bool IsControlling() const { return m_ControlMode != ControlMode::kNone; }

        const glm::vec3& GetFocalPoint() const { return m_FocalPoint; }
        float GetDistance() const { return m_Distance; }
        glm::vec3 GetPosition() const;

    protected:
        enum class ControlMode
        {
            kNone,
            kFly,
            kOrbit,
            kPan,
            kZoom,
        };

        glm::quat GetOrientation() const;
        glm::vec3 GetForward() const;
        glm::vec3 GetRight() const;
        glm::vec3 GetUp() const;

        void Rotate(const glm::vec2& _MouseDelta);
        void Pan(const glm::vec2& _MouseDelta);
        void Zoom(float _Delta);
        void Fly(Timestep _Timestep);

        float GetPanSpeed() const;

    protected:
        CameraProjection m_Projection;

        glm::vec3 m_FocalPoint { 0.0f, 0.0f, 0.0f };
        float m_Distance = 3.0f;
        float m_YawDeg = 0.0f;
        float m_PitchDeg = 0.0f;

        float m_FlySpeed = 3.0f;
        float m_RotationSpeedDeg = 0.2f;

        ControlMode m_ControlMode = ControlMode::kNone;
    };

}    // namespace Vega
