#include "Behavior.h"
#include "Sprite.h"
#include "Game.h"
#include "Utils.h"
#include <any>
#include <cmath>

WatchBehavior::WatchBehavior(std::shared_ptr<Sprite> sprite, std::shared_ptr<Sprite> targetSprite)
	: self{ sprite }, target{ targetSprite } {
}

void WatchBehavior::update(float deltaTime) {
	if (auto s = self.lock(), t = target.lock(); s && t) {
		if (t->position.x < s->position.x) {
			s->lastDirection = LEFT;
		}
		else {
			s->lastDirection = RIGHT;
		}
	}
}

RandomWalkBehavior::RandomWalkBehavior(std::shared_ptr<Sprite> sprite)
	: self{ sprite } {
	if (auto s = self.lock()) {
		walkTarget = s->position;
	}
	hasWalkTarget = false;
}

void RandomWalkBehavior::update(float deltaTime) {
	if (auto s = self.lock()) {
		if (waitTimer > 0.0f) {
			waitTimer -= deltaTime;
			return;
		}
		float dx = walkTarget.x - s->position.x;
		float dy = walkTarget.y - s->position.y;
		float distSq = dx * dx + dy * dy;

		if (distSq > 2.0f * 2.0f) {
			float dist = sqrtf(distSq);
			s->acc.x = dx / dist;
			s->acc.y = dy / dist;
		}
		else {
			s->acc = { 0.0f, 0.0f };
			s->vel = { 0.0f, 0.0f };
			hasWalkTarget = false;
		}

		if (!hasWalkTarget) {
			int tries = 10;
			while (tries-- > 0) {
				direction dir = static_cast<direction>(GetRandomValue(LEFT, DOWN));
				int tiles = GetRandomValue(1, 4);
				float offset = tiles * 16.0f;
				Vector2 candidate = s->position;
				switch (dir) {
				case UP: candidate.y -= offset; break;
				case LEFT: candidate.x -= offset; break;
				case DOWN: candidate.y += offset; break;
				case RIGHT: candidate.x += offset; break;
				}
				if (isPathClear(s->rect, candidate, s->game.walls)) {
					walkTarget = candidate;
					hasWalkTarget = true;
					waitTimer = float(rand() % 5 + 1);
					break;
				}
			}
		}
	}
}

ChaseBehavior::ChaseBehavior(std::shared_ptr<Sprite> sprite, std::shared_ptr<Sprite> targetSprite, float aggroDist, float minDist, float deAggroDist)
	: self{ sprite }, other{ targetSprite }, aggroDist{ aggroDist }, minDist{ minDist }, deAggroDist{ deAggroDist } {
}

void ChaseBehavior::update(float deltaTime) {
	if (auto s = self.lock(), o = other.lock(); s && o) {
		Vector2 selfCenter = GetRectCenter(s->rect);
		Vector2 otherCenter = GetRectCenter(o->rect);
		float dx = otherCenter.x - selfCenter.x;
		float dy = otherCenter.y - selfCenter.y;
		float distSq = dx * dx + dy * dy;

		if (!isChasing) {
			if (distSq < aggroDist * aggroDist) {
				isChasing = true;
			}
		}
		else {
			float dist = sqrtf(distSq);
			s->acc.x = dx / dist;
			s->acc.y = dy / dist;
			if (dist > deAggroDist) {
				isChasing = false;
			}
			else if (dist <= minDist) {
				s->acc = { 0.0f, 0.0f };
				s->vel = { 0.0f, 0.0f };
			}
		}
	}
}

WeaponBehavior::WeaponBehavior(Game& game, std::shared_ptr<Sprite> sprite, std::shared_ptr<Sprite> ownerSprite, float lifetime)
	: game{ game }, self {sprite}, owner{ ownerSprite }, lifetime{ lifetime }, originalLifetime{ lifetime } {
	if (auto s = self.lock(), o = owner.lock(); s && o) {
		s->lastDirection = o->lastDirection;
		s->hurtboxOffset.x = (s->lastDirection == LEFT) ? -12.0f : 12.0f;
		// the player character is left handed, change the drawing order of the weapon accordingly
		s->drawLayer = (s->lastDirection == LEFT) ? 1 : -1;
		s->hurtboxOffset.y = 8.0f;
	}
}

