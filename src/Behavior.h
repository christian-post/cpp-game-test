#pragma once
#include "raylib.h"
#include <string>
#include <memory>
#include <vector>

class Game;
class Sprite;
struct Emitter;
struct Particle;

enum direction {
	RIGHT,
	UP,
	LEFT,
	DOWN
};

enum weaponType {
	SWING = 0,
	POKE = 1,
	SHOOT = 2,
	WHACK = 3
};

class Behavior {
public:
	virtual ~Behavior() = default;
	virtual void update(float deltaTime) = 0;
	virtual void draw() {};
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
	ChaseBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other, float aggroDist, float minDist, float deAggroDist);
	void update(float deltaTime) override;
	
private:
	Game& game;
	std::weak_ptr<Sprite> self;
	std::weak_ptr<Sprite> other;
	float aggroDist;
	float minDist;
	float deAggroDist;
	bool isChasing = false;
};

class WeaponBehavior : public Behavior {
public:
	WeaponBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> owner, float lifetime, weaponType type);
	void update(float deltaTime) override;

private:
	Game& game;
	std::weak_ptr<Sprite> self;
	std::weak_ptr<Sprite> owner;
	float lifetime;
	float originalLifetime;
	weaponType type;
	bool shaken = false;
};

class DeathBehavior : public Behavior {
public:
	DeathBehavior(Game& game, std::shared_ptr<Sprite> self, float lifetime);
	void update(float deltaTime) override;

private:
	Game& game;
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

class CollectItemBehavior : public Behavior {
	// used for consumable sprites that heal the player
public:
	CollectItemBehavior(
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
	uint32_t state = 0;
	float maxLifetime = 2.0f; 
	float lifetime = maxLifetime;
};

class DialogueBehavior : public Behavior {
public:
	DialogueBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> player, std::vector<std::string> dialogTexts, std::string voice);
	void update(float deltaTime) override;

private:
	Game& game;
	std::weak_ptr<Sprite> self;
	std::weak_ptr<Sprite> player;
	std::vector<std::string> dialogTexts;
	std::string voice;
	size_t currentTextIndex = 0;
	bool triggered = false;
	bool collided = false;
};

class TradeItemBehavior : public Behavior {
public:
	TradeItemBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> player, std::string name, uint32_t price);
	void update(float deltaTime) override;
	void draw() override;

private:
	Game& game;
	std::weak_ptr<Sprite> self;
	std::weak_ptr<Sprite> player;
	std::string name;
	uint32_t price;
	bool triggered = false;
	bool collided = false;
};

class ProjectileBehavior : public Behavior {
public:
	ProjectileBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, bool steer = false);
	void update(float deltaTime) override;

private:
	Game& game;
	std::weak_ptr<Sprite> self;
	std::weak_ptr<Sprite> target;
	bool steer;
	Vector2 direction = { 0.0f, 0.0f };
};

class ShootBehavior : public Behavior {
public:
	ShootBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target);
	void update(float deltaTime) override;

private:
	Game& game;
	std::weak_ptr<Sprite> self;
	std::weak_ptr<Sprite> target;
	float timer = 0.0f;
	// TODO change in constructor
	float interval = 2.0f;
};


class EmitterBehavior : public Behavior {
public:
	EmitterBehavior(Game& game, std::shared_ptr<Sprite> self, std::unique_ptr<Emitter> emitter, std::unique_ptr<Particle> prototype);
	void update(float deltaTime) override;
	void draw() override;

private:
	Game& game;
	std::weak_ptr<Sprite> self;
	std::unique_ptr<Emitter> emitter;
	std::unique_ptr<Particle> prototype;
};

class ChestBehavior : public Behavior {
public:
	ChestBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> player, const std::string& itemName, uint32_t itemAmount);
	void update(float deltaTime) override;
	void draw() override;

private:
	Game& game;
	std::weak_ptr<Sprite> self;
	std::weak_ptr<Sprite> player;
	std::string itemName; 
	uint32_t itemAmount;
	bool triggered = false;
	bool collided = false;
	bool showItem = false;
	Rectangle interactionRect = Rectangle{ 0.0f, 0.0f ,0.0f, 0.0f };
};

class OpenLockBehavior : public Behavior {
public:
	OpenLockBehavior(Game& game, std::shared_ptr<Sprite> door, std::shared_ptr<Sprite> player);
	void update(float deltaTime) override;

private:
	Game& game;
	std::weak_ptr<Sprite> door;
	std::weak_ptr<Sprite> player;
	bool triggered = false;
	bool collided = true;
	Rectangle interactionRect = Rectangle{ 0.0f, 0.0f ,0.0f, 0.0f };
};