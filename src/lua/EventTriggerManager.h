#pragma once
#include <json.hpp>
#include <string>
#include <vector>

class Game;
class InGame;

struct EventTrigger
{
    std::string id;
    std::string scriptPath;
    nlohmann::json conditions;
    bool hasTriggered = false;

    bool checkConditions(Game& game) const;
};

class EventTriggerManager
{
public:
    EventTriggerManager(Game& game);

    void loadTriggers(const std::string& filepath);
    void update(); // called every frame to check conditions

private:
    Game& game;
    std::vector<EventTrigger> triggers;
};