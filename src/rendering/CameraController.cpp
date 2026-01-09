#include "CameraController.h"
#include "Game.h"
#include "Sprite.h"
#include "Utils.h"
#include <algorithm>

CameraController::CameraController(Game& game) : game(game) 
{
    camera.offset = { 0.0f, 0.0f };
    camera.target = { 0.0f, 0.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    setupEventListeners();
}

void CameraController::initialize(float screenW, float screenH)
{
    screenWidth = screenW;
    screenHeight = screenH;
    hudHeight = game.getSetting<float>("HudHeight");

    // Set camera offset to center of screen
    camera.offset = { screenWidth / 2.0f, screenHeight / 2.0f };
}

void CameraController::setWorldBounds(float worldW, float worldH)
{
    worldWidth = worldW;
    worldHeight = worldH;
}

void CameraController::setPosition(Vector2 position)
{
    manualTarget = position;
}

void CameraController::startShake(float duration, float xMag, float yMag)
{
    shakeActive = true;
    shakeDuration = duration;
    shakeTimeRemaining = duration;
    shakeXMagnitude = xMag;
    shakeYMagnitude = yMag;
}

void CameraController::update(float deltaTime)
{
    // Update camera shake
    updateShake(deltaTime);

    // Calculate the target position
    Vector2 target = calculateTargetPosition();

    // Clamp to world bounds
    target = clampToBounds(target);

    // Apply camera shake if active
    if (shakeActive) {
        target = applyShake(target);
    }

    // Update camera target
    camera.target = target;
}

Vector2 CameraController::calculateTargetPosition() const
{
    // Manual control takes priority (for cutscenes)
    if (game.cutsceneManager.hasCameraControl()) {
        return manualTarget;
    }

    // Follow the target sprite
    if (followTarget) {
        return {
            followTarget->rect.x + followTarget->rect.width / 2.0f,
            followTarget->rect.y + followTarget->rect.height / 2.0f
        };
    }

    // Default to current target
    return camera.target;
}

Vector2 CameraController::clampToBounds(Vector2 target) const
{
    if (worldWidth <= 0.0f || worldHeight <= 0.0f) {
        return target;
    }

    float minX = screenWidth / 2.0f;
    float minY = screenHeight / 2.0f - hudHeight;
    float maxX = worldWidth - screenWidth / 2.0f;
    float maxY = worldHeight - screenHeight / 2.0f;

    // Adjust target Y to account for HUD
    float adjustedY = target.y - hudHeight * 0.5f;

    return {
        Clamp(target.x, minX, maxX),
        Clamp(adjustedY, minY, maxY)
    };
}

void CameraController::updateShake(float deltaTime)
{
    if (!shakeActive) {
        return;
    }

    shakeTimeRemaining -= deltaTime;

    if (shakeTimeRemaining <= 0.0f) {
        shakeActive = false;
        shakeTimeRemaining = 0.0f;
    }
}

Vector2 CameraController::applyShake(Vector2 target)
{
    if (!shakeActive || shakeDuration <= 0.0f) {
        return target;
    }

    // Calculate intensity based on remaining time TODO unused??
    //float intensity = shakeTimeRemaining / shakeDuration;

    // Use sine wave for smoother shake
    float t = 1.0f - (shakeTimeRemaining / shakeDuration);
    float strength = sinf(t * 3.14159f);

    return {
        target.x + strength * shakeXMagnitude,
        target.y - strength * shakeYMagnitude
    };
}

void CameraController::setupEventListeners()
{
    // Listen for manual camera movement events
    game.eventManager.addListener(MOVE_CAMERA, [this](std::any data) {
        if (!data.has_value()) {
            return;
        }

        auto [x, y] = std::any_cast<std::pair<float, float>>(data);
        setManualTarget(x, y);
        });

    // Listen for screen shake events
    // data expects a tuple of duration, x magnitude, y magnitude
    game.eventManager.addListener(SCREEN_SHAKE, [this](std::any data) {
        if (!data.has_value() || !game.getSetting<bool>("screenShake")) {
            // TODO: default shake intensity?
            return;
        }

        if (data.type() == typeid(std::tuple<float, float, float>)) {
            auto [duration, xMag, yMag] = std::any_cast<std::tuple<float, float, float>>(data);
            startShake(duration, xMag, yMag);
        }
        });
}