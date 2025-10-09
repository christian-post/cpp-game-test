#include "StateMachine.h"
#include "Sprite.h"
#include "Game.h"
#include "Behavior.h"
#include "Utils.h"
#include <algorithm>

const std::string StateMachine::emptyString = "";

StateMachine::StateMachine(Game& game, std::shared_ptr<Sprite> owner) : game(game), owner(owner) {
}

void StateMachine::addState(std::unique_ptr<State> state) {
    std::string name = state->name;
    states[name] = std::move(state);
}

void StateMachine::addBehavior(const std::string& key, std::unique_ptr<Behavior> behavior) {
    behaviors[key] = std::move(behavior);
}

void StateMachine::setInitialState(const std::string& stateName) {
    auto it = states.find(stateName);
    if (it != states.end())
        enterState(it->second.get());
    else
        TraceLog(LOG_ERROR, "Initial state not found: %s", stateName.c_str());
}

void StateMachine::update(float deltaTime) {
    if (!currentState)
        return;

    auto sprite = owner.lock();
    if (!sprite)
        return;

    // Check for transitions
    for (const auto& transition : currentState->transitions) {
        if (transition.canTransition(*sprite)) {
            transitionTo(transition.targetState);
            return; // Only one transition per frame
        }
    }

    // Update only the ACTIVE behaviors
    for (const auto& key : activeBehaviorKeys) {
        auto it = behaviors.find(key);
        if (it != behaviors.end())
            it->second->update(deltaTime);
    }

    // Call state's update callback if it exists
    if (currentState->onUpdate)
        currentState->onUpdate(*sprite, deltaTime);
}

void StateMachine::draw() {
    // TODO is this unecessarily slow? Most behaviors don't have a draw method...
    for (const auto& key : activeBehaviorKeys) {
        auto it = behaviors.find(key);
        if (it != behaviors.end())
            it->second->draw();
    }
}

void StateMachine::transitionTo(const std::string& stateName) {
    auto it = states.find(stateName);
    if (it == states.end()) {
        TraceLog(LOG_ERROR, "Target state not found: %s", stateName.c_str());
        return;
    }

    State* newState = it->second.get();

    if (currentState == newState)
        return; // Already in this state

    std::string oldStateName = currentState ? currentState->name : "null";

    TraceLog(LOG_INFO, "State transition: %s -> %s",
        oldStateName.c_str(),
        newState->name.c_str());

    exitState(currentState);
    enterState(newState);

    // Call state change callback
    if (onStateChange)
        onStateChange(oldStateName, newState->name);
}

void StateMachine::enterState(State* state) {
    if (!state)
        return;

    currentState = state;

    auto sprite = owner.lock();
    if (!sprite)
        return;

    // Simply activate the behaviors for this state
    activateBehaviors(state->activeBehaviorKeys);

    // Call onEnter callback
    if (state->onEnter)
        state->onEnter(*sprite);
}

void StateMachine::exitState(State* state) {
    if (!state)
        return;

    auto sprite = owner.lock();
    if (!sprite)
        return;

    // Call onExit callback
    if (state->onExit)
        state->onExit(*sprite);
}

void StateMachine::activateBehaviors(const std::vector<std::string>& keys) {
    // Just swap which behaviors are active - no creation/destruction!
    activeBehaviorKeys.clear();
    for (const auto& key : keys) {
        if (behaviors.find(key) != behaviors.end())
            activeBehaviorKeys.insert(key);
        else
            TraceLog(LOG_WARNING, "Behavior not found: %s", key.c_str());
    }
}

