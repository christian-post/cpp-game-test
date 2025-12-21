#pragma once
#include "EventNames.h"
#include "raylib.h"
#include <functional>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <any>
#include <string>
#include <optional>


class EventManager {
private:
    // store a list of events (key : data), data can be of any type or NULL
    std::unordered_map<int, std::any> events;
    // store a list of event listeners (callbacks that are triggered by an event with that key)
    std::unordered_map<int, std::vector<std::function<void(std::any)>>> listeners;
    // a list of previous events

    // callbacks that fire after a given amount of time
    struct TimedEvent {
        int key;
        float timeRemaining;
        std::any value;
        std::function<void()> callback;
    };
    std::vector<TimedEvent> delayedEvents;

    // events that allow for arbitrary conditions to be checked
    struct ConditionalEvent {
        std::function<bool()> condition;
        std::function<void()> callback;
    };
    std::vector<ConditionalEvent> conditionalEvents;

    // events that repeat
    struct RepeatedEvent {
        int key;
        float interval;
        float timeRemaining;
        std::any value;
        std::function<void()> callback;
        int repeatsLeft;
        std::function<void()> onComplete;
    };
    std::vector<RepeatedEvent> repeatedEvents;

public:
    EventManager();
    void pushEvent(const int key, std::any value = std::any{});
    void pushDelayedEvent(const int key, float delay, std::any value, std::function<void()> callback = nullptr);
    void pushRepeatedEvent(const int key, float interval, std::any value, std::function<void()> callback, int numRepeats, std::function<void()> onComplete = nullptr);
    void addListener(const int key, std::function<void(std::any)> callback);
    void removeListeners(const int key);
    void cancelRepeatedEvent(const int key);
    void update(float deltaTime); // used to advance timers
    void pushConditionalEvent(std::function<bool()> condition, std::function<void()> callback);
    void clearAll();

    // Get all events (for direct iteration in the scene)
    const std::unordered_map<int, std::any>& peekEvents() const {
        return events;
    }

    std::unordered_map<int, std::any> popEvents() {
        std::unordered_map<int, std::any> popped = std::move(events);
        events.clear();
        return popped;
    } // currently unused

    void clearEvent(const int key) {
        events.erase(key);
    } // currently unused
};