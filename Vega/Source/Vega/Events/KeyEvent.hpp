#pragma once

#include "Event.hpp"
#include "Vega/Core/KeyCodes.hpp"


namespace Vega
{

    class KeyEvent : public Event
    {
    public:
        KeyCode GetKeyCode() const { return m_KeyCode; }

        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

    protected:
        KeyEvent(KeyCode _KeyCode) : m_KeyCode(_KeyCode) { }

    protected:
        KeyCode m_KeyCode;
    };

    class KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(KeyCode _KeyCode, uint16_t _RepeatCount) : KeyEvent(_KeyCode), m_RepeatCount(_RepeatCount) { }

        uint16_t GetRepeatCount() const { return m_RepeatCount; }

        std::string ToString() const override
        {
            return "KeyPressedEvent: " + std::to_string(m_KeyCode) + ", " + std::to_string(m_RepeatCount) + " repeats";
        }

        EVENT_CLASS_TYPE(KeyPressed)
    protected:
        uint16_t m_RepeatCount;
    };

    class KeyReleasedEvent : public KeyEvent
    {
    public:
        KeyReleasedEvent(KeyCode _KeyCode) : KeyEvent(_KeyCode) { }

        std::string ToString() const override { return "KeyReleasedEvent: " + std::to_string(m_KeyCode); }
        EVENT_CLASS_TYPE(KeyReleased)
    };

    class KeyTypedEvent : public KeyEvent
    {
    public:
        KeyTypedEvent(KeyCode _KeyCode) : KeyEvent(_KeyCode) { }

        std::string ToString() const override { return "KeyTypedEvent: " + std::to_string(m_KeyCode); }

        EVENT_CLASS_TYPE(KeyTyped)
    };

}    // namespace Vega
