#pragma once
#include "Behavior.h"
#include <memory>
#include <string>
#include <vector>

class Game;
class Sprite;

class DialogueBehavior : public Behavior
{
public:
    DialogueBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> player, std::vector<std::string> dialogTexts, std::string voice);
    void update(float deltaTime) override;
    void reset() override;

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
