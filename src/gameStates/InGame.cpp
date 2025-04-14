#include "InGame.h"
#include "Game.h"
#include "Commands.h"
#include "raylib.h"
#include "raymath.h"
#include "Utils.h"


void InGame::startup(Game* game) {
    // retrieve the tilemap
    tileMap = &game->loader.getTilemap("test_dungeon2");
    int tileSize = tileMap->tileWidth; // just assume tiles are square

    // create the player sprite
    // the "spriteName" argument has to match the texture keys (the part before the "_")
    player = std::make_shared <Sprite>(
        *game, float(10 * tileSize), float(15 * tileSize), 16.0f, 16.0f, "player"
    );
    spriteMap["player"] = player;
    game->sprites.push_back(player);  // add to the sprites vector
    player->setTextures({ "player_idle", "player_run", "player_hit" });

    // reduced health for testing
    player->health = 10;

    // build static collision objects from map data
    for (auto& obj : tileMap->getObjects()) {
        if (obj.type == "wall") {
            game->walls.push_back(std::make_unique<Rectangle>(
                Rectangle{ obj.x, obj.y, obj.width, obj.height })
            );
        }
    }

    // calculate the world dimensions
    worldWidth = tileMap->width * tileSize;
    worldHeight = tileMap->height * tileSize;

    // setup the camera for this scene
    camera.target = Vector2{ player->rect.x, player->rect.y };
    camera.offset = Vector2{ game->gameScreenWidth / 2.0f, game->gameScreenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    // create another NPC for testing
    auto npc = std::make_shared<Sprite>(
        *game, float(10 * tileSize), float(10 * tileSize), 12.0f, 12.0f, "npc"
    );
    game->sprites.push_back(npc);
    spriteMap["npc"] = npc;
    npc->speed = 10.0f; // half as slow as the player
    npc->setTextures({ "npc_idle", "npc_run" });
    // the npc should randomly walk about, but look towards the player if idle
    npc->addBehavior(std::make_unique<RandomWalkBehavior>(*npc));
    npc->addBehavior(std::make_unique<WatchBehavior>(*npc, *player));

    // test enemy
    auto enemy = std::make_shared<Sprite>(
        *game, float(9 * tileSize), float(5 * tileSize), 12.0f, 12.0f, "skelet"
    );
    game->sprites.push_back(enemy);
    enemy->speed = 15.0f;
    enemy->damage = 1;
    enemy->health = 4;
    enemy->canHurtPlayer = true;
    enemy->setTextures({ "skelet_idle", "skelet_run" });
    enemy->addBehavior(std::make_unique<RandomWalkBehavior>(*enemy));
    enemy->addBehavior(std::make_unique<ChaseBehavior>(*enemy, *player, 48.0f, 2.0f, 64.0f));

    // queue a cutscene
    /*game->eventManager.pushDelayedEvent("cutsceneStart", 0.1f, nullptr, [game, this]() {
        cutsceneFlag = true;
        Sprite& npcRef = *spriteMap["npc"];
        float npcX = npcRef.position.x;
        float npcY = npcRef.position.y + 3.0f * (float)tileMap->tileHeight;

        cutsceneManager.queueCommand(new Command_Wait(1.0f));
        cutsceneManager.queueCommand(new Command_MoveTo(npcRef, npcX, npcY, 2.0f));
        cutsceneManager.queueCommand(new Command_Wait(0.5f));
        cutsceneManager.queueCommand(new Command_Textbox(*game, "Please help me, Sir Knight! This skeleton there is up to no good. Do something about it!"));
    });*/
}

Sprite* InGame::getSprite(const std::string& name) {
    auto it = spriteMap.find(name);
    if (it != spriteMap.end() && it->second) {
        return it->second.get();
    }
    return nullptr;
}

void InGame::events(Game* game) {
    // TODO
}

void InGame::update(Game* game, float dt) {
    // control the sprites and apply physics

    // remove sprites that are dead
    for (const auto& sprite : game->sprites) {
        if (sprite && sprite->health < 1) {
            game->killSprite(sprite);
        }
    }
    
    if (!cutsceneManager.isActive()) {
        player->getControls();
        for (const auto& sprite : game->sprites) {
            sprite->executeBehavior(dt);
            sprite->update(dt);
            sprite->animate(dt);
        }
    } else {
        cutsceneManager.update(dt);
        for (const auto& sprite : game->sprites) {
            sprite->animate(dt);
        }
    }

    // test a weapon
    if (game->buttonsDown & CONTROL_ACTION2) {
        // spawn the weapon
        // TODO write a wrapper for this, or get from text file
        if (!getSprite("weapon_sword")) {
            auto sword = std::make_shared<Sprite>(
                *game, player->position.x, player->position.y, 16.0f, 16.0f, "weapon_sword"
            );
            spriteMap["weapon_sword"] = sword;
            game->sprites.push_back(sword);
            sword->setTextures({ "weapon_sword" });
            sword->setHurtbox(-1.0f, -1.0f, 16.0f, 36.0f); // slightly taller
            sword->doesAnimate = false;
            sword->damage = 2;
            sword->addBehavior(std::make_unique<WeaponBehavior>(*sword, *player, 0.4f));
            game->eventManager.addListener("killSword", [this, sword, game](std::any) {
                game->killSprite(sword);
                spriteMap.erase("weapon_sword");
            });
        }
    }

    // collision of sprites with static objects
    // TODO: make this a method of Sprite
    // 
    // resolve collision in the X direction
    for (const auto& sprite : game->sprites) {
        // Resolve X axis
        sprite->rect.x = sprite->position.x;
        for (const auto& wall : game->walls) {
            if (CheckCollisionRecs(sprite->rect, *wall)) {
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
        for (const auto& wall : game->walls) {
            if (CheckCollisionRecs(sprite->rect, *wall)) {
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

        // collision with dynamic objects
        // damage and other interactions
        if (sprite->canHurtPlayer && player->iFrameTimer <= 0.0f && CheckCollisionRecs(sprite->hurtbox, player->rect)) {
            player->health -= sprite->damage;
            player->iFrameTimer = 1.0f; // TODO: get amount from game settings
            applyKnockback(*sprite, *player, 10.0f);
        }

        // weapon damage
        // TODO: only works for "skelet"
        if (sprite->spriteName == "skelet") {
            Sprite* weapon = getSprite("weapon_sword");
            if (weapon && CheckCollisionRecs(weapon->rect, sprite->rect)) {
                sprite->health -= weapon->damage;
                sprite->iFrameTimer = 0.5;
                applyKnockback(*weapon, *sprite, 8.0f);
            }
        }
    }

    // Camera follows player
    camera.target = Vector2{ player->rect.x + player->rect.width / 2, player->rect.y + player->rect.height / 2 };

    // Define camera boundaries (factor in the HUD dimensions)
    float HudHeight = 32.0f;  // TODO make this a setting

    float minX = game->gameScreenWidth / 2.0f;
    float minY = game->gameScreenHeight / 2.0f - HudHeight;
    float maxX = worldWidth - game->gameScreenWidth / 2.0f;
    float maxY = worldHeight - game->gameScreenHeight / 2.0f;

    // Clamp camera target within world bounds
    camera.target.x = Clamp(camera.target.x, minX, maxX);
    camera.target.y = Clamp(camera.target.y - HudHeight * 0.5f, minY, maxY);

    // player dies
    // TODO add a "die" behavior or something that's different for each sprite
    // and only actually remove the sprite pointers at the end of the game loop
    if (player->health < 1) {
        game->pauseScene(getName());
        game->stopScene("HUD");
        game->startScene("GameOver");
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

void InGame::draw(Game* game) {
    ClearBackground(BLACK);

    BeginMode2D(camera);
        // draw the textures that are affected by the camera

        const auto& tiles = game->loader.getTextures("dungeon");
        int totalLayers = static_cast<int>(tileMap->layers.size());

        for (int layerIndex = 0; layerIndex < totalLayers - 1; ++layerIndex) {
            drawTiles(tileMap, tiles, layerIndex);
        }

        for (const auto& sprite : game->sprites) {
            sprite->draw();
        }

        // Draw layer 3 (on top of the player)
        drawTiles(tileMap, tiles, totalLayers - 1);

        // draw the static collision objects in debug mode
        if (game->debug) {
            for (const auto& wall : game->walls) {
                DrawRectangleLines((int)wall->x, (int)wall->y, (int)wall->width, (int)wall->height, BLUE);
            }

            for (const auto& sprite : game->sprites) {
                DrawRectangleLines((int)sprite->rect.x, (int)sprite->rect.y, (int)sprite->rect.width, (int)sprite->rect.height, GREEN);
            }

            for (const auto& sprite : game->sprites) {
                DrawRectangleLines((int)sprite->hurtbox.x, (int)sprite->hurtbox.y, (int)sprite->hurtbox.width, (int)sprite->hurtbox.height, RED);
            }
        }

    EndMode2D();

    // cutscene stuff (textboxes etc) gets drawn relative to window position
    cutsceneManager.draw();

    // overlay debug info texts
    if (game->debug) {
        //std::string debugText = "Enemy: (" + std::to_string((int)spriteMap["npc"]->position.x) + "," + std::to_string((int)spriteMap["npc"]->position.y) + ")";
        //debugText += "NPC: (" + std::to_string((int)spriteMap["npc"]->rect.x) + "," + std::to_string((int)spriteMap["npc"]->rect.y) + ")";
        std::string debugText = "Debug: ";
        for (const auto& sprite : game->sprites) {
            if (sprite->spriteName == "skelet") {
                Sprite* weapon = getSprite("weapon_sword");
                if (weapon && CheckCollisionRecs(weapon->hurtbox, sprite->rect)) {
                    debugText += weapon->spriteName;
                    debugText += "  Hit!";
                }
            }
        }
        
        DrawText(debugText.c_str(), 4, game->gameScreenHeight - 22, 10, LIGHTGRAY);

        //DrawTextEx(game->loader.getFont("slkscr"), "Hello, World", Vector2{ 50, 50 }, 32, 0, WHITE);
    }
}

void InGame::end() {
    // wait for a split second
    WaitTime(0.25);
}