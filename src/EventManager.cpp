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
		for (auto& callback : it->second) {
			callback(value);
		}
	}
}

void EventManager::pushDelayedEvent(const std::string& key, float delay, std::any value, std::function<void()> callback) {
	delayedEvents.push_back({ key, delay, value, callback });
}


void EventManager::addListener(const std::string& key, std::function<void(std::any)> callback) {
	listeners[key].push_back(callback);
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
}