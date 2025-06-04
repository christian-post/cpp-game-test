#include "Behavior.h"
#include "Sprite.h"
#include "Game.h"
#include "InGame.h"
#include "Commands.h"
#include "Controls.h"
#include "Utils.h"
#include "ItemData.h"
#include <any>
#include <cmath>
#include <array>
#include <vector>

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

ChaseBehavior::ChaseBehavior(Game& game, std::shared_ptr<Sprite> sprite, std::shared_ptr<Sprite> targetSprite, float aggroDist, float minDist, float deAggroDist)
	: game{ game }, self{ sprite }, other{ targetSprite }, aggroDist{ aggroDist }, minDist{ minDist }, deAggroDist{ deAggroDist } {
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
	// seperation behavior between enemies
	// TODO: does this scale well with deltaTime?
	for (auto& sprite : game.sprites) {
		if (!sprite->isEnemy) continue;

		Vector2 sum = { 0, 0 };
		int count = 0;
		float desiredSeparation = sprite->rect.width / 2.0f;

		for (auto& other : game.sprites) {
			if (other != sprite && other->isEnemy) {
				float dx = sprite->position.x - other->position.x;
				float dy = sprite->position.y - other->position.y;
				float distSq = dx * dx + dy * dy;
				if (distSq < desiredSeparation * desiredSeparation) {
					float dist = std::sqrt(distSq);
					Vector2 diff = { dx / dist, dy / dist };
					float mag = 1.0f / dist;
					diff.x *= mag;
					diff.y *= mag;
					sum.x += diff.x;
					sum.y += diff.y;
					count++;
				}
			}
		}
		if (count > 0) {
			sum.x /= count;
			sum.y /= count;

			float mag = std::sqrt(sum.x * sum.x + sum.y * sum.y);
			sum.x = sum.x / mag;
			sum.y = sum.y / mag;

			sprite->acc.x += (sum.x - sprite->acc.x);
			sprite->acc.y += (sum.y - sprite->acc.y);
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
			game.eventManager.pushDelayedEvent("teleportStart", 0.0f, nullptr, [this]() {
				game.eventManager.pushEvent("teleport", std::any(TeleportEvent{ targetMap, targetPos }));
				game.playSound("bookPlace1");
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
					game.eventManager.pushEvent("itemAdded", std::string("coin")); // TODO: testing this here
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

DialogueBehavior::DialogueBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> player, std::vector<std::string> dialogTexts)
	: game{ game }, self{ self }, player{ player }, dialogTexts{ std::move(dialogTexts) } {
}

void DialogueBehavior::update(float deltaTime) {
	if (triggered) return;
	if (auto s = self.lock(), p = player.lock(); s && p) {
		if (CheckCollisionRecs(s->rect, p->rect) && (game.buttonsDown & CONTROL_ACTION1)) {
			triggered = true;
			if (auto scene = dynamic_cast<InGame*>(game.getScene("InGame"))) {
				game.cutsceneManager.queueCommand(new Command_Textbox(game, dialogTexts[currentTextIndex]));
				game.cutsceneManager.queueCommand(new Command_Callback([this]() {
					game.eventManager.pushDelayedEvent("resetDialogTrigger", 0.1f, nullptr, [this]() {
						if (currentTextIndex < dialogTexts.size() - 1)
							++currentTextIndex;
						triggered = false;
						});
					}));
			}
		}
	}
}

TradeItemBehavior::TradeItemBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> player, std::string name, uint32_t price) 
	: game{ game }, self{ self }, player{ player }, name{ name }, price{ price } {
}

void TradeItemBehavior::update(float deltaTime) {
	if (triggered) return;
	// TODO: show the price next to the item

	if (auto s = self.lock(), p = player.lock(); s && p) {
		if (CheckCollisionRecs(s->rect, p->rect)) {
			// show the coin amount
			if (!collided) {
				game.eventManager.pushEvent("showCoinAmount");
				collided = true;
			}
			if (game.buttonsDown & CONTROL_ACTION1) {
				triggered = true;
				// player stands next to the item and tries to buy it

				// check if the item can be afforded
				uint32_t qty = game.inventory.getItemQuantity("coin");

				if (qty >= price) {
					game.eventManager.pushEvent("addItem", std::make_any<std::pair<std::string, uint32_t>>(name, 1));
					game.eventManager.pushEvent("removeItem", std::make_any<std::pair<std::string, uint32_t>>("coin", price));
					done = true;
					game.playSound("cash");
					game.cutsceneManager.queueCommand(new Command_Textbox(game, "Thanks for your purchase."));
					game.cutsceneManager.queueCommand(new Command_Callback([this]() {
						game.eventManager.pushDelayedEvent("resetDialogTrigger", 0.1f, nullptr, [this]() {
							triggered = false;
							});
						}));
				}
				else {
					game.cutsceneManager.queueCommand(new Command_Textbox(game, "You can't afford this item."));
					game.cutsceneManager.queueCommand(new Command_Callback([this]() {
						game.eventManager.pushDelayedEvent("resetDialogTrigger", 0.1f, nullptr, [this]() {
							triggered = false;
							});
						}));
				}
			}
		}
		else {
			if (collided) {
				collided = false;
				done = false;
				game.eventManager.pushEvent("hideCoinAmount");
			}
		}
	}
}

void TradeItemBehavior::draw() {
	// draw the coin amount needed to buy this item
	if (auto s = self.lock()) {
		int x = (int)s->position.x - 4;
		int y = (int)s->position.y + 16;
		const auto& coinTex = game.loader.getTextures("itemDropCoin_idle")[0];
		DrawTexture(coinTex, x, y, WHITE);
		std::string priceText = "x" + std::to_string(price);
		DrawText(priceText.c_str(), x + 8, y, 10, WHITE);
	}
}