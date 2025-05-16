#pragma once
#include "raylib.h"
#include <functional>
#include <vector>
#include <unordered_map>
#include <any>
#include <string>
#include <optional>


class EventManager {
private:
	// store a list of events (key : data), data can be of any type or NULL
	std::unordered_map<std::string, std::any> events;
	// store a list of event listeners (callbacks that are triggered by an event with that key)
	std::unordered_map<std::string, std::vector<std::function<void(std::any)>>> listeners;

	// callbacks that fire after a given amount of time
	struct TimedEvent {
		std::string key;
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

public:
	EventManager();
	void pushEvent(const std::string& key, std::any value = std::any{});
	void pushDelayedEvent(const std::string& key, float delay, std::any value, std::function<void()> callback = nullptr);
	void addListener(const std::string& key, std::function<void(std::any)> callback);
	void removeListeners(const std::string& key);
	void update(float deltaTime); // used to advance timers

	void pushConditionalEvent(std::function<bool()> condition, std::function<void()> callback);

	void clearAll();

	// Get all events (for direct iteration in the scene)
	const std::unordered_map<std::string, std::any>& peekEvents() const {
		return events;
	}

	std::unordered_map<std::string, std::any> popEvents() {
		std::unordered_map<std::string, std::any> popped = std::move(events);
		events.clear();
		return popped;
	}

	void clearEvent(const std::string& key) {
		events.erase(key);
	}
};