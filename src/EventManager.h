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

	struct TimedEvent {
		std::string key;
		float timeRemaining;
		std::any value;
		std::function<void()> callback;
	};

	std::vector<TimedEvent> delayedEvents;

public:
	EventManager();
	void pushEvent(const std::string& key, std::any value);
	void pushDelayedEvent(const std::string& key, float delay, std::any value, std::function<void()> callback = nullptr);
	void addListener(const std::string& key, std::function<void(std::any)> callback);
	void removeListeners(const std::string& key);
	void update(float deltaTime); // used to advance timers

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