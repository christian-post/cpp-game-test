#include "InGame.h"
#include "Game.h"
#include "Commands.h"
#include "raylib.h"
#include "raymath.h"
#include "Utils.h"
#include "Behavior.h"


void InGame::startup() {
    // create the player sprite
    // the "spriteName" argument has to match the texture keys (the part before the "_")
    player = std::make_shared<Sprite>(
        game, 0.0f, 0.0f, 16.0f, 16.0f, "player"
    );

    spriteMap["player"] = player;
    player->persistent = true;
    game.sprites.push_back(player);  // add to the sprites vector
    player->setTextures({ "player_idle", "player_run", "player_hit" });
    player->health = 10;

    // retrieve the tilemap
    // some sprites need the player reference, so this has to come after the player
    loadTilemap("test_dungeon2");
    // set the player's position in the first room
    player->moveTo(float(10 * tileSize), float(15 * tileSize));

    // setup the camera
    camera.target = Vector2{ player->rect.x, player->rect.y };
    camera.offset = Vector2{ game.gameScreenWidth / 2.0f, game.gameScreenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    // test enemies
    // TODO: get them from Tiled data
    auto addBehaviors = [&](auto& e) {
        e->addBehavior(std::make_unique<WatchBehavior>(*e, *player));
        e->addBehavior(std::make_unique<RandomWalkBehavior>(*e));
        e->addBehavior(std::make_unique<ChaseBehavior>(*e, *player, 48.0f, 2.0f, 64.0f));
        };

    //auto e1 = spawnEnemy("skelet", 8, 5); addBehaviors(e1);
    //auto e2 = spawnEnemy("skelet", 10, 4); addBehaviors(e2);
    //auto e3 = spawnEnemy("skelet", 12, 6); addBehaviors(e3);

    // queue a cutscene
    //game.eventManager.pushDelayedEvent("cutsceneStart", 0.1f, nullptr, [game, this]() {
    //    cutsceneFlag = true;
    //    Sprite& npcRef = *spriteMap["npc"];
    //    float npcX = npcRef.position.x;
    //    float npcY = npcRef.position.y + 3.0f * (float)tileMap->tileHeight;
    //    cutsceneManager.queueCommand(new Command_Wait(1.0f));
    //    cutsceneManager.queueCommand(new Command_MoveTo(npcRef, npcX, npcY, 2.0f));
    //    cutsceneManager.queueCommand(new Command_Wait(0.5f));
    //    cutsceneManager.queueCommand(new Command_Textbox(*game, "Please help me, Sir Knight! These skeletons there are up to no good. Do something about them!!!"));
    //});
}

Sprite* InGame::getSprite(const std::string& name) {
    auto it = spriteMap.find(name);
    if (it != spriteMap.end() && it->second) {
        return it->second.get();
    }
    return nullptr;
}

std::shared_ptr<Sprite> InGame::spawnEnemy(const std::string& name, int tileX, int tileY) {
    // helper function that creates an enemy on the map
    // TODO: gets replaced with function that takes the data from TileMap
    auto enemy = std::make_shared<Sprite>(
        game, float(tileX * tileSize), float(tileY * tileSize), 12.0f, 12.0f, name
    );
    enemy->speed = 12.0f;
    enemy->damage = 1;
    enemy->health = 4;
    enemy->canHurtPlayer = true;
    enemy->setTextures({ name + "_idle", name + "_run" });
    game.sprites.push_back(enemy);
    return enemy;
}

void InGame::loadTilemap(const std::string& name) {
    tileMap = &game.loader.getTilemap(name);
    tileSize = tileMap->tileWidth;
    // remove static and dynamic (non-persistent) sprites
    game.walls.clear();
    game.clearSprites();
    // build static collision objects from map data
    for (auto& obj : tileMap->getObjects()) {
        if (obj.type == "wall") {
            game.walls.push_back(std::make_unique<Rectangle>(
                Rectangle{ obj.x, obj.y, obj.width, obj.height })
            );
        }
        else if (obj.type == "sprite") {
            auto sprite = std::make_shared<Sprite>(
                game, obj.x, obj.y, obj.width, obj.height, obj.name
            );
            if (obj.name == "teleport") {
                sprite->isColliding = false;
                std::string targetMap = obj.properties.value("targetMap", ""); // TODO: can this be safely done without copying the string?
                float targetX = obj.properties.value("targetPosX", 0.0f);
                float targetY = obj.properties.value("targetPosY", 0.0f);
                sprite->addBehavior(std::make_unique<TeleportBehavior>(
                    game, *sprite, *player, targetMap, 
                    Vector2{ targetX, targetY }
                ));
            }
            else if (obj.name == "npc") {
                sprite->speed = obj.properties.value("speed", 20.0f);
                sprite->setTextures({ "npc_idle", "npc_run" }); // TODO: make this more modular
                // get behaviors from the data
                std::string raw = obj.properties.value("behaviors", "");
                std::istringstream ss(raw);
                std::string token;
                while (std::getline(ss, token, ',')) {
                    if (token == "RandomWalk") {
                        sprite->addBehavior(std::make_unique<RandomWalkBehavior>(*sprite));
                    }
                    else if (token == "Watch") {
                        std::string targetName = obj.properties.value("watchTarget", "");
                        if (spriteMap.find(targetName) != spriteMap.end()) {
                            sprite->addBehavior(std::make_unique<WatchBehavior>(*sprite, *spriteMap[targetName]));
                        }
                        else {
                            Log(targetName + " not found in spriteMap. Skipping WatchBehavior.");
                        }
                    } // TODO: add other behaviors, maybe do a helper function
                }
            }
            else if (obj.name == "enemy") {
                std::string enemyType = obj.properties.value("enemyType", "ERROR_ENEMY_TYPE");
                sprite->spriteName = enemyType; //TODO this is not consistent between npcs and enemies
                sprite->speed = obj.properties.value("speed", 20.0f);
                sprite->damage = obj.properties.value("damage", 1);
                sprite->health = obj.properties.value("health", 10);
                sprite->canHurtPlayer = true;
                sprite->setTextures({ enemyType + "_idle", enemyType + "_run" });
                // TODO: enemy behaviors
            }
            game.sprites.push_back(sprite);
        }
    }
    // calculate the map dimensions (to be used by the camera)
    worldWidth = tileMap->width * tileSize;
    worldHeight = tileMap->height * tileSize;
}


void InGame::events(const std::unordered_map<std::string, std::any>& events) {
    for (const auto& [name, data] : events) {
        if (name == "teleport") {
            const auto& teleportData = std::any_cast<const TeleportEvent&>(data);
            loadTilemap(teleportData.targetMap);
            player->moveTo(teleportData.targetPos.x * tileSize, teleportData.targetPos.y * tileSize);
        }
    }
}

void InGame::update(float dt) {
    // control the sprites and apply physics

    // remove sprites that are dead (from last frame)
    for (const auto& sprite : game.sprites) {
        if (sprite && sprite->health < 1 && !sprite->dying) {
            sprite->dying = true;
            sprite->removeAllBehaviors();
            sprite->addBehavior(std::make_unique<DeathBehavior>(*sprite, 2.0f));
            game.eventManager.pushDelayedEvent("killSprite", 3.0f, nullptr, [this, sprite]() {
                this->game.killSprite(sprite);
            });
        }
    }
    // if a cutscene is active, it takes control over the player
    // otherwise, the player is controled by input
    if (!cutsceneManager.isActive()) {
        player->getControls();
        for (const auto& sprite : game.sprites) {
            sprite->executeBehavior(dt);
            sprite->update(dt);
            sprite->animate(dt);
        }
    } else {
        cutsceneManager.update(dt);
        for (const auto& sprite : game.sprites) {
            sprite->animate(dt);
        }
    }

    // testing a weapon (sword)
    // TODO: needs to be more modular
    if (game.buttonsDown & CONTROL_ACTION2) {
        // spawn the weapon next to the player if not already there
        // TODO write a wrapper for this, or get weapon data from JSON file
        if (!getSprite("weapon_sword")) {
            auto sword = std::make_shared<Sprite>(
                game, player->position.x, player->position.y, 16.0f, 16.0f, "weapon_sword"
            );
            spriteMap["weapon_sword"] = sword;
            game.sprites.push_back(sword);
            sword->setTextures({ "weapon_sword" });
            sword->setHurtbox(-1.0f, -1.0f, 16.0f, 36.0f);
            sword->doesAnimate = false; // TODO: some weapons might have animations
            sword->isColliding = false; // weapons don't have collision with walls
            sword->damage = 2;
            sword->addBehavior(std::make_unique<WeaponBehavior>(*sword, *player, 0.4f));
            // add an event listener that removes the sword
            // the "killSword" event is dispatched by WeaponBehavior once it's finished
            game.eventManager.addListener("killSword", [this, sword](std::any) {
                this->game.killSprite(sword);
                spriteMap.erase("weapon_sword");
            });
        }
    }

    // collision of sprites with static objects (walls)
    // TODO: make this a method of Sprite?
    // 
    // resolve collision in the X direction
    for (const auto& sprite : game.sprites) {
        // Resolve X axis
        sprite->rect.x = sprite->position.x;
        for (const auto& wall : game.walls) {
            if (sprite->isColliding && CheckCollisionRecs(sprite->rect, *wall)) {
                float spriteCenterX = sprite->rect.x + sprite->rect.width * 0.5f;
                float wallCenterX = wall->x + wall->width * 0.5f;
                if (spriteCenterX < wallCenterX) {
                    sprite->position.x = wall->x - sprite->rect.width;
                }
                else {
                    sprite->position.x = wall->x + wall->width;
                }
                sprite->vel.x = 0.0f;
                sprite->rect.x = sprite->position.x;
            }
        }

        // Resolve Y axis
        sprite->rect.y = sprite->position.y;
        for (const auto& wall : game.walls) {
            if (sprite->isColliding && CheckCollisionRecs(sprite->rect, *wall)) {
                float spriteCenterY = sprite->rect.y + sprite->rect.height * 0.5f;
                float wallCenterY = wall->y + wall->height * 0.5f;
                if (spriteCenterY < wallCenterY) {
                    sprite->position.y = wall->y - sprite->rect.height;
                }
                else {
                    sprite->position.y = wall->y + wall->height + 0.1f;
                }
                sprite->vel.y = 0.0f;
                sprite->rect.y = sprite->position.y;
            }
        }
        // center the hurtbox
        sprite->hurtbox.x = sprite->rect.x + (sprite->rect.width - sprite->hurtbox.width) / 2 + sprite->hurtboxOffset.x;
        sprite->hurtbox.y = sprite->rect.y + (sprite->rect.height - sprite->hurtbox.height) / 2 + sprite->hurtboxOffset.y;
        // collisions with dynamic objects
        // damage and other interactions
        if (sprite->canHurtPlayer && player->iFrameTimer < 0.001f && CheckCollisionRecs(sprite->hurtbox, player->rect)) {
            player->health -= sprite->damage;
            player->iFrameTimer = game.settings["PlayeriFrames"];
            applyKnockback(*sprite, *player, 10.0f); // TODO put knockback in settings
        }
        // weapon damage
        // TODO: only works for "skelet"
        if (sprite->spriteName == "skelet") {
            Sprite* weapon = getSprite("weapon_sword");
            if (weapon && sprite->iFrameTimer < 0.001f && sprite->health > 0 && CheckCollisionRecs(weapon->hurtbox, sprite->rect)) {
                sprite->health -= weapon->damage;
                sprite->iFrameTimer = 0.5; // TODO: put in weapon settings
                applyKnockback(*weapon, *sprite, 8.0f); // TODO: put in weapon settings
            }
        }
    }
    // Camera follows player
    camera.target = Vector2{ player->rect.x + player->rect.width / 2, player->rect.y + player->rect.height / 2 };
    // Define camera boundaries (and factor in the HUD dimensions)
    float HudHeight = game.settings["HudHeight"];
    float minX = game.gameScreenWidth / 2.0f;
    float minY = game.gameScreenHeight / 2.0f - HudHeight;
    float maxX = worldWidth - game.gameScreenWidth / 2.0f;
    float maxY = worldHeight - game.gameScreenHeight / 2.0f;
    // Clamp camera target within world bounds
    camera.target.x = Clamp(camera.target.x, minX, maxX);
    camera.target.y = Clamp(camera.target.y - HudHeight * 0.5f, minY, maxY);

    // player dies, GameOver scene starts
    if (player->health < 1) {
        game.pauseScene(getName());
        game.stopScene("HUD");
        game.startScene("GameOver");
    }
}

void InGame::drawTiles(const TileMap* tileMap, const std::vector<Texture2D>& tiles, int layerIndex) {
    const auto& layer = tileMap->getLayer(layerIndex);

    for (int y = 0; y < tileMap->height; y++) {
        for (int x = 0; x < tileMap->width; x++) {
            int tileIndex = layer.data[y][x] - 1; // Tiled index starts at 1
            if (tileIndex >= 0) { // Ensure it's a valid tile
                DrawTexture(tiles[tileIndex], x * tileMap->tileWidth, y * tileMap->tileHeight, WHITE);
            }
        }
    }
}

void InGame::draw() {
    ClearBackground(BLACK);

    BeginMode2D(camera);
        // draw the textures that are affected by the camera
        const auto& tiles = game.loader.getTextures("dungeon");
        int totalLayers = static_cast<int>(tileMap->layers.size());

        for (int layerIndex = 0; layerIndex < totalLayers - 1; ++layerIndex) {
            drawTiles(tileMap, tiles, layerIndex);
        }
        for (const auto& sprite : game.sprites) {
            sprite->draw();
        }
        // Draw layer 3 (on top of the player)
        drawTiles(tileMap, tiles, totalLayers - 1);
        // draw the static collision objects in debug mode
        if (game.debug) {
            for (const auto& wall : game.walls) {
                DrawRectangleLines((int)wall->x, (int)wall->y, (int)wall->width, (int)wall->height, BLUE);
            }
            for (const auto& sprite : game.sprites) {
                DrawRectangleLines((int)sprite->rect.x, (int)sprite->rect.y, (int)sprite->rect.width, (int)sprite->rect.height, GREEN);
            }
            for (const auto& sprite : game.sprites) {
                DrawRectangleLines((int)sprite->hurtbox.x, (int)sprite->hurtbox.y, (int)sprite->hurtbox.width, (int)sprite->hurtbox.height, RED);
            }
        }
    EndMode2D();
    // cutscene stuff (textboxes etc) gets drawn relative to window position
    cutsceneManager.draw();
    // overlay debug info texts
    if (game.debug) {
        //std::string debugText = "Enemy: (" + std::to_string((int)spriteMap["npc"]->position.x) + "," + std::to_string((int)spriteMap["npc"]->position.y) + ")";
        //debugText += "NPC: (" + std::to_string((int)spriteMap["npc"]->rect.x) + "," + std::to_string((int)spriteMap["npc"]->rect.y) + ")";
        std::string debugText = "Debug: ";
        debugText += "player vel.: " + std::to_string(Vector2Length(player->vel));
        DrawText(debugText.c_str(), 4, game.gameScreenHeight - 22, 10, LIGHTGRAY);
        //DrawTextEx(game.loader.getFont("slkscr"), "Hello, World", Vector2{ 50, 50 }, 32, 0, WHITE);
    }
}

void InGame::end() {
    // wait for a split second
    WaitTime(0.25);
}