#pragma once

#include <iostream>
#include <string>

#include "Vega/Core/Base.hpp"

namespace LM
{
    enum class EventType
    {
        None = 0,
        WindowClose,
        WindowResize,
        WindowFocus,
        WindowLostFocus,
        WindowMoved,
        WindowMonitorScaleChanged,
        AppTick,
        AppUpdate,
        AppRender,
        KeyPressed,
        KeyReleased,
        KeyTyped,
        MouseButtonPressed,
        MouseButtonReleased,
        MouseMoved,
        MouseScrolled
    };

    enum EventCategory
    {
        None = 0,
        EventCategoryApplication = BIT(0),
        EventCategoryInput = BIT(1),
        EventCategoryKeyboard = BIT(2),
        EventCategoryMouse = BIT(3),
        EventCategoryMouseButton = BIT(4)
    };

#define EVENT_CLASS_TYPE(type)                                                                                         \
    static EventType GetStaticEventType() { return EventType::type; }                                                  \
    virtual EventType GetEventType() const override { return GetStaticEventType(); }                                   \
    virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category)                                                                                 \
    virtual int GetCategoryFlags() const override { return category; }

    class Event
    {
    public:
        virtual ~Event() = default;

        virtual EventType GetEventType() const = 0;
        virtual const char* GetName() const = 0;
        virtual int GetCategoryFlags() const = 0;
        virtual std::string ToString() const { return GetName(); }

        bool IsInCategory(EventCategory _Category) const { return GetCategoryFlags() & _Category; }

        bool IsHandled = false;
    };

    inline std::ostream& operator<<(std::ostream& _Out, const Event& _Event) { return _Out << _Event.ToString(); }

}    // namespace LM
