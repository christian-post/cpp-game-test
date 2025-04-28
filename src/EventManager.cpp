#include "EventManager.h"
#include "Utils.h"


EventManager::EventManager() : events{}, listeners{}, delayedEvents{} {
	// TODO
}

void EventManager::pushEvent(const std::string& key, std::any value) {
	events[key] = value;
	// also call listeners
	auto it = listeners.find(key);
	if (it != listeners.end()) {
		TraceLog(LOG_INFO, "calling the event listeners for %s", key.c_str());
		for (auto& callback : it->second) {
			callback(value);
		}
	}
	else {
		TraceLog(LOG_WARNING, "No event listeners for %s", key.c_str());
	}
}

void EventManager::pushDelayedEvent(const std::string& key, float delay, std::any value, std::function<void()> callback) {
	delayedEvents.push_back({ key, delay, value, callback });
}


void EventManager::addListener(const std::string& key, std::function<void(std::any)> callback) {
	TraceLog(LOG_INFO, "Adding a event listener for %s", key.c_str());
	listeners[key].push_back(callback);
	TraceLog(LOG_INFO, "Listener count for %s: %zu", key.c_str(), listeners[key].size());
}

void EventManager::pushConditionalEvent(std::function<bool()> condition, std::function<void()> callback) {
	conditionalEvents.push_back({ condition, callback });
}

void EventManager::removeListeners(const std::string& key) {
	listeners.erase(key);
}

void EventManager::update(float deltaTime) {
	for (auto it = delayedEvents.begin(); it != delayedEvents.end(); ) {
		it->timeRemaining -= deltaTime;
		if (it->timeRemaining <= 0.0f) {
			if (it->callback) {
				it->callback();
			}
			pushEvent(it->key, it->value);
			it = delayedEvents.erase(it);
		}
		else {
			++it;
		}
	}

	for (auto it = conditionalEvents.begin(); it != conditionalEvents.end(); ) {
		if (it->condition()) {
			it->callback();
			it = conditionalEvents.erase(it);
		}
		else {
			++it;
		}
	}
}