std::unique_ptr<StateMachine> StateMachine::createFromJSON(
    Game& game,
    std::shared_ptr<Sprite> owner,
    const nlohmann::json& data)
{
    auto stateMachine = std::make_unique<StateMachine>(game, owner);

    //TraceLogLong(LOG_INFO, data.dump(2).c_str());

    if (!data.contains("states"))
    {
        TraceLog(LOG_ERROR, "State machine data missing 'states' field");
        return stateMachine;
    }

    // Create states and their behaviors
    for (const auto& stateData : data["states"])
    {
        std::string stateName = stateData["name"];
        auto state = std::make_unique<State>(stateName);

        // Get this state's behavior data
        nlohmann::json behaviorData = stateData.value("behaviorData", nlohmann::json::object());

        // Create behaviors for this specific state
        if (stateData.contains("behaviors"))
        {
            for (const auto& behaviorKey : stateData["behaviors"])
            {
                std::string behaviorKeyStr = behaviorKey.get<std::string>();
                std::unique_ptr<Behavior> behavior = nullptr;

                if (behaviorKeyStr == "RandomWalk")
                {
                    behavior = std::make_unique<RandomWalkBehavior>(owner);
                }
                else if (behaviorKeyStr == "Watch")
                {
                    std::string targetName = behaviorData.value("watchTarget", "player");
                    if (game.spriteMap.find(targetName) != game.spriteMap.end())
                        behavior = std::make_unique<WatchBehavior>(owner, game.spriteMap[targetName]);
                }
                else if (behaviorKeyStr == "Chase")
                {
                    std::string targetName = behaviorData.value("chaseTarget", "player");
                    float aggroDist = behaviorData.value("aggroDist", 48.0f);
                    float minDist = behaviorData.value("minDist", 2.0f);
                    float deAggroDist = behaviorData.value("deAggroDist", 64.0f);

                    if (game.spriteMap.find(targetName) != game.spriteMap.end())
                    {
                        behavior = std::make_unique<ChaseBehavior>(
                            game, owner, game.spriteMap[targetName],
                            aggroDist, minDist, deAggroDist);
                    }
                }
                else if (behaviorKeyStr == "Shoot")
                {
                    std::string targetName = behaviorData.value("shootTarget", "player");
                    shootingConfig conf;
                    conf.projectileKey = behaviorData.value("shootProjectile", "magic_ball");
                    conf.sound = behaviorData.value("shootSound", "fireball");
                    conf.damage = behaviorData.value("shootDamage", 1);
                    conf.shootInterval = behaviorData.value("shootInterval", 2.0f);
                    conf.speed = behaviorData.value("shootSpeed", 20.0f);
                    // TODO particle configuration

                    if (game.spriteMap.find(targetName) != game.spriteMap.end())
                    {
                        behavior = std::make_unique<ShootBehavior>(
                            game, owner, game.spriteMap[targetName], conf);
                    }
                }
                else
                {
                    TraceLog(LOG_WARNING, "Unknown behavior type: %s", behaviorKeyStr.c_str());
                }

                if (behavior)
                {
                    // Create unique key: "stateName_behaviorType"
                    std::string uniqueKey = stateName + "_" + behaviorKeyStr;
                    stateMachine->addBehavior(uniqueKey, std::move(behavior));
                    state->activeBehaviorKeys.push_back(uniqueKey);
                }
            }
        }

        stateMachine->addState(std::move(state));
    }

    // Add transitions (after all states exist)
    for (const auto& stateData : data["states"])
    {
        std::string stateName = stateData["name"];
        State* state = stateMachine->states[stateName].get();

        if (!stateData.contains("transitions"))
            continue;

        for (const auto& transData : stateData["transitions"])
        {
            StateTransition transition;
            transition.targetState = transData["to"];
            transition.requiresAll = transData.value("requiresAll", true);

            if (transData.contains("conditions"))
            {
                for (const auto& condData : transData["conditions"])
                {
                    std::string type = condData["type"];

                    if (type == "playerInRange")
                    {
                        float distance = condData["distance"];
                        transition.conditions.push_back(
                            TransitionConditions::PlayerInRange(distance));
                    }
                    else if (type == "playerOutOfRange")
                    {
                        float distance = condData["distance"];
                        transition.conditions.push_back(
                            TransitionConditions::PlayerOutOfRange(distance));
                    }
                    else if (type == "healthBelow")
                    {
                        uint32_t threshold = condData["threshold"];
                        transition.conditions.push_back(
                            TransitionConditions::HealthBelow(threshold));
                    }
                    else if (type == "healthBelowPercent")
                    {
                        float percent = condData["percent"];
                        transition.conditions.push_back(
                            TransitionConditions::HealthBelowPercent(percent));
                    }
                    else if (type == "hasLineOfSight")
                    {
                        transition.conditions.push_back(
                            TransitionConditions::HasLineOfSightToPlayer());
                    }
                    else if (type == "lostLineOfSight")
                    {
                        transition.conditions.push_back(
                            TransitionConditions::LostLineOfSightToPlayer());
                    }
                    else
                    {
                        TraceLog(LOG_WARNING, "Unknown condition type: %s", type.c_str());
                    }
                }
            }

            state->transitions.push_back(std::move(transition));
        }
    }

    if (data.contains("initialState"))
        stateMachine->setInitialState(data["initialState"]);

    return stateMachine;
}

