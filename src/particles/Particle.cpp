#include "Particle.h"
#include <cmath>
#include <string>

Particle::Particle()
{
    startAlpha = alpha;
}

void Particle::update(float deltaTime) {
    if (!active)
        return;

    age += deltaTime;
    if (age >= lifetime) {
        active = false;
        return;
    }

    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;

    alpha = startAlpha + (endAlpha - startAlpha) * (age / lifetime);

    size = startSize + (endSize - startSize) * (age / lifetime);

    if (!animationFrames.empty()) {
        animationTimer += deltaTime;
        if (animationTimer >= animationSpeed) {
            animationTimer = 0.0f;
            currentFrame = (currentFrame + 1) % animationFrames.size();
        }
    }
}

void Particle::draw() {
    if (!active || animationFrames.empty())
        return;

    Texture2D* tex = animationFrames[currentFrame];
    if (!tex)
        return;

    Color finalColor = tint;
    finalColor.a = static_cast<unsigned char>(alpha * 255.0f);

    Vector2 origin = { tex->width / 2.0f, tex->height / 2.0f };

    Rectangle source = { 0, 0, static_cast<float>(tex->width), static_cast<float>(tex->height) };
    Rectangle dest = { position.x, position.y, static_cast<float>(tex->width) * size, static_cast<float>(tex->height) * size };

    DrawTexturePro(*tex, source, dest, origin, 0.0f, finalColor);
}

void Particle::reset() {
    age = 0.0f;
    currentFrame = 0;
    animationTimer = 0.0f;
    startAlpha = alpha;
    active = true;
}

void Particle::fromJSON(const nlohmann::json& data, const nlohmann::json& defaultData)
{
    velocity.x = data.value("velocityX", defaultData.value("velocityX", 0.0f));
    velocity.y = data.value("velocityY", defaultData.value("velocityY", 0.0f));
    startAlpha = data.value("startAlpha", defaultData.value("startAlpha", 1.0f));
    endAlpha = data.value("endAlpha", defaultData.value("endAlpha", 0.0f));
    lifetime = data.value("lifetime", defaultData.value("lifetime", 1.0f));
    startSize = data.value("startSize", defaultData.value("startSize", 1.0f));
    endSize = data.value("endSize", defaultData.value("endSize", 1.0f));
    animationSpeed = data.value("animationSpeed", defaultData.value("animationSpeed", 0.1f));

    if (data.contains("tint")) {
        auto tintArray = data.at("tint");
        tint = Color{
            static_cast<unsigned char>(tintArray[0].get<int>()),
            static_cast<unsigned char>(tintArray[1].get<int>()),
            static_cast<unsigned char>(tintArray[2].get<int>()),
            static_cast<unsigned char>(tintArray[3].get<int>())
        };
    }
    else if (defaultData.contains("tint")) {
        auto tintArray = defaultData.at("tint");
        tint = Color{
            static_cast<unsigned char>(tintArray[0].get<int>()),
            static_cast<unsigned char>(tintArray[1].get<int>()),
            static_cast<unsigned char>(tintArray[2].get<int>()),
            static_cast<unsigned char>(tintArray[3].get<int>())
        };
    }
}

void Particle::setAnimationFrames(const std::vector<Texture2D>& textures) {
    animationFrames.clear();
    for (const auto& tex : textures)
        animationFrames.push_back(const_cast<Texture2D*>(&tex));
}