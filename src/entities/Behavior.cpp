#include "Behavior.h"
#include "WatchBehavior.h"
#include "RandomWalkBehavior.h"
#include "ChaseBehavior.h"
#include "DialogueBehavior.h"
#include "TradeItemBehavior.h"
#include "ProjectileBehavior.h"
#include "ShootBehavior.h"
#include "ShootBurstBehavior.h"
#include "ShootSpreadBehavior.h"
#include "KiteBehavior.h"
#include "LungeBehavior.h"
#include "EmitterBehavior.h"
#include "OpenLockBehavior.h"
#include "Game.h"
#include "Sprite.h"
#include "Emitter.h"
#include "Utils.h"
#include <iostream>


std::unique_ptr<Behavior> createBehaviorFromJSON(Game& game, std::shared_ptr<Sprite> sprite, const std::string& behaviorKey, const nlohmann::json& behaviorData)
{
    if (behaviorKey == "RandomWalk")
    {
        return std::make_unique<RandomWalkBehavior>(sprite);
    }
    else if (behaviorKey == "Watch")
    {
        std::string targetName = behaviorData.value("watchTarget", "");
        if (game.spriteMap.find(targetName) != game.spriteMap.end())
        {
            return std::make_unique<WatchBehavior>(sprite, game.spriteMap[targetName]);
        }
        else
        {
            TraceLog(LOG_WARNING, "Target \"%s\" not found in spriteMap. Skipping WatchBehavior.", targetName.c_str());
            return nullptr;
        }
    }
    else if (behaviorKey == "Chase")
    {
        std::string targetName = behaviorData.value("chaseTarget", "player");
        float minDist = behaviorData.value("minDist", 20.0f);

        if (game.spriteMap.find(targetName) != game.spriteMap.end())
        {
            return std::make_unique<ChaseBehavior>(game, sprite, game.spriteMap[targetName], minDist);
        }
        else
        {
            TraceLog(LOG_WARNING, "Target \"%s\" not found in spriteMap. Skipping ChaseBehavior.", targetName.c_str());
            return nullptr;
        }
    }
    else if (behaviorKey == "Dialogue")
    {
        std::string textKey = behaviorData.value("dialogue", "");
        if (textKey.length())
        {
            std::vector<std::string> texts = game.loader.getText(textKey);
            std::string voice = behaviorData.value("voice", "tone");
            return std::make_unique<DialogueBehavior>(game, sprite, game.spriteMap["player"], texts, voice);
        }
        return nullptr;
    }
    else if (behaviorKey == "Shoot")
    {
        std::string targetName = behaviorData.value("shootTarget", "player");
        shootingConfig conf;
        conf.projectileKey = behaviorData.value("shootProjectile", conf.projectileKey);
        conf.sound = behaviorData.value("shootSound", conf.sound);
        conf.damage = behaviorData.value("shootDamage", conf.damage);
        conf.shootInterval = behaviorData.value("shootInterval", 2.0f);
        conf.speed = behaviorData.value("shootSpeed", 20.0f);
        conf.emitterKey = behaviorData.value("emitterKey", "");
        conf.emitterKey = behaviorData.value("projectileTrailEmitterKey", "");
        conf.emitterKey = behaviorData.value("projectileImpactEmitterKey", "");

        if (game.spriteMap.find(targetName) != game.spriteMap.end())
        {
            return std::make_unique<ShootBehavior>(game, sprite, game.spriteMap[targetName], conf);
        }
        else
        {
            TraceLog(LOG_WARNING, "Target \"%s\" not found in spriteMap. Skipping ShootBehavior.", targetName.c_str());
            return nullptr;
        }
    }
    else if (behaviorKey == "Emitter")
    {
        std::string emitterKey = behaviorData.value("emitter", behaviorData.value("particle", ""));
        if (emitterKey.empty())
        {
            TraceLog(LOG_WARNING, "No emitter/particle key specified for Emitter behavior");
            return nullptr;
        }
        // check for overrides in behavior data
        const nlohmann::json& overrides = behaviorData.contains("emitterData")
            ? behaviorData.at("emitterData")
            : nlohmann::json::object();
        std::shared_ptr<Emitter> emitter = createEmitter(game, emitterKey, overrides);
        return std::make_unique<EmitterBehavior>(game, sprite, emitter);
    }
    else if (behaviorKey == "Kite")
    {
        std::string targetName = behaviorData.value("kiteTarget", "player");
        float orbitDistance = behaviorData.value("orbitDistance", 80.0f);
        float moveSpeed = behaviorData.value("moveSpeed", 1.5f);

        if (game.spriteMap.find(targetName) != game.spriteMap.end())
        {
            return std::make_unique<KiteBehavior>(game, sprite, game.spriteMap[targetName], orbitDistance, moveSpeed);
        }
        else
        {
            TraceLog(LOG_WARNING, "Target \"%s\" not found in spriteMap. Skipping KiteBehavior.", targetName.c_str());
            return nullptr;
        }
    }
    else if (behaviorKey == "ShootBurst")
    {
        std::string targetName = behaviorData.value("shootTarget", "player");
        shootingConfig conf;
        conf.projectileKey = behaviorData.value("shootProjectile", conf.projectileKey);
        conf.sound = behaviorData.value("shootSound", conf.sound);
        conf.damage = behaviorData.value("shootDamage", conf.damage);
        conf.speed = behaviorData.value("shootSpeed", 20.0f);
        conf.emitterKey = behaviorData.value("emitterKey", "");
        conf.projectileTrailEmitterKey = behaviorData.value("projectileTrailEmitterKey", "");
        conf.projectileImpactEmitterKey = behaviorData.value("projectileImpactEmitterKey", "");
        uint32_t burstCount = behaviorData.value("burstCount", 3);
        float burstDelay = behaviorData.value("burstDelay", 0.3f);

        if (game.spriteMap.find(targetName) != game.spriteMap.end())
        {
            return std::make_unique<ShootBurstBehavior>(game, sprite, game.spriteMap[targetName], conf, burstCount, burstDelay);
        }
        else
        {
            TraceLog(LOG_WARNING, "Target \"%s\" not found in spriteMap. Skipping ShootBurstBehavior.", targetName.c_str());
            return nullptr;
        }
    }
    else if (behaviorKey == "ShootSpread")
    {
        std::string targetName = behaviorData.value("shootTarget", "player");
        shootingConfig conf;
        conf.projectileKey = behaviorData.value("shootProjectile", conf.projectileKey);
        conf.sound = behaviorData.value("shootSound", conf.sound);
        conf.damage = behaviorData.value("shootDamage", conf.damage);
        conf.speed = behaviorData.value("shootSpeed", 20.0f);
        conf.emitterKey = behaviorData.value("emitterKey", "");
        conf.projectileTrailEmitterKey = behaviorData.value("projectileTrailEmitterKey", "");
        conf.projectileImpactEmitterKey = behaviorData.value("projectileImpactEmitterKey", "");
        uint32_t projectileCount = behaviorData.value("projectileCount", 3);
        float spreadAngle = behaviorData.value("spreadAngle", 0.5f);

        if (game.spriteMap.find(targetName) != game.spriteMap.end())
        {
            return std::make_unique<ShootSpreadBehavior>(game, sprite, game.spriteMap[targetName], conf, projectileCount, spreadAngle);
        }
        else
        {
            TraceLog(LOG_WARNING, "Target \"%s\" not found in spriteMap. Skipping ShootSpreadBehavior.", targetName.c_str());
            return nullptr;
        }
    }
    else if (behaviorKey == "Lunge")
    {
        std::string targetName = behaviorData.value("lungeTarget", "player");
        float lungeSpeed = behaviorData.value("lungeSpeed", 30.0f);
        uint32_t jumpForce = behaviorData.value("jumpForce", 600);

        if (game.spriteMap.find(targetName) != game.spriteMap.end())
        {
            return std::make_unique<LungeBehavior>(game, sprite, game.spriteMap[targetName], lungeSpeed, jumpForce);
        }
        else
        {
            TraceLog(LOG_WARNING, "Target \"%s\" not found in spriteMap. Skipping LungeBehavior.", targetName.c_str());
            return nullptr;
        }
    }
    else
    {
        TraceLog(LOG_WARNING, "Unknown behavior type: %s", behaviorKey.c_str());
        return nullptr;
    }
}


void addBehaviorsToSprite(Game& game, std::shared_ptr<Sprite> sprite, const std::vector<std::string>& behaviors, const nlohmann::json& behaviorData)
{
    for (const auto& key : behaviors)
    {
        auto behavior = createBehaviorFromJSON(game, sprite, key, behaviorData);
        if (behavior)
            sprite->addBehavior(std::move(behavior));
    }
}