void WeaponBehavior::update(float deltaTime) {
	if (auto s = self.lock(), o = owner.lock(); s && o) {
		lifetime -= deltaTime;
		if (lifetime < 0.0f && !done) {
			s->game.eventManager.pushEvent("killWeapon", nullptr);
			done = true;
		}
		s->position.x = o->position.x;
		s->position.y = o->position.y - 8.0f;
		float progress = 1.0f - (lifetime / originalLifetime);
		s->rotationAngle = (s->lastDirection == RIGHT) ? 180.0f * progress : -180.0f * progress;
	}
}

DeathBehavior::DeathBehavior(Game& game, std::shared_ptr<Sprite> sprite, float lifetime)
	: game{ game }, self {sprite}, lifetime{ lifetime }, maxLifetime{ lifetime } {
	if (auto s = self.lock()) {
		shader = &s->game.loader.getShader("crumble");
		game.playSound("creature_die_01");
	}
}

void DeathBehavior::update(float deltaTime) {
	if (auto s = self.lock(); s && !done) {
		float elapsed = maxLifetime - lifetime;
		s->activeShader = ShaderState{ shader, elapsed, maxLifetime, 1 };
		lifetime -= deltaTime;
		if (lifetime < 0.0f) {
			done = true;
			s->visible = false;
		}
	}
}

TeleportBehavior::TeleportBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other, const std::string& targetMap, Vector2 targetPos)
	: game{ game }, self{ self }, other{ other }, targetMap{ targetMap }, targetPos{ targetPos } {
}

void TeleportBehavior::update(float deltaTime) {
	if (auto s = self.lock(), o = other.lock(); s && o && !done) {
		if (CheckCollisionRecs(s->rect, o->rect)) {
			done = true;
			TeleportEvent event{ targetMap, targetPos };
			game.eventManager.pushDelayedEvent("asdf", 0.01f, nullptr, [this, event]() {
				game.eventManager.pushEvent("teleport", std::any(event));
			});
		}
	}
}

HealBehavior::HealBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other, uint32_t amount) 
	: game{ game }, self{ self }, other{ other }, amount{ amount }{ 
}

void HealBehavior::update(float deltaTime) {
	if (auto s = self.lock(), o = other.lock(); s && o && !done) {
		if (CheckCollisionRecs(s->rect, o->rect)) {
			done = true;
			// add the amount to health, cap at maxHealth
			o->health = std::min(o->health + amount, o->maxHealth);
			// play sound
			game.playSound("heart");
			// delete this item
			s->markForDeletion();
		}
	}
}

CollectItemBehavior::CollectItemBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other, const std::string& name, uint32_t amount)
	: game{ game }, self{ self }, other{ other }, name{ name }, amount {
	amount
} {}

void CollectItemBehavior::update(float deltaTime) {
	if (auto s = self.lock(), o = other.lock(); s && o && !done) {
		switch (state) {
			case 0:
			{
				// check collision and collect the item
				if (CheckCollisionRecs(s->rect, o->rect)) {
					// add the item
					game.eventManager.pushEvent("addItem", std::make_any<std::pair<std::string, uint32_t>>(name, amount));
					game.playSound("rupee");
					state++;
				}
				break;
			}
			case 1: {
				// display the item above the player
				s->position.x = o->position.x + (o->rect.width - s->rect.width) / 2.0f;
				// oscillate the y position slightly
				float offset = std::sin((maxLifetime - lifetime) * 10.0f) * 4.0f;
				s->position.y = o->position.y - 20.0f + offset;
				lifetime -= deltaTime;
				if (lifetime < 0.0f) {
					state++;
				}
				break;
			}
			default:
			{
				// delete the item sprite
				done = true;
				s->markForDeletion();
			}
		}
	}
}