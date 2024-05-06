#pragma once

#include "Event.hpp"
#include "Vega/Core/MouseCodes.hpp"

namespace LM
{

    class MouseMovedEvent : public Event
    {
    public:
        MouseMovedEvent(float _X, float _Y) : m_MouseX(_X), m_MouseY(_Y) { }

        float GetX() const { return m_MouseX; }
        float GetY() const { return m_MouseY; }

        std::string ToString() const override
        {
            return "MouseMovedEvent: " + std::to_string(m_MouseX) + ", " + std::to_string(m_MouseY);
        }

        EVENT_CLASS_TYPE(MouseMoved)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
    protected:
        float m_MouseX;
        float m_MouseY;
    };

    class MouseScrolledEvent : public Event
    {
    public:
        MouseScrolledEvent(float _OffsetX, float _OffsetY) : m_OffsetX(_OffsetX), m_OffsetY(_OffsetY) { }

        float GetOffsetX() const { return m_OffsetX; }
        float GetOffsetY() const { return m_OffsetY; }

        std::string ToString() const override
        {
            return "MouseScrolledEvent: " + std::to_string(m_OffsetX) + ", " + std::to_string(m_OffsetY);
        }

        EVENT_CLASS_TYPE(MouseScrolled)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
    protected:
        float m_OffsetX;
        float m_OffsetY;
    };

    class MouseButtonEvent : public Event
    {
    public:
        MouseCode GetMouseButton() const { return m_MouseButton; }
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryMouseButton | EventCategoryInput)
    protected:
        MouseButtonEvent(MouseCode _MouseButton) : m_MouseButton(_MouseButton) { }

    protected:
        MouseCode m_MouseButton;
    };

    class MouseButtonPressedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonPressedEvent(MouseCode _MouseButton) : MouseButtonEvent(_MouseButton) { }

        std::string ToString() const override { return "MouseButtonPressedEvent: " + std::to_string(m_MouseButton); }

        EVENT_CLASS_TYPE(MouseButtonPressed)
    };

    class MouseButtonReleasedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonReleasedEvent(MouseCode _MouseButton) : MouseButtonEvent(_MouseButton) { }

        std::string ToString() const override { return "MouseButtonReleasedEvent: " + std::to_string(m_MouseButton); }

        EVENT_CLASS_TYPE(MouseButtonReleased)
    };

}    // namespace LM
