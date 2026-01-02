#pragma once
#include "json.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

class Sprite;
class Behavior;
class Game;

// Represents a condition that can trigger a state transition
struct TransitionCondition
{
    std::function<bool(Sprite&)> check;
    std::string description; // For debugging
};

// Represents a transition from one state to another
struct StateTransition
{
    std::string targetState;
    std::vector<TransitionCondition> conditions;
    float weight = 1.0f;
    bool requiresAll = true; // If true, ALL conditions must be met; if false, ANY condition

    bool canTransition(Sprite& sprite) const
    {
        if (conditions.empty())
            return false;

        if (requiresAll)
        {
            for (const auto& condition : conditions)
            {
                if (!condition.check(sprite))
                    return false;
            }
            return true;
        }
        else
        {
            for (const auto& condition : conditions)
            {
                if (condition.check(sprite))
                    return true;
            }
            return false;
        }
    }
};

// Represents a single state in the state machine
struct State
{
    std::string name;
    std::vector<std::string> activeBehaviorKeys; // Behaviors that should be active in this state
    std::vector<StateTransition> transitions;
    float timer = 0.0f; // keeps the time spent in this state

    // Optional callbacks
    std::function<void(Sprite&)> onEnter;
    std::function<void(Sprite&)> onExit;
    std::function<void(Sprite&, float)> onUpdate;

    State(const std::string& name) : name(name) {}
};

// The state machine that manages states and transitions
class StateMachine
{
public:
    StateMachine(Game& game, std::shared_ptr<Sprite> owner);

    // Add a state to the machine
    void addState(std::unique_ptr<State> state);

    // Add a behavior instance that can be used by any state
    void addBehavior(const std::string& key, std::unique_ptr<Behavior> behavior);

    // Set the initial state
    void setInitialState(const std::string& stateName);

    // Update the state machine (checks transitions and updates active behaviors)
    void update(float deltaTime);

    // Behaviors can have a draw() method, this calls them
    void draw();

    // Force a transition to a specific state
    void transitionTo(const std::string& stateName);

    // Get current state name
    const std::string& getCurrentStateName() const
    {
        return currentState ? currentState->name : emptyString;
    }

    // get the time spent in current state
    float getCurrentStateTimer() const
    {
        return currentState ? currentState->timer : 0.0f;
    }

    // Check if in a specific state
    bool isInState(const std::string& stateName) const
    {
        return currentState && currentState->name == stateName;
    }

    // Get list of active behavior keys
    std::vector<std::string> getActiveBehaviorKeys() const;

    // Create a state machine from JSON data
    static std::unique_ptr<StateMachine> createFromJSON(
        Game& game,
        std::shared_ptr<Sprite> owner,
        const nlohmann::json& data
    );

    // Debug visualization (text)
    void drawDebugInfo(int x, int y, int fontSize = 8) const;

    // Get list of all states
    std::vector<std::string> getAllStateNames() const;

    using StateChangeCallback = std::function<void(const std::string& from, const std::string& to)>;
    void setStateChangeCallback(StateChangeCallback callback)
    {
        onStateChange = callback;
    }

private:
    Game& game;
    std::weak_ptr<Sprite> owner;
    State* currentState = nullptr;
    std::unordered_map<std::string, std::unique_ptr<State>> states;
    std::unordered_map<std::string, std::unique_ptr<Behavior>> behaviors; // All behavior instances
    std::unordered_set<std::string> activeBehaviorKeys; // Which behaviors are currently active
    StateChangeCallback onStateChange;
    static const std::string emptyString;

    void enterState(State* state);
    void exitState(State* state);
    void activateBehaviors(const std::vector<std::string>& keys);
};

// Helper functions to create common transition conditions
namespace TransitionConditions
{
    // Distance-based conditions
    TransitionCondition PlayerInRange(float distance);
    TransitionCondition PlayerOutOfRange(float distance);

    // Health-based conditions
    TransitionCondition HealthBelow(uint32_t threshold);
    TransitionCondition HealthAbove(uint32_t threshold);
    TransitionCondition HealthBelowPercent(float percent);
    TransitionCondition HealthAbovePercent(float percent);

    // Line of sight
    TransitionCondition HasLineOfSightToPlayer();
    TransitionCondition LostLineOfSightToPlayer();

    // Timer-based (TODO)
    TransitionCondition TimeInStateExceeds(float seconds);

    // Custom condition
    TransitionCondition Custom(std::function<bool(Sprite&)> func, const std::string& desc);
}