#pragma once
#include "Behavior.h"
#include <memory>

class Sprite;

class WatchBehavior : public Behavior {
public:
    WatchBehavior(std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target);
    void update(float deltaTime) override;

private:
    std::weak_ptr<Sprite> self;
    std::weak_ptr<Sprite> target;
};
