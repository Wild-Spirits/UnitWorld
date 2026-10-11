#include "EditorCamera.hpp"

#include "imgui.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace Vega
{

    constexpr float kMinDistance = 0.5f;
    constexpr float kMaxPitchDeg = 89.0f;
    constexpr float kZoomStep = 0.85f;
    constexpr float kFlyFastMultiplier = 4.0f;

    void EditorCamera::OnUpdate(Timestep _Timestep, bool _IsViewportHovered)
    {
        ImGuiIO& io = ImGui::GetIO();

        // Start a control mode only over the viewport, a started drag continues outside of it
        if (m_ControlMode == ControlMode::kNone && _IsViewportHovered)
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                m_ControlMode = io.KeyAlt ? ControlMode::kZoom : ControlMode::kFly;
            }
            else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && io.KeyAlt)
            {
                m_ControlMode = ControlMode::kOrbit;
            }
            else if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
            {
                m_ControlMode = ControlMode::kPan;
            }
        }

        glm::vec2 mouseDelta = { io.MouseDelta.x, io.MouseDelta.y };

        switch (m_ControlMode)
        {
            case ControlMode::kNone: break;
            case ControlMode::kFly: {
                if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
                {
                    m_ControlMode = ControlMode::kNone;
                    break;
                }
                // Look around in place: keep the position, move the focal point
                glm::vec3 position = GetPosition();
                Rotate(mouseDelta);
                m_FocalPoint = position + GetForward() * m_Distance;
                Fly(_Timestep);
                break;
            }
            case ControlMode::kOrbit: {
                if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    m_ControlMode = ControlMode::kNone;
                    break;
                }
                Rotate(mouseDelta);
                break;
            }
            case ControlMode::kPan: {
                if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle))
                {
                    m_ControlMode = ControlMode::kNone;
                    break;
                }
                Pan(mouseDelta);
                break;
            }
            case ControlMode::kZoom: {
                if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
                {
                    m_ControlMode = ControlMode::kNone;
                    break;
                }
                Zoom(-mouseDelta.y * 0.02f);
                break;
            }
        }

        if (io.MouseWheel != 0.0f && (_IsViewportHovered || m_ControlMode != ControlMode::kNone))
        {
            if (m_ControlMode == ControlMode::kFly)
            {
                m_FlySpeed = std::clamp(m_FlySpeed * std::pow(1.2f, io.MouseWheel), 0.05f, 500.0f);
            }
            else
            {
                Zoom(io.MouseWheel);
            }
        }

        if (_IsViewportHovered && m_ControlMode == ControlMode::kNone && !io.WantTextInput &&
            ImGui::IsKeyPressed(ImGuiKey_F, false))
        {
            m_FocalPoint = glm::vec3(0.0f);
        }
    }

    RenderView EditorCamera::GetRenderView(const glm::uvec2& _ViewportSize) const
    {
        glm::mat4 cameraWorld = glm::translate(glm::mat4(1.0f), GetPosition()) * glm::mat4_cast(GetOrientation());
        return MakeRenderView(m_Projection, cameraWorld, _ViewportSize);
    }

    void EditorCamera::Reset() { *this = EditorCamera {}; }

    void DrawGuiCameraProjection(CameraProjection& _Projection)
    {
        const char* projectionTypes[] = { "Perspective", "Orthographic" };
        int projectionTypeIndex = static_cast<int>(_Projection.ProjectionType);
        if (ImGui::Combo("Projection", &projectionTypeIndex, projectionTypes, IM_ARRAYSIZE(projectionTypes)))
        {
            _Projection.ProjectionType = static_cast<CameraProjection::Type>(projectionTypeIndex);
        }

        if (_Projection.ProjectionType == CameraProjection::Type::kPerspective)
        {
            ImGui::DragFloat("FOV Y", &_Projection.FovYDeg, 0.1f, 1.0f, 179.0f);
        }
        else
        {
            ImGui::DragFloat("Ortho Size", &_Projection.OrthoSize, 0.1f, 0.01f, 10000.0f);
        }
        ImGui::DragFloat("Near", &_Projection.Near, 0.01f, 0.001f, _Projection.Far);
        ImGui::DragFloat("Far", &_Projection.Far, 1.0f, _Projection.Near, 100000.0f);
    }

    void EditorCamera::DrawGuiSettings()
    {
        DrawGuiCameraProjection(m_Projection);

        ImGui::Separator();

        ImGui::DragFloat("Fly Speed", &m_FlySpeed, 0.05f, 0.05f, 500.0f);
        ImGui::DragFloat("Rotation Speed", &m_RotationSpeedDeg, 0.01f, 0.01f, 2.0f);

        ImGui::Separator();

        glm::vec3 position = GetPosition();
        ImGui::BeginDisabled();
        ImGui::DragFloat3("Position", &position.x);
        ImGui::DragFloat3("Focal Point", &m_FocalPoint.x);
        ImGui::DragFloat("Distance", &m_Distance);
        ImGui::DragFloat("Yaw", &m_YawDeg);
        ImGui::DragFloat("Pitch", &m_PitchDeg);
        ImGui::EndDisabled();

        if (ImGui::Button("Reset"))
        {
            Reset();
        }

        ImGui::Separator();
        ImGui::TextDisabled("RMB + mouse: look, WASD/QE: move, Shift: fast, wheel: speed");
        ImGui::TextDisabled("Alt + LMB: orbit, MMB: pan, wheel / Alt + RMB: zoom");
        ImGui::TextDisabled("F: focus on origin");
    }

    glm::vec3 EditorCamera::GetPosition() const { return m_FocalPoint - GetForward() * m_Distance; }

    glm::quat EditorCamera::GetOrientation() const
    {
        return glm::angleAxis(glm::radians(m_YawDeg), glm::vec3(0.0f, 1.0f, 0.0f)) *
               glm::angleAxis(glm::radians(m_PitchDeg), glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::vec3 EditorCamera::GetForward() const { return GetOrientation() * glm::vec3(0.0f, 0.0f, -1.0f); }

    glm::vec3 EditorCamera::GetRight() const { return GetOrientation() * glm::vec3(1.0f, 0.0f, 0.0f); }

    glm::vec3 EditorCamera::GetUp() const { return GetOrientation() * glm::vec3(0.0f, 1.0f, 0.0f); }

    void EditorCamera::Rotate(const glm::vec2& _MouseDelta)
    {
        // Mouse right turns right (negative yaw around +Y), mouse down looks down (negative pitch around +X)
        m_YawDeg -= _MouseDelta.x * m_RotationSpeedDeg;
        m_PitchDeg = std::clamp(m_PitchDeg - _MouseDelta.y * m_RotationSpeedDeg, -kMaxPitchDeg, kMaxPitchDeg);
        m_YawDeg = std::fmod(m_YawDeg, 360.0f);
    }

    void EditorCamera::Pan(const glm::vec2& _MouseDelta)
    {
        // Drag the scene with the cursor: mouse right moves the camera left
        float speed = GetPanSpeed();
        m_FocalPoint += (-GetRight() * _MouseDelta.x + GetUp() * _MouseDelta.y) * speed;
    }

    void EditorCamera::Zoom(float _Delta)
    {
        float factor = std::pow(kZoomStep, _Delta);
        float newDistance = m_Distance * factor;
        if (newDistance < kMinDistance)
        {
            // Can't get closer to the focal point: dolly forward together with it
            m_FocalPoint += GetForward() * std::max(m_Distance - newDistance, 0.1f);
            newDistance = m_Distance;
        }
        m_Distance = newDistance;

        if (m_Projection.ProjectionType == CameraProjection::Type::kOrthographic)
        {
            m_Projection.OrthoSize = std::max(m_Projection.OrthoSize * factor, 0.01f);
        }
    }

    void EditorCamera::Fly(Timestep _Timestep)
    {
        glm::vec3 direction { 0.0f };
        if (ImGui::IsKeyDown(ImGuiKey_W))
        {
            direction += GetForward();
        }
        if (ImGui::IsKeyDown(ImGuiKey_S))
        {
            direction -= GetForward();
        }
        if (ImGui::IsKeyDown(ImGuiKey_D))
        {
            direction += GetRight();
        }
        if (ImGui::IsKeyDown(ImGuiKey_A))
        {
            direction -= GetRight();
        }
        if (ImGui::IsKeyDown(ImGuiKey_E))
        {
            direction += glm::vec3(0.0f, 1.0f, 0.0f);
        }
        if (ImGui::IsKeyDown(ImGuiKey_Q))
        {
            direction -= glm::vec3(0.0f, 1.0f, 0.0f);
        }

        if (glm::dot(direction, direction) == 0.0f)
        {
            return;
        }

        float speed = m_FlySpeed * (ImGui::GetIO().KeyShift ? kFlyFastMultiplier : 1.0f);
        // Focal point moves with the camera, so the distance (and orbit pivot) is kept
        m_FocalPoint += glm::normalize(direction) * speed * _Timestep.GetSeconds();
    }

    float EditorCamera::GetPanSpeed() const { return std::max(m_Distance, kMinDistance) * 0.0015f; }

}    // namespace Vega
