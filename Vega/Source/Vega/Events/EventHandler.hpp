#pragma once

#include "Event.hpp"

#include <functional>

namespace Vega
{

    template <typename EventType>
    using EventHandler = std::function<void(const EventType& _Event)>;

    class IEventHandlerWrapper
    {
    public:
        virtual ~IEventHandlerWrapper() = default;

        void Exec(const Event& _Event) { Call(_Event); }

        virtual std::string GetType() const = 0;
        virtual bool IsDestroyOnSuccess() const = 0;

    private:
        virtual void Call(const Event& _Event) = 0;
    };

    template <typename EventType>
    class EventHandlerWrapper : public IEventHandlerWrapper
    {
    public:
        explicit EventHandlerWrapper(const EventHandler<EventType>& _Handler, const bool _DestroyOnSuccess = false)
            : m_Handler(_Handler),
              m_HandlerType(_Handler.target_type().name()),
              m_DestroyOnSuccess(_DestroyOnSuccess)
        { }

    private:
        void Call(const Event& _Event) override
        {
            if (_Event.GetEventType() == EventType::GetStaticEventType())
            {
                m_Handler(static_cast<const EventType&>(_Event));
            }
        }

        std::string GetType() const override { return m_HandlerType; }
        bool IsDestroyOnSuccess() const { return m_DestroyOnSuccess; }

        EventHandler<EventType> m_Handler;
        const std::string m_HandlerType;
        bool m_DestroyOnSuccess { false };
    };
}    // namespace Vega
