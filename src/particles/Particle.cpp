#include "Particle.h"
#include "raymath.h"
#include <cmath>
#include <string>

static Vector3 getColorAtLifetime(float progress, const std::vector<Vector3>& colors) {
    if (colors.empty())
        return { 1.0f, 1.0f, 1.0f };

    if (colors.size() == 1)
        return colors[0];

    float scaledProgress = progress * (colors.size() - 1);
    int index = static_cast<int>(scaledProgress);
    float t = scaledProgress - index;

    if (index >= colors.size() - 1)
        return colors.back();

    return Vector3Lerp(colors[index], colors[index + 1], t);
}

static float applyEasing(EasingType type, float t) {
    switch (type) {
    case EasingType::None:
        return 1.0f;

    case EasingType::QuadIn:
        return t * t;

    case EasingType::QuadOut:
        return 1.0f - (1.0f - t) * (1.0f - t);

    case EasingType::QuadInOut: {
        if (t < 0.5f) {
            return 2.0f * t * t;
        }
        else {
            return  1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
        }
    }

    case EasingType::CubicIn: {
        return t * t * t;
    }

    case EasingType::CubicOut:
        return 1.0f - std::pow(1.0f - t, 3.0f);

    case EasingType::CubicInOut: {
        if (t < 0.5f) {
            return 4.0f * t * t * t;
        }
        else {
            return 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
        }
    }

    default:
        return 1.0f;
    }
}


Particle::Particle()
{
    startAlpha = alpha;
    initialVelocity = velocity;
}

void Particle::update(float deltaTime) {
    if (!active)
        return;

    age += deltaTime;
    if (age >= lifetime) {
        active = false;
        return;
    }

    velocity.x += gravity.x * deltaTime;
    velocity.y += gravity.y * deltaTime;

    // Apply velocity easing if enabled
    Vector2 finalVelocity = velocity;
    if (velocityEasing != EasingType::None) {
        float t = age / lifetime;
        float easeFactor = applyEasing(velocityEasing, t);
        finalVelocity.x = initialVelocity.x * easeFactor;
        finalVelocity.y = initialVelocity.y * easeFactor;
        //TraceLog(LOG_INFO, "%f", Vector2Length(finalVelocity));
    }

    position.x += finalVelocity.x * deltaTime;
    position.y += finalVelocity.y * deltaTime;

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
    if (!active)
        return;

    //Color finalColor = tint;
    //finalColor.a = static_cast<unsigned char>(alpha * 255.0f);
    float progress = age / lifetime;
    Vector3 colorNorm = getColorAtLifetime(progress, colorGradient);

    Color finalColor = {
        static_cast<unsigned char>(colorNorm.x * 255.0f),
        static_cast<unsigned char>(colorNorm.y * 255.0f),
        static_cast<unsigned char>(colorNorm.z * 255.0f),
        static_cast<unsigned char>(alpha * 255.0f)
    };

    if (type == ParticleType::Circle) {
        float radius = primitiveSize * size;
        DrawCircle(int(position.x), int(position.y), radius, finalColor);
    }
    else if (type == ParticleType::Square) {
        float halfSize = primitiveSize * size;
        Rectangle rect = { position.x - halfSize, position.y - halfSize, halfSize * 2, halfSize * 2 };
        DrawRectangleRec(rect, finalColor);
    }
    else if (type == ParticleType::Texture) {
        if (animationFrames.empty())
            return;

        Texture2D* tex = animationFrames[currentFrame];
        if (!tex)
            return;

        Vector2 origin = { tex->width / 2.0f * size, tex->height / 2.0f * size };
        Rectangle source = { 0, 0, static_cast<float>(tex->width), static_cast<float>(tex->height) };
        Rectangle dest = { position.x, position.y, static_cast<float>(tex->width) * size, static_cast<float>(tex->height) * size };
        DrawTexturePro(*tex, source, dest, origin, 0.0f, finalColor);
    }
}

void Particle::reset() {
    age = 0.0f;
    currentFrame = 0;
    animationTimer = 0.0f;
    startAlpha = alpha;
    initialVelocity = velocity;
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

    // TODO this has a lot of repeating code
    if (data.contains("tint")) {
        auto& tintData = data.at("tint");
        if (tintData[0].is_array()) {
            // Multi-color gradient
            colorGradient.clear();
            for (const auto& colorArray : tintData) {
                Vector3 normalized = {
                    colorArray[0].get<int>() / 255.0f,
                    colorArray[1].get<int>() / 255.0f,
                    colorArray[2].get<int>() / 255.0f
                };
                colorGradient.push_back(normalized);
            }
        }
        else {
            // Single color
            Vector3 normalized = {
                tintData[0].get<int>() / 255.0f,
                tintData[1].get<int>() / 255.0f,
                tintData[2].get<int>() / 255.0f
            };
            colorGradient = { normalized };
        }
    }
    else if (defaultData.contains("tint")) {
        auto& tintData = defaultData.at("tint");
        if (tintData[0].is_array()) {
            colorGradient.clear();
            for (const auto& colorArray : tintData) {
                Vector3 normalized = {
                    colorArray[0].get<int>() / 255.0f,
                    colorArray[1].get<int>() / 255.0f,
                    colorArray[2].get<int>() / 255.0f
                };
                colorGradient.push_back(normalized);
            }
        }
        else {
            Vector3 normalized = {
                tintData[0].get<int>() / 255.0f,
                tintData[1].get<int>() / 255.0f,
                tintData[2].get<int>() / 255.0f
            };
            colorGradient = { normalized };
        }
    }

    if (data.contains("primitiveType")) {
        std::string typeStr = data.at("primitiveType");
        if (typeStr == "circle")
            type = ParticleType::Circle;
        else if (typeStr == "square")
            type = ParticleType::Square;
        else
            TraceLog(LOG_WARNING, "Unsupported primitive type: %s", typeStr.c_str());
    }
    primitiveSize = data.value("primitiveSize", defaultData.value("primitiveSize", 5.0f));

    if (data.contains("velocityEasing")) {
        std::string easingStr = data.at("velocityEasing");
        if (easingStr == "quadIn")
            velocityEasing = EasingType::QuadIn;
        else if (easingStr == "quadOut")
            velocityEasing = EasingType::QuadOut;
        else if (easingStr == "quadInOut")
            velocityEasing = EasingType::QuadInOut;
        else if (easingStr == "cubicIn")
            velocityEasing = EasingType::CubicIn;
        else if (easingStr == "cubicOut")
            velocityEasing = EasingType::CubicOut;
        else if (easingStr == "cubicInOut")
            velocityEasing = EasingType::CubicInOut;
        else
            velocityEasing = EasingType::None;
    }
}

void Particle::setAnimationFrames(const std::vector<Texture2D>& textures) {
    animationFrames.clear();
    for (const auto& tex : textures)
        animationFrames.push_back(const_cast<Texture2D*>(&tex));
}