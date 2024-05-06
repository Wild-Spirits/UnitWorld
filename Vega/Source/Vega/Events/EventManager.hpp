#pragma once

#include "EventHandler.hpp"

namespace LM
{

    using EventId = uint64_t;

    class EventManager
    {
    public:
        EventManager() = default;
        EventManager(const EventManager&) = delete;
        EventManager(const EventManager&&) = delete;
        const EventManager& operator=(const EventManager&) = delete;
        const EventManager& operator=(const EventManager&&) = delete;

        template <typename EventType>
        inline void Subscribe(const EventHandler<EventType>& _Callback, EventId _EventId = 0,
                              const bool _UnsubscribeOnSuccess = false)
        {
            Scope<IEventHandlerWrapper> handler =
                CreateScope<EventHandlerWrapper<EventType>>(_Callback, _UnsubscribeOnSuccess);
            Subscribe(EventType::GetStaticEventType(), std::move(handler), _EventId);
        }

        template <typename EventType>
        inline void Unsubscribe(const EventHandler<EventType>& _Callback, EventId _EventId = 0)
        {
            const std::string handlerName = _Callback.target_type().name();
            Unsubscribe(EventType::GetStaticEventType(), handlerName, _EventId);
        }

        void QueueEvent(Scope<Event>&& _Event, EventId _EventId = 0);
        void DispatchEvents();

    private:
        void Subscribe(EventType _EventType, Scope<IEventHandlerWrapper>&& _Handler, EventId _EventId);
        void Unsubscribe(EventType _EventType, const std::string& _HandlerName, EventId _EventId);
        void TriggerEvent(const Event& _Event, EventId _EventId);

    private:
        std::vector<std::pair<Scope<Event>, EventId>> m_EventsQueue;
        std::unordered_map<EventType, std::vector<Scope<IEventHandlerWrapper>>> m_Subscribers;
        std::unordered_map<EventType, std::unordered_map<EventId, std::vector<Scope<IEventHandlerWrapper>>>>
            m_SubscribersByEventId;
    };

}    // namespace LM