// Implementation of transition condition helpers
namespace TransitionConditions {
    TransitionCondition PlayerInRange(float distance) {
        return TransitionCondition{
            [distance](Sprite& sprite) {
                auto& game = sprite.game;
                Sprite* player = game.getPlayer();
                if (!player)
                    return false;

                Vector2 selfCenter = GetRectCenter(sprite.rect);
                Vector2 playerCenter = GetRectCenter(player->rect);
                float dx = playerCenter.x - selfCenter.x;
                float dy = playerCenter.y - selfCenter.y;
                float distSq = dx * dx + dy * dy;

                return distSq < (distance * distance);
            },
            "PlayerInRange(" + std::to_string(distance) + ")"
        };
    }

    TransitionCondition PlayerOutOfRange(float distance) {
        return TransitionCondition{
            [distance](Sprite& sprite) {
                auto& game = sprite.game;
                Sprite* player = game.getPlayer();
                if (!player)
                    return true;

                Vector2 selfCenter = GetRectCenter(sprite.rect);
                Vector2 playerCenter = GetRectCenter(player->rect);
                float dx = playerCenter.x - selfCenter.x;
                float dy = playerCenter.y - selfCenter.y;
                float distSq = dx * dx + dy * dy;

                return distSq >= (distance * distance);
            },
            "PlayerOutOfRange(" + std::to_string(distance) + ")"
        };
    }

    TransitionCondition HealthBelow(uint32_t threshold) {
        return TransitionCondition{
            [threshold](Sprite& sprite) {
                return sprite.health < threshold;
            },
            "HealthBelow(" + std::to_string(threshold) + ")"
        };
    }

    TransitionCondition HealthAbove(uint32_t threshold) {
        return TransitionCondition{
            [threshold](Sprite& sprite) {
                return sprite.health >= threshold;
            },
            "HealthAbove(" + std::to_string(threshold) + ")"
        };
    }

    TransitionCondition HealthBelowPercent(float percent) {
        return TransitionCondition{
            [percent](Sprite& sprite) {
                if (sprite.maxHealth == 0)
                    return false;

                float healthPercent = static_cast<float>(sprite.health) /
                                     static_cast<float>(sprite.maxHealth);
                return healthPercent < percent;
            },
            "HealthBelowPercent(" + std::to_string(percent) + ")"
        };
    }

    TransitionCondition HasLineOfSightToPlayer() {
        return TransitionCondition{
            [](Sprite& sprite) {
                auto& game = sprite.game;
                Sprite* player = game.getPlayer();
                if (!player)
                    return false;

                Vector2 targetPos = player->position;
                return isPathClear(sprite.rect, targetPos, game.walls);
            },
            "HasLineOfSightToPlayer()"
        };
    }

