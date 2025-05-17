#pragma once
#include "raylib.h"
#include <string>
#include <memory>

class Game;
class Sprite;

enum direction {
	LEFT,
	RIGHT,
	UP,
	DOWN
};

class Behavior {
public:
	virtual ~Behavior() = default;
	virtual void update(float deltaTime) = 0;
	bool done = false;
};

class WatchBehavior : public Behavior {
public:
	WatchBehavior(std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target);
	void update(float deltaTime) override;

private:
	std::weak_ptr<Sprite> self;
	std::weak_ptr<Sprite> target;
};

class RandomWalkBehavior : public Behavior {
public:
	RandomWalkBehavior(std::shared_ptr<Sprite> self);
	void update(float deltaTime) override;

private:
	std::weak_ptr<Sprite> self;
	float waitTimer = 0.0f;
	Vector2 walkTarget = { 0.0f, 0.0f };
	bool hasWalkTarget = false;
};

class ChaseBehavior : public Behavior {
public:
	ChaseBehavior(std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other, float aggroDist, float minDist, float deAggroDist);
	void update(float deltaTime) override;

private:
	std::weak_ptr<Sprite> self;
	std::weak_ptr<Sprite> other;
	float aggroDist;
	float minDist;
	float deAggroDist;
	bool isChasing = false;
};

class WeaponBehavior : public Behavior {
public:
	WeaponBehavior(std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> owner, float lifetime);
	void update(float deltaTime) override;

private:
	std::weak_ptr<Sprite> self;
	std::weak_ptr<Sprite> owner;
	float lifetime;
	float originalLifetime;
};

class DeathBehavior : public Behavior {
public:
	DeathBehavior(std::shared_ptr<Sprite> self, float lifetime);
	void update(float deltaTime) override;

private:
	std::weak_ptr<Sprite> self;
	float lifetime;
	float maxLifetime;
	const Shader* shader = nullptr;
};

struct TeleportEvent {
	std::string targetMap;
	Vector2 targetPos;
};

class TeleportBehavior : public Behavior {
public:
	TeleportBehavior(
		Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other,
		const std::string& targetMap, Vector2 targetPos
	);
	void update(float deltaTime) override;

private:
	Game& game;
	std::weak_ptr<Sprite> self;
	std::weak_ptr<Sprite> other;
	std::string targetMap;
	Vector2 targetPos;
};

class HealBehavior : public Behavior {
	// used for consumable sprites that heal the player
public:
	HealBehavior(
		Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other,
		uint32_t amount
	);
	void update(float deltaTime) override;

private:
	Game& game;
	std::weak_ptr<Sprite> self;
	std::weak_ptr<Sprite> other;
	uint32_t amount;
};

class AddItemBehavior : public Behavior {
	// used for consumable sprites that heal the player
public:
	AddItemBehavior(
		Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other,
		const std::string& name, uint32_t amount
	);
	void update(float deltaTime) override;

private:
	Game& game;
	std::weak_ptr<Sprite> self;
	std::weak_ptr<Sprite> other;
	std::string name;
	uint32_t amount;
};