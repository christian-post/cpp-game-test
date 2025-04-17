#include "Behavior.h"
#include "Sprite.h"
#include "Game.h"
#include "Utils.h"
#include <any>


WatchBehavior::WatchBehavior(Sprite& self, Sprite& target)
	: self{ self }, target{ target } {}

void WatchBehavior::update(float deltaTime) {
	if (target.position.x < self.position.x) {
		self.lastDirection = LEFT;
	}
	else {
		self.lastDirection = RIGHT;
	}
}


RandomWalkBehavior::RandomWalkBehavior(Sprite& self) 
    : self{ self }, walkTarget{ self.position.x, self.position.y }, hasWalkTarget{ false } {
}

void RandomWalkBehavior::update(float deltaTime) {
	if (waitTimer > 0.0f) {
		// do nothing until the timer runs out
		waitTimer -= deltaTime;
		return;
	}
    float dx = walkTarget.x - self.position.x;
    float dy = walkTarget.y - self.position.y;
    float distSq = dx * dx + dy * dy;

    if (distSq > 2.0f * 2.0f) {
        // accelerate towards the target if within a certain number of pixels
        float dist = sqrtf(distSq);
        self.acc.x = dx / dist;
        self.acc.y = dy / dist;
    }
    else {
        self.acc = { 0.0f, 0.0f };
        self.vel = { 0.0f, 0.0f };
        hasWalkTarget = false;
    }

    if (!hasWalkTarget) {
        int tries = 10;
        while (tries-- > 0) {
            direction dir = static_cast<direction>(GetRandomValue(LEFT, DOWN));
            int tiles = GetRandomValue(1, 4);
            float offset = tiles * 16.0f;
            // move the new walkTarget a random number of tiles in one random direction
            Vector2 candidate = self.position;
            switch (dir) {
                case UP: candidate.y -= offset; break;
                case LEFT: candidate.x -= offset; break;
                case DOWN: candidate.y += offset; break;
                case RIGHT: candidate.x += offset; break;
            }
            // check if there is static collision in the way
            if (isPathClear(self.rect, candidate, self.game.walls)) {
                walkTarget = candidate;
                hasWalkTarget = true;
                waitTimer = float(rand() % 5 + 1); // new waiting time between 1 and 5 seconds
                break;
            }
        }
    }
}

ChaseBehavior::ChaseBehavior(Sprite& self, Sprite& other, float aggroDist, float minDist, float deAggroDist) 
    : self{ self }, other{ other }, aggroDist{ aggroDist }, minDist{ minDist }, deAggroDist{ deAggroDist } {
}

void ChaseBehavior::update(float deltaTime) {
    // use the rects' center points as references
    Vector2 selfCenter = GetRectCenter(self.rect);
    Vector2 otherCenter = GetRectCenter(other.rect);
    float dx = otherCenter.x - selfCenter.x;
    float dy = otherCenter.y - selfCenter.y;

    float distSq = dx * dx + dy * dy;
    if (!isChasing) {
        // target is within aggro distance
        if (distSq < aggroDist * aggroDist) {
            isChasing = true;
        } // otherwise do nothing (or another behavior)
    }
    else {
        // sprite is chasing the target
        float dist = sqrtf(distSq);
        self.acc.x = dx / dist;
        self.acc.y = dy / dist;
        // stop if the target is lost
        if (dist > deAggroDist) {
            isChasing = false;
        }
        else if (dist <= minDist) {
            // keep minimum distance
            self.acc = { 0.0f, 0.0f };
            self.vel = { 0.0f, 0.0f };
        }
    }
}


WeaponBehavior::WeaponBehavior(Sprite& self, Sprite& owner, float lifetime) :
    self{ self }, owner{ owner }, lifetime{ lifetime }, originalLifetime{ lifetime } 
{
    self.lastDirection = owner.lastDirection;
    // TODO: change hurtbox offset instead
    if (self.lastDirection == LEFT) {
        self.hurtboxOffset.x = -12.0f;
    }
    else {
        self.hurtboxOffset.x = 12.0f;
    }
    self.hurtboxOffset.y = 8.0f;

}

void WeaponBehavior::update(float deltaTime) {
    lifetime -= deltaTime;
    if (lifetime < 0 && !done) {
        self.game.eventManager.pushEvent("killSword", nullptr);
        done = true; // prevent multiple events, dunno if necessary
    }
    self.position.x = owner.position.x;
    self.position.y = owner.position.y - 8.0f;
    // rotate the weapon
    float progress = 1.0f - (lifetime / originalLifetime);
    self.rotationAngle = (self.lastDirection == RIGHT) ? 180.0f * progress : -180.0f * progress;
}


DeathBehavior::DeathBehavior(Sprite& self, float lifetime) : self{ self }, lifetime{ lifetime }, maxLifetime{ lifetime } {
    shader = &self.game.loader.getShader("crumble");
}

void DeathBehavior::update(float deltaTime) {
    if (!done) {
        float elapsed = maxLifetime - lifetime;
        self.activeShader = ShaderState{ shader, elapsed, maxLifetime, 1 };
    }
    lifetime -= deltaTime;
    if (lifetime < 0 && !done) {
        done = true;
        self.visible = false;
    }
}


TeleportBehavior::TeleportBehavior(Game& game, Sprite& self, Sprite& other, const std::string& targetMap, Vector2 targetPos) : 
    game{ game }, self {self }, other{ other }, targetMap{ targetMap }, targetPos{ targetPos } {
}

void TeleportBehavior::update(float deltaTime) {
    if (!done) {
        if (CheckCollisionRecs(self.rect, other.rect)) {
            done = true;
            TeleportEvent event{ targetMap, targetPos };
            game.eventManager.pushEvent("teleport", std::any(event));
        }
    }
}