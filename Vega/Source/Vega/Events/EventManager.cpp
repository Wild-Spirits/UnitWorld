#include "EventManager.hpp"

#include "Vega/Core/Assert.hpp"

namespace LM
{

    void EventManager::Subscribe(EventType _EventType, Scope<IEventHandlerWrapper>&& _Handler, EventId _EventId)
    {
        if (_EventId)
        {
            auto subscribers = m_SubscribersByEventId.find(_EventType);

            if (subscribers != m_SubscribersByEventId.end())
            {
                auto& handlersMap = subscribers->second;
                auto handlers = handlersMap.find(_EventId);
                if (handlers != handlersMap.end())
                {
                    handlers->second.emplace_back(std::move(_Handler));
                    return;
                }
            }
            m_SubscribersByEventId[_EventType][_EventId].emplace_back(std::move(_Handler));
        }
        else
        {
            auto subscribers = m_Subscribers.find(_EventType);
            if (subscribers != m_Subscribers.end())
            {
                auto& handlers = subscribers->second;
                for (auto& it : handlers)
                {
                    if (it->GetType() == _Handler->GetType())
                    {
                        LM_ASSERT(false, "Attempting to double-register callback");
                        return;
                    }
                }
                handlers.emplace_back(std::move(_Handler));
            }
            else
            {
                m_Subscribers[_EventType].emplace_back(std::move(_Handler));
            }
        }
    }

    void EventManager::Unsubscribe(EventType _EventType, const std::string& _HandlerName, EventId _EventId)
    {
        if (_EventId)
        {
            auto subscribers = m_SubscribersByEventId.find(_EventType);
            if (subscribers != m_SubscribersByEventId.end())
            {
                auto& handlersMap = subscribers->second;
                auto handlers = handlersMap.find(_EventId);
                if (handlers != handlersMap.end())
                {
                    auto& callbacks = handlers->second;
                    for (auto it = callbacks.begin(); it != callbacks.end(); ++it)
                    {
                        if (it->get()->GetType() == _HandlerName)
                        {
                            it = callbacks.erase(it);
                            return;
                        }
                    }
                }
            }
        }
        else
        {
            auto handlersIt = m_Subscribers.find(_EventType);
            if (handlersIt != m_Subscribers.end())
            {
                auto& handlers = handlersIt->second;
                for (auto it = handlers.begin(); it != handlers.end(); ++it)
                {
                    if (it->get()->GetType() == _HandlerName)
                    {
                        it = handlers.erase(it);
                        return;
                    }
                }
            }
        }
    }

    void EventManager::TriggerEvent(const Event& _Event, EventId _EventId)
    {
        for (auto& handler : m_Subscribers[_Event.GetEventType()])
        {
            handler->Exec(_Event);
        }

        auto& handlersMap = m_SubscribersByEventId[_Event.GetEventType()];
        auto handlers = handlersMap.find(_EventId);
        if (handlers != handlersMap.end())
        {
            auto& callbacks = handlers->second;
            for (auto it = callbacks.begin(); it != callbacks.end();)
            {
                auto& handler = *it;
                handler->Exec(_Event);
                if (handler->IsDestroyOnSuccess())
                {
                    it = callbacks.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    void EventManager::QueueEvent(Scope<Event>&& _Event, EventId _EventId)
    {
        m_EventsQueue.emplace_back(std::move(_Event), _EventId);
    }

    void EventManager::DispatchEvents()
    {
        for (auto eventIt = m_EventsQueue.begin(); eventIt != m_EventsQueue.end();)
        {
            if (!eventIt->first.get()->IsHandled)
            {
                TriggerEvent(*eventIt->first.get(), eventIt->second);
                eventIt = m_EventsQueue.erase(eventIt);
            }
            else
            {
                ++eventIt;
            }
        }
    }

}    // namespace LM
