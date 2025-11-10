#pragma once
#include "raylib.h"
#include "raymath.h"
#include <functional>

class Game;
class Sprite;

class CameraController {
public:
    CameraController(Game& game);

    // Setup and configuration
    void initialize(float screenWidth, float screenHeight);
    void setWorldBounds(float worldWidth, float worldHeight);
    void setTarget(const Sprite* target) { followTarget = target; }

    // Camera control
    void update(float deltaTime);
    void setPosition(Vector2 position);
    void setPosition(float x, float y) { setPosition({ x, y }); }

    // Effects
    void startShake(float duration, float xMagnitude, float yMagnitude);

    // Camera state
    const Camera2D& getCamera() const { return camera; }
    Camera2D& getCamera() { return camera; }
    Vector2 getTarget() const { return camera.target; }
    bool isShaking() const { return shakeActive; }

    // Manual target for cutscenes (controlled via CutsceneManager)
    void setManualTarget(Vector2 target) { manualTarget = target; }
    void setManualTarget(float x, float y) { manualTarget = { x, y }; }

private:
    Game& game;
    Camera2D camera = {};

    // Camera shake
    bool shakeActive = false;
    float shakeDuration = 0.0f;
    float shakeTimeRemaining = 0.0f;
    float shakeXMagnitude = 0.0f;
    float shakeYMagnitude = 0.0f;

    // Follow settings
    const Sprite* followTarget = nullptr;
    Vector2 manualTarget = { 0.0f, 0.0f };

    // World bounds
    float worldWidth = 0.0f;
    float worldHeight = 0.0f;
    float screenWidth = 0.0f;
    float screenHeight = 0.0f;
    float hudHeight = 0.0f;

    // Private methods
    Vector2 calculateTargetPosition() const;
    Vector2 clampToBounds(Vector2 target) const;
    Vector2 applyShake(Vector2 target);
    void updateShake(float deltaTime);
    void setupEventListeners();
};