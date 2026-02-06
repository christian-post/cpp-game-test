#include "EventManager.h"
#include "Utils.h"


EventManager::EventManager() : events{}, listeners{}, delayedEvents{}
{}

void EventManager::pushEvent(const int key, std::any value)
{
    events[key] = value;
    // also call the listeners
    auto it = listeners.find(key);
    if (it != listeners.end())
    {
        for (auto& callback : it->second)
        {
            callback(value);
        }
    }
}

void EventManager::pushDelayedEvent(const int key, float delay, std::any value, std::function<void()> callback)
{
    delayedEvents.push_back({ key, delay, value, callback });
}

void EventManager::addListener(const int key, std::function<void(std::any)> callback)
{
    listeners[key].push_back(callback);
    TraceLog(LOG_INFO, "Adding an event listener for %s. Listener count: %zu", GetEventNameString(static_cast<EventName>(key)), listeners[key].size());
}

void EventManager::pushConditionalEvent(std::function<bool()> condition, std::function<void()> callback)
{
    conditionalEvents.push_back({ condition, callback });
}

void EventManager::pushRepeatedEvent(const int key, float interval, std::any value, std::function<void()> callback, int numRepeats, std::function<void()> onComplete)
{
    repeatedEvents.push_back({ key, interval, interval, value, callback, numRepeats, onComplete });
}

void EventManager::cancelRepeatedEvent(const int key)
{
    for (auto it = repeatedEvents.begin(); it != repeatedEvents.end(); )
    {
        if (it->key == key)
            it = repeatedEvents.erase(it);
        else
            ++it;
    }
}

void EventManager::removeListeners(const int key)
{
    listeners.erase(key);
}

void EventManager::clearAll()
{
    listeners.clear();
    delayedEvents.clear();
    conditionalEvents.clear();
}

void EventManager::update(float deltaTime)
{
    // collect delayed events to fire
    std::vector<TimedEvent> eventsToFire;

    for (auto it = delayedEvents.begin(); it != delayedEvents.end(); )
    {
        it->timeRemaining -= deltaTime;
        if (it->timeRemaining <= 0.0f)
        {
            eventsToFire.push_back(std::move(*it));
            it = delayedEvents.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // fire collected delayed events
    for (auto& event : eventsToFire)
    {
        if (event.callback)
            event.callback();
        pushEvent(event.key, event.value);
    }

    // collect conditional events to fire
    std::vector<ConditionalEvent> conditionalToFire;

    for (auto it = conditionalEvents.begin(); it != conditionalEvents.end(); )
    {
        if (it->condition())
        {
            conditionalToFire.push_back(std::move(*it));
            it = conditionalEvents.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // fire collected conditional events
    for (auto& event : conditionalToFire)
    {
        event.callback();
    }

    // collect repeated events to fire
    std::vector<std::function<void()>> repeatedCallbacks;
    std::vector<std::pair<int, std::any>> repeatedToFire;
    std::vector<std::function<void()>> completionCallbacks;

    for (auto it = repeatedEvents.begin(); it != repeatedEvents.end(); )
    {
        it->timeRemaining -= deltaTime;
        if (it->timeRemaining <= 0.0f)
        {
            if (it->callback)
                repeatedCallbacks.push_back(it->callback);
            repeatedToFire.push_back({ it->key, it->value });
            it->timeRemaining += it->interval;

            if (--it->repeatsLeft <= 0)
            {
                if (it->onComplete)
                    completionCallbacks.push_back(it->onComplete);
                it = repeatedEvents.erase(it);
                continue;
            }
        }
        ++it;
    }

    // fire repeated event callbacks
    for (size_t i = 0; i < repeatedCallbacks.size(); ++i)
    {
        repeatedCallbacks[i]();
        pushEvent(repeatedToFire[i].first, repeatedToFire[i].second);
    }

    // fire completion callbacks
    for (auto& callback : completionCallbacks)
    {
        callback();
    }
}
