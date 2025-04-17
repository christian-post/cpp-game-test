#pragma once
#include "raylib.h"
#include <string>


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
};

class WatchBehavior : public Behavior {
public:
	WatchBehavior(Sprite& self, Sprite& target);
	void update(float deltaTime) override;

private:
	Sprite& self;
	Sprite& target;
};


class RandomWalkBehavior : public Behavior {
public:
	RandomWalkBehavior(Sprite& self);
	void update(float deltaTime) override;

private:
	Sprite& self;
	float waitTimer = 0.0f;
	Vector2 walkTarget;
	bool hasWalkTarget;
};


class ChaseBehavior : public Behavior {
public:
	ChaseBehavior(Sprite& self, Sprite& other, float aggroDist, float minDist, float deAggroDist);
	void update(float deltaTime) override;

private:
	Sprite& self;
	Sprite& other;
	float aggroDist; // triggers the chasing
	float minDist; // the sprite stops moving when it is this close to the other 
	float deAggroDist; // the sprite stops chasing 
	bool isChasing = false;
};


class WeaponBehavior : public Behavior {
	// used for weapons that are being held by a sprite
public:
	WeaponBehavior(Sprite& self, Sprite& owner, float lifetime);
	void update(float deltaTime) override;

private:
	Sprite& self;
	Sprite& owner;
	float lifetime;
	float originalLifetime;
	bool done = false;
};


class DeathBehavior : public Behavior {
	// death animation
public:
	DeathBehavior(Sprite& self, float lifetime);
	void update(float deltaTime) override;

private:
	Sprite& self;
	float lifetime;
	float maxLifetime;
	bool done = false;
	const Shader* shader;
};


struct TeleportEvent {
	std::string targetMap;
	Vector2 targetPos;
};

class TeleportBehavior : public Behavior {
public:
	TeleportBehavior(
		Game& game, Sprite& self, Sprite& other, 
		const std::string& targetMap, Vector2 targetPos
	);
	void update(float deltaTime) override;

private:
	Game& game;
	Sprite& self;
	Sprite& other;
	std::string targetMap;
	Vector2 targetPos;
	bool done = false;
};