    TransitionCondition LostLineOfSightToPlayer() {
        return TransitionCondition{
            [](Sprite& sprite) {
                auto& game = sprite.game;
                Sprite* player = game.getPlayer();
                if (!player)
                    return true;

                Vector2 targetPos = player->position;
                return !isPathClear(sprite.rect, targetPos, game.walls);
            },
            "LostLineOfSightToPlayer()"
        };
    }

    TransitionCondition TimeInStateExceeds(float seconds) {
        // This requires storing time in state - would need StateMachine refactor
        return TransitionCondition{
            [seconds](Sprite& sprite) {
                // TODO: Implement time tracking in StateMachine
                return false;
            },
            "TimeInStateExceeds(" + std::to_string(seconds) + ")"
        };
    }

    TransitionCondition Custom(std::function<bool(Sprite&)> func, const std::string& desc) {
        return TransitionCondition{
            func,
            desc
        };
    }
}

// Debug and helper methods
std::vector<std::string> StateMachine::getActiveBehaviorKeys() const {
    return std::vector<std::string>(activeBehaviorKeys.begin(), activeBehaviorKeys.end());
}

std::vector<std::string> StateMachine::getAllStateNames() const {
    std::vector<std::string> names;
    for (const auto& [name, state] : states)
        names.push_back(name);
    return names;
}

void StateMachine::drawDebugInfo(int x, int y, int fontSize) const {
    if (!currentState) {
        DrawText("No active state", x, y, fontSize, RED);
        return;
    }

    auto sprite = owner.lock();
    if (!sprite)
        return;

    int lineHeight = fontSize + 2;
    int currentY = y;

    // Draw sprite name
    std::string spriteName = "Sprite: " + sprite->spriteName;
    DrawText(spriteName.c_str(), x, currentY, fontSize, WHITE);
    currentY += lineHeight;

    // Draw current state
    std::string stateName = "State: " + currentState->name;
    DrawText(stateName.c_str(), x, currentY, fontSize, YELLOW);
    currentY += lineHeight;

    // Draw active behaviors
    if (!activeBehaviorKeys.empty()) {
        DrawText("Active Behaviors:", x, currentY, fontSize, LIGHTGRAY);
        currentY += lineHeight;

        for (const auto& key : activeBehaviorKeys) {
            std::string behaviorText = "  - " + key;
            DrawText(behaviorText.c_str(), x, currentY, fontSize, GREEN);
            currentY += lineHeight;
        }
    }

    // Draw all behaviors (for debugging)
    if (sprite->game.debug) {
        DrawText("All Behaviors:", x, currentY, fontSize - 2, DARKGRAY);
        currentY += lineHeight - 2;

        for (const auto& [key, behavior] : behaviors) {
            bool isActive = activeBehaviorKeys.find(key) != activeBehaviorKeys.end();
            Color color = isActive ? GREEN : DARKGRAY;
            std::string text = "  " + key + (isActive ? " (active)" : " (inactive)");
            DrawText(text.c_str(), x, currentY, fontSize - 2, color);
            currentY += lineHeight - 2;
        }
        currentY += 2;
    }

    // Draw possible transitions
    if (!currentState->transitions.empty()) {
        DrawText("Transitions:", x, currentY, fontSize, LIGHTGRAY);
        currentY += lineHeight;

        for (const auto& transition : currentState->transitions) {
            Color color = transition.canTransition(*sprite) ? GREEN : DARKGRAY;
            std::string transText = "  -> " + transition.targetState;
            DrawText(transText.c_str(), x, currentY, fontSize, color);
            currentY += lineHeight;

            // Draw conditions if in debug mode
            if (sprite->game.debug) {
                for (const auto& condition : transition.conditions) {
                    bool met = condition.check(*sprite);
                    Color condColor = met ? GREEN : RED;
                    std::string condText = "     " + condition.description;
                    DrawText(condText.c_str(), x, currentY, fontSize - 2, condColor);
                    currentY += lineHeight - 2;
                }
            }
        }
    }
}