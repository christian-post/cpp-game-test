#include "InGame.h"
#include "Game.h"
#include "Commands.h"
#include "raylib.h"
#include "raymath.h"
#include "Utils.h"
#include "Behavior.h"
#include "Controls.h"
#include <stdexcept>
#include "Events.h"


void InGame::startup() {
    // create the player sprite
    // the "spriteName" argument has to match the texture keys (the part before the "_")
    player = std::make_shared<Sprite>(
        game, 0.0f, 0.0f, 14.0f, 12.0f, "player", "player"
    );

    spriteMap["player"] = player;
    player->persistent = true;
    game.sprites.push_back(player);  // add to the sprites vector
    player->setTextures({ "player_idle", "player_run", "player_hit" });
    player->health = 3;

    // retrieve the tilemap
    // and set the player's position in the first room
    // some sprites need the player reference, so this has to come after the player
    // 
    //loadTilemap("test_map_large");
    //player->moveTo(float(44 * tileSize), float(73 * tileSize) + 4.0f); // for the large test map only
    //loadTilemap("test_dungeon1");
    loadTilemap("test_dungeon2");
    player->moveTo(float(10 * tileSize), float(15 * tileSize) + 4.0f); // test_dungeon2
    //loadTilemap("dungeon_shop");
    //player->moveTo(float(6 * tileSize), float(5 * tileSize) + 4.0f); 

    // setup the camera
    camera.target = Vector2{ player->rect.x, player->rect.y };
    camera.offset = Vector2{ game.gameScreenWidth / 2.0f, game.gameScreenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    // event listeners for the InGame scene
    game.eventManager.addListener("teleport", [this](std::any data) {
        const auto& teleportData = std::any_cast<const TeleportEvent&>(data);
        loadTilemap(teleportData.targetMap);
        player->moveTo(teleportData.targetPos.x * tileSize, teleportData.targetPos.y * tileSize);
        });

    game.eventManager.addListener("setMusicVolume", [this](std::any data) {
            if (music) SetMusicVolume(*music, std::any_cast<float>(data));
        });

    // event listener that changes the current weapon key
    game.eventManager.addListener("weaponSet", [this](const std::any& data) {
        if (data.has_value()) {
            currentWeapon = std::any_cast<std::string>(data);
        }
        else {
            // event allows for removal of the weapon
            currentWeapon = std::nullopt;
        }
        });

    // ##### Events that progress the game ####
    // // TODO: comment out during debugging
    //setupConditionalEvents(*this);

    // TODO: adding some items for testing
    game.eventManager.pushDelayedEvent("testItemsForStart", 0.1f, nullptr, [this]() {
        // give the player the sword for starters
        game.eventManager.pushEvent("addItem", std::make_any<std::pair<std::string, uint32_t>>("weapon_sword", 1));
        game.eventManager.pushEvent("addItem", std::make_any<std::pair<std::string, uint32_t>>("weapon_double_axe", 1));
        //game.eventManager.pushEvent("weaponSet", std::string("weapon_sword"));
        game.eventManager.pushEvent("weaponSet", std::string("weapon_double_axe"));
        // another item
        game.eventManager.pushEvent("addItem", std::make_any<std::pair<std::string, uint32_t>>("red_potion", 2));
        game.eventManager.pushEvent("addItem", std::make_any<std::pair<std::string, uint32_t>>("coin", 99));
        });
}

Sprite* InGame::getSprite(const std::string& name) {
    auto it = spriteMap.find(name);
    if (it != spriteMap.end() && it->second) {
        return it->second.get();
    }
    return nullptr;
}

void InGame::advanceRoomState(const std::string& name) {
    // room states are powers of 2
    roomStates[name] <<= 1;
    if (roomStates[name] == 0)
        roomStates[name] = 1;
    TraceLog(LOG_INFO, "room state of %s is now %d", name.c_str(), static_cast<int>(roomStates[name]));
}

void InGame::addBehaviorsToSprite(std::shared_ptr<Sprite> sprite, const std::vector<std::string>& behaviors, const nlohmann::json& behaviorData) {
    for (const auto& key : behaviors) {
        if (key == "RandomWalk") {
            sprite->addBehavior(std::make_unique<RandomWalkBehavior>(sprite));
        }
        else if (key == "Watch") {
            std::string targetName = behaviorData.value("watchTarget", "");
            if (spriteMap.find(targetName) != spriteMap.end()) {
                sprite->addBehavior(std::make_unique<WatchBehavior>(sprite, spriteMap[targetName]));
            }
            else {
                TraceLog(LOG_WARNING, "Target \"%s\" not found in spriteMap. Skipping WatchBehavior for %s.", targetName.c_str(), sprite->spriteName.c_str());
            }
        }
        else if (key == "Chase") {
            // TODO get distance values from file
            std::string targetName = behaviorData.value("chaseTarget", "");
            if (spriteMap.find(targetName) != spriteMap.end()) {
                sprite->addBehavior(std::make_unique<ChaseBehavior>(game, sprite, spriteMap[targetName], 48.0f, 2.0f, 64.0f));
            }
            else {
                TraceLog(LOG_WARNING, "Target \"%s\" not found in spriteMap. Skipping ChaseBehavior for %s.", targetName.c_str(), sprite->spriteName.c_str());
            }
        }
        else if (key == "Dialogue") {
            std::string textKey = behaviorData.value("dialogue", "");
            if (textKey.length()) {
                std::vector<std::string> texts = game.loader.getText(textKey);
                sprite->addBehavior(std::make_unique<DialogueBehavior>(game, sprite, player, texts));
            }
        }
    }
}

void InGame::loadTilemap(const std::string& name) {
    // TODO: this gets big, put this somewhere else
    tileMap = &game.loader.getTilemap(name);
    tileSize = tileMap->tileWidth;
    // remove static and dynamic (non-persistent) sprites
    game.walls.clear();
    game.clearSprites();

    // the room state controls how objects are spawned
    // states start with 1
    if (roomStates.find(name) == roomStates.end()) {
        roomStates[name] = 1;
    }
    uint8_t currentState = roomStates[name];

    TraceLog(LOG_INFO, "Loading room %s in state %d", name.c_str(), (int)currentState);

    const auto& spriteData = game.loader.getSpriteData();
    // build static collision objects from map data
    for (auto& obj : tileMap->getObjects()) {
        if (!obj.visible) continue;

        // first, check if the object should be in this state
        uint8_t objectState = obj.properties.value("roomState", 0); // objects spawn in every state by default
        TraceLog(LOG_INFO, "creating %s - <%s> objectState: %d",
            obj.type.c_str(), obj.name.empty() ? "unnamed" : obj.name.c_str(), objectState);

        if (objectState != 0 && (objectState & currentState) == 0)
            // object does not spawn in the currentState
            continue;

        if (obj.type == "wall") {
            game.walls.push_back(std::make_unique<Rectangle>(
                Rectangle{ obj.x, obj.y, obj.width, obj.height })
            );
        }
        else if (obj.type == "sprite") {
            std::string spriteName = obj.properties.value("spriteName", "sprite_default");
            const auto& data = spriteData.contains(spriteName)
                ? spriteData.at(spriteName)
                : spriteData.at("sprite_default");
            if (!spriteData.contains(spriteName)) {
                TraceLog(LOG_WARNING, "Missing sprite data for %s, falling back to sprite_default", spriteName.c_str());
            }
            const auto& defaultData = spriteData.at("sprite_default");
            // instanciate the sprite
            auto textureKey = data.contains("textureKey") ? data.at("textureKey").get<std::string>() : defaultData.at("textureKey").get<std::string>();
            auto sprite = std::make_shared<Sprite>(
                game, obj.x, obj.y, obj.width, obj.height, obj.name, textureKey
            );
            // generic attributes
            // from JSON data
            sprite->health = data.contains("health") ? data.at("health").get<int>() : defaultData.at("health").get<int>();
            sprite->damage = data.contains("damage") ? data.at("damage").get<int>() : defaultData.at("damage").get<int>();
            sprite->speed = data.contains("speed") ? data.at("speed").get<float>() : defaultData.at("speed").get<float>();
            sprite->knockback = data.contains("knockback") ? data.at("knockback").get<float>() : defaultData.at("knockback").get<float>();
            // attributes from Tiled data (instance-specific, overwrite JSON data)
            sprite->spriteName = spriteName;
            sprite->speed = obj.properties.value("speed", sprite->speed);
            sprite->damage = obj.properties.value("damage", sprite->damage);
            sprite->knockback = obj.properties.value("knockback", sprite->knockback);

            float hurtboxW = obj.properties.value("hurtboxW", 0.0f);
            float hurtboxH = obj.properties.value("hurtboxH", 0.0f);
            if (hurtboxW != 0.0f && hurtboxH != 0.0f) {
                sprite->setHurtbox(-1.0f, -1.0f, hurtboxW, hurtboxH);
            }

            // specific attributes
            // TODO: for persistent sprites, check if they exist in the spriteMap
            if (obj.name == "teleport") {
                sprite->isColliding = false;
                std::string targetMap = obj.properties.value("targetMap", "");
                float targetX = obj.properties.value("targetPosX", 0.0f);
                float targetY = obj.properties.value("targetPosY", 0.0f);
                sprite->addBehavior(std::make_unique<TeleportBehavior>(
                    game, sprite, player, targetMap,
                    Vector2{ targetX, targetY }
                ));
            }
            else if (obj.name == "npc" && !spriteMap[spriteName]) {
                spriteMap[spriteName] = sprite;
                sprite->setTextures({ textureKey + "_idle", textureKey + "_run" });
            }
            else if (obj.name == "tradeItem") {
                sprite->setTextures({ spriteName + "_idle" });
                sprite->doesAnimate = false;
                uint32_t cost = obj.properties.value("cost", 999);
                std::string name = obj.properties.value("name", "error"); // TODO switch spriteName and Name
                sprite->addBehavior(std::make_unique<TradeItemBehavior>(game, sprite, player, name, cost));
            }
            else if (obj.name == "enemy") {
                sprite->canHurtPlayer = true;
                sprite->isEnemy = true;
                sprite->setTextures({ textureKey + "_idle", textureKey + "_run" });
                // spawn the item drops if the enemy is defeated
                if (data.contains("itemDrops")) {
                    std::weak_ptr<Sprite> weakSprite = sprite;
                    std::string eventName = "killSprite_" + std::to_string(reinterpret_cast<uintptr_t>(sprite.get()));
                    game.eventManager.addListener(eventName, [this, weakSprite, data](std::any) {
                        auto s = weakSprite.lock();
                        if (!s) return;

                        float rand = static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f;
                        float accum = 0.0f;

                        for (const auto& drop : data["itemDrops"]) {
                            std::string itemId = drop.at(0);
                            float chance = drop.at(1);
                            accum += chance;

                            if (rand < accum) {
                                auto item = std::make_shared<Sprite>(
                                    game, s->position.x, s->position.y, 12.0f, 12.0f, itemId, itemId
                                );
                                item->setTextures({ itemId + "_idle" });
                                item->drawLayer = 1;
                                item->doesAnimate = false;
                                item->isColliding = false;
                                // TODO: this does not scale well. write a function that handles any itemID
                                if (itemId == "itemDropHeart" && player) {
                                    item->addBehavior(std::make_unique<HealBehavior>(game, item, player, 2));
                                }
                                else if (itemId == "itemDropCoin") {
                                    item->addBehavior(std::make_unique<CollectItemBehavior>(game, item, player, "coin", 1));
                                }
                                else if (itemId == "flask_big_red") {
                                    item->addBehavior(std::make_unique<CollectItemBehavior>(game, item, player, "red_potion", 1));
                                }
                               
                                game.sprites.push_back(item);
                                break;
                            }
                        }
                        });
                }
            }
            else if (obj.name == "door") {
                sprite->spriteName = spriteName;
                sprite->setTextures({ textureKey + "_idle" });
                sprite->doesAnimate = false;
                // TODO: set the open state in Tiled Data
                if (currentState == 1) {
                    sprite->staticCollision = true;
                }
                else {
                    sprite->currentFrame = 1;
                    sprite->staticCollision = false;
                }
                // door trigger
                std::string triggerKey = obj.properties.value("event", "");
                game.eventManager.addListener(triggerKey, [this, sprite](std::any) {
                    sprite->currentFrame = 1; // second anim frame has to be the opened door
                    sprite->staticCollision = false;
                    });
            }
            else if (obj.name == "hurt") {
                // invisible sprite with hurtbox (e.g. floor spikes)
                sprite->canHurtPlayer = true;
                sprite->visible = false;
                sprite->isColliding = false;
            }
            if (data.contains("behaviors")) {
                addBehaviorsToSprite(sprite, data.at("behaviors"), data.at("behaviorData"));
            }
            game.sprites.push_back(sprite);
        }
    }
    // calculate the map dimensions (to be used by the camera)
    worldWidth = tileMap->width * tileSize;
    worldHeight = tileMap->height * tileSize;

    // Tile map calculations, used for rendering
    const Tileset& tileset = game.loader.getTileset(tileMap->getTilesetName());
    const Texture2D& texture = game.loader.getTextures(tileset.name)[0];
    const size_t tilesPerRow = tileset.columns;
    const size_t tilesPerChunkX = tileChunkSize / tileSize;
    const size_t tilesPerChunkY = tileChunkSize / tileSize;
    numChunksX = (worldWidth + tileChunkSize - 1) / tileChunkSize;
    numChunksY = (worldHeight + tileChunkSize - 1) / tileChunkSize;

    // prepare the Tilemap texture chunks
    // TODO: draw chunks dynamically instead of storing them all beforehand?
    size_t totalLayers = tileMap->layers.size();
    tilemapChunks.resize(totalLayers);

    for (size_t layerIndex = 0; layerIndex < totalLayers; ++layerIndex) {
        const auto& layer = tileMap->getLayer(layerIndex);
        if (!layer.visible) continue;

        tilemapChunks[layerIndex].resize(numChunksX * numChunksY);

        for (size_t cy = 0; cy < numChunksY; ++cy) {
            for (size_t cx = 0; cx < numChunksX; ++cx) {
                size_t idx = cy * numChunksX + cx;
                RenderTexture2D chunk = LoadRenderTexture(tileChunkSize, tileChunkSize);
                BeginTextureMode(chunk);
                ClearBackground(BLANK);

                size_t startTileX = cx * tilesPerChunkX;
                size_t startTileY = cy * tilesPerChunkY;

                for (size_t y = 0; y < tilesPerChunkY; ++y) {
                    for (size_t x = 0; x < tilesPerChunkX; ++x) {
                        size_t mapX = startTileX + x;
                        size_t mapY = startTileY + y;
                        if (mapX >= tileMap->width || mapY >= tileMap->height) continue;

                        if (!layer.data[mapY][mapX]) continue; // 0 == transparent
                        int tileIndex = layer.data[mapY][mapX] - 1;

                        size_t tileX = ((size_t)tileIndex % tilesPerRow) * tileSize;
                        size_t tileY = ((size_t)tileIndex / tilesPerRow) * tileSize;
                        float srcX = std::clamp(static_cast<float>(tileX), 0.0f, static_cast<float>(texture.width - tileSize));
                        float srcY = std::clamp(static_cast<float>(tileY), 0.0f, static_cast<float>(texture.height - tileSize));
                        Rectangle src = { srcX, srcY, static_cast<float>(tileSize), static_cast<float>(tileSize) };

                        Vector2 pos = { static_cast<float>(x * tileSize), static_cast<float>(y * tileSize) };
                        DrawTextureRec(texture, src, pos, WHITE);
                    }
                }
                EndTextureMode();
                tilemapChunks[layerIndex][idx] = chunk;
            }
        }
    }
    // check if a different music track should be played
    const std::string musicKey = tileMap->getMusicKey();
    if (!musicKey.empty() && musicKey != currentMusicKey) {
        currentMusicKey = tileMap->getMusicKey();
        music = &const_cast<Music&>(game.loader.getMusic(currentMusicKey));
        PlayMusicStream(*music);
    }
}

void InGame::resolveAxisX(const std::shared_ptr<Sprite>& sprite, const Rectangle& obstacle) {
    if (!sprite->isColliding || !CheckCollisionRecs(sprite->rect, obstacle))
        return;

    float spriteCenterX = sprite->rect.x + sprite->rect.width * 0.5f;
    float obstacleCenterX = obstacle.x + obstacle.width * 0.5f;
    if (spriteCenterX < obstacleCenterX) {
        sprite->position.x = obstacle.x - sprite->rect.width;
    }
    else {
        sprite->position.x = obstacle.x + obstacle.width;
    }
    sprite->vel.x = 0.0f;
    sprite->rect.x = sprite->position.x;
}

void InGame::resolveAxisY(const std::shared_ptr<Sprite>& sprite, const Rectangle& obstacle) {
    if (!sprite->isColliding || !CheckCollisionRecs(sprite->rect, obstacle))
        return;

    float spriteCenterY = sprite->rect.y + sprite->rect.height * 0.5f;
    float obstacleCenterY = obstacle.y + obstacle.height * 0.5f;
    if (spriteCenterY < obstacleCenterY) {
        sprite->position.y = obstacle.y - sprite->rect.height;
    }
    else {
        sprite->position.y = obstacle.y + obstacle.height + 0.1f;
    }
    sprite->vel.y = 0.0f;
    sprite->rect.y = sprite->position.y;
}


void InGame::update(float deltaTime) {
    // control the sprites and apply physics

    // handle sprites that are dead (from last frame)
    for (const auto& sprite : game.sprites) {
        if (sprite && sprite->health < 1 && !sprite->dying) {
            sprite->dying = true;
            sprite->removeAllBehaviors();
            sprite->addBehavior(std::make_unique<DeathBehavior>(game, sprite, 2.0f));
            std::string eventName = "killSprite_" + std::to_string(reinterpret_cast<uintptr_t>(sprite.get()));
            game.eventManager.pushDelayedEvent(eventName, 2.01f, nullptr, [this, sprite]() {
                sprite->markForDeletion();
                });
        }
    }
    // if a cutscene is active, it takes control over the player
    // otherwise, the player is controled by input
    if (!game.cutsceneManager.isActive()) {
        player->getControls();

        // testing a weapon (sword)
        // TODO: needs to be more modular
        if (game.buttonsDown & CONTROL_ACTION2) {
            // spawn the weapon next to the player if not already there
            // TODO write a wrapper for this, or get weapon data from JSON file
            // and bind the Keys to events maybe?
            if (currentWeapon && !getSprite(*currentWeapon)) {
                std::string weaponKey = *currentWeapon;
                const auto& weaponData = game.loader.getSpriteData();

                const auto& data = weaponData.contains(weaponKey)
                    ? weaponData.at(weaponKey)
                    : weaponData.at("weapon_default");

                if (!weaponData.contains(weaponKey)) {
                    TraceLog(LOG_WARNING, "Missing weapon data for %s, falling back to weapon_default", weaponKey.c_str());
                }

                float offsetX = data.at("offsetX");
                float offsetY = data.at("offsetY");

                auto wpn = std::make_shared<Sprite>(
                    game, player->position.x + offsetX, player->position.y + offsetY, 16.0f, 16.0f, weaponKey, weaponKey
                );

                spriteMap[*currentWeapon] = wpn;
                game.sprites.push_back(wpn);

                wpn->setTextures({ weaponKey });
                wpn->setHurtbox(data.at("offsetX"), data.at("offsetY"), data.at("HurtboxWidth"), data.at("HurtboxHeight"));
                wpn->doesAnimate = false;
                wpn->isColliding = false;
                wpn->damage = data.at("damage");
                wpn->addBehavior(std::make_unique<WeaponBehavior>(game, wpn, player, data.at("lifetime")));

                // add an event listener that removes the sword
                // the "killWeapon" event is dispatched by WeaponBehavior once it's finished
                game.eventManager.addListener("killWeapon", [this, wpn](std::any) {
                    spriteMap.erase(*currentWeapon); // TODO: is this safe to do it here?
                    wpn->markForDeletion();
                    game.eventManager.removeListeners("killWeapon");
                    });

                // play the sound
                game.playSound("slash");
            }
        }

        if (game.buttonsPressed & CONTROL_CONFIRM) {
            // TODO: bind events to all the button functionality
            game.pauseScene(this->getName());
            game.startScene("InventoryUI");
            game.eventManager.addListener("InventoryDone", [this](std::any) {
                this->game.stopScene("InventoryUI");
                this->game.resumeScene(this->getName());
                });
            game.eventManager.pushEvent("setMusicVolume", 0.3f);
        }

        for (const auto& sprite : game.sprites) {
            if (sprite) {
                sprite->executeBehavior(deltaTime);
                sprite->update(deltaTime);
                sprite->animate(deltaTime);
            }
        }
    } else {
        game.cutsceneManager.update(deltaTime);
        for (const auto& sprite : game.sprites) {
            sprite->animate(deltaTime);
        }
    }

    // collision of sprites with static objects (walls)
    // TODO: make this a method of Sprite?
    // 
    // resolve collision in the X direction
    for (const auto& sprite : game.sprites) {
        sprite->rect.x = sprite->position.x;
        for (const auto& wall : game.walls) {
            resolveAxisX(sprite, *wall);
        }
        for (const auto& other : game.sprites) {
            if (other != sprite && other->staticCollision) {
                resolveAxisX(sprite, other->rect);
            }
        }

        sprite->rect.y = sprite->position.y;
        for (const auto& wall : game.walls) {
            resolveAxisY(sprite, *wall);
        }
        for (const auto& other : game.sprites) {
            if (other != sprite && other->staticCollision) {
                resolveAxisY(sprite, other->rect);
            }
        }

        // hurtbox centering midbottom
        sprite->hurtbox.x = sprite->rect.x + (sprite->rect.width - sprite->hurtbox.width) / 2 + sprite->hurtboxOffset.x;
        sprite->hurtbox.y = sprite->rect.y + (sprite->rect.height - sprite->hurtbox.height) + sprite->hurtboxOffset.y;

        // player damage
        if (sprite->canHurtPlayer && player->iFrameTimer < 0.001f && CheckCollisionRecs(sprite->hurtbox, player->rect)) {
            if (sprite->damage < player->health) {
                player->health -= sprite->damage;
            }
            else {
                player->health = 0;
            }
            player->iFrameTimer = game.getSetting("PlayeriFrames");
            applyKnockback(*sprite, *player, sprite->knockback);
            game.playSound("hurt1");
        }

        // weapon damage
        // everything that can hurt the player can also be damaged
        if (sprite->isEnemy) {
            Sprite* weapon = getSprite(*currentWeapon);
            if (weapon && sprite->iFrameTimer < 0.001f && sprite->health > 0 &&
                CheckCollisionRecs(weapon->hurtbox, sprite->rect)) {
                sprite->health = (weapon->damage > sprite->health) ? 0 : sprite->health - weapon->damage;
                sprite->iFrameTimer = 0.5f;
                applyKnockback(*weapon, *sprite, 8.0f);
                game.playSound("creature_hurt_02");
            }
        }
    }

    // Camera follows player
    camera.target = Vector2{ player->rect.x + player->rect.width / 2, player->rect.y + player->rect.height / 2 };
    // Define camera boundaries (and factor in the HUD dimensions)
    float HudHeight = game.getSetting("HudHeight");
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
        if (music) StopMusicStream(*music);
        game.stopScene("HUD");
        game.startScene("GameOver");
    }

    // TODO: debug mode stuff
    if (game.debug) {
        if (game.buttonsPressed & CONTROL_DEBUG_K1) {
            // teleport to next map
            currentRoomIndex = (currentRoomIndex + 1) % roomDebug.size();
            auto [targetMap, targetPos] = roomDebug[currentRoomIndex];

            TraceLog(LOG_INFO, "going to next room %s", targetMap.c_str());
            TeleportEvent event{ targetMap, targetPos };
            game.eventManager.pushEvent("teleport", std::any(event));
        }
    }
}

void InGame::drawTilemapChunks(int layerIndex) {
    float viewX = camera.target.x - (camera.offset.x / camera.zoom);
    float viewY = camera.target.y - (camera.offset.y / camera.zoom);

    for (size_t cy = 0; cy < numChunksY; ++cy) {
        for (size_t cx = 0; cx < numChunksX; ++cx) {
            size_t chunkWorldX = cx * tileChunkSize;
            size_t chunkWorldY = cy * tileChunkSize;

            // chunk is outside the camera fov
            if (chunkWorldX + tileChunkSize < viewX || chunkWorldX > viewX + game.gameScreenWidth / camera.zoom ||
                chunkWorldY + tileChunkSize < viewY || chunkWorldY > viewY + game.gameScreenHeight / camera.zoom)
                continue;

            size_t idx = cy * numChunksX + cx;
            Vector2 drawPos = { (float)chunkWorldX, (float)chunkWorldY };

            // chunks are flipped, so the src rect has to be flipped to draw the chunk correctly
            Rectangle src = { 0, 0, (float)tileChunkSize, -(float)tileChunkSize };
            Rectangle dst = { drawPos.x, drawPos.y, (float)tileChunkSize, (float)tileChunkSize };
            Vector2 origin = { 0, 0 };
            DrawTexturePro(tilemapChunks[layerIndex][idx].texture, src, dst, origin, 0.0f, WHITE);
            if (game.debug)
                DrawRectangleLines((int)drawPos.x, (int)drawPos.y, tileChunkSize, tileChunkSize, RED);
        }
    }
}

void InGame::draw() {
    ClearBackground(BLACK);

    BeginMode2D(camera);
        // draw the textures that are affected by the camera
        
        // draw each tilemap layer except the top one
        int totalLayers = static_cast<int>(tileMap->layers.size());
        int lastLayer = (totalLayers > 1) ? totalLayers - 1 : -1;

        for (int layerIndex = 0; layerIndex < totalLayers; ++layerIndex) {
            if (layerIndex == lastLayer || !tileMap->layers[layerIndex].visible) continue;
            drawTilemapChunks(layerIndex);
        }
 
        // Draw the sprites after sorting them by their bottom y position, also respect the drawing layer of each sprite (fixed)
        std::vector<Sprite*> drawOrder;
        drawOrder.reserve(game.sprites.size());
        for (const auto& sprite : game.sprites) {
            drawOrder.push_back(sprite.get());
        }
        std::sort(drawOrder.begin(), drawOrder.end(), [](Sprite* a, Sprite* b) {
            if (a->drawLayer != b->drawLayer)
                return a->drawLayer < b->drawLayer;
            return (a->rect.y + a->rect.height) < (b->rect.y + b->rect.height);
            });
        for (Sprite* sprite : drawOrder) {
            sprite->draw();
            sprite->drawBehavior();
        }
        // now draw the top layer above the sprites
        if (lastLayer >= 0 && tileMap->layers[lastLayer].visible) {
            drawTilemapChunks(lastLayer);
        }

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
    game.cutsceneManager.draw();
    // overlay debug info texts
    if (game.debug) {
        std::string debugText = "Debug: ";
        //debugText += "player vel.: " + std::to_string(Vector2Length(player->vel));

        //DrawTexture(game.loader.getTextures("sprite_default_idle")[0], 100.0f, 100.0f, WHITE);

        debugText += game.cutsceneManager.currentCommandName();
        DrawText(debugText.c_str(), 4, game.gameScreenHeight - 22, 10, LIGHTGRAY);
        //DrawTextEx(game.loader.getFont("slkscr"), "Hello, World", Vector2{ 50, 50 }, 32, 0, WHITE);
    }
}

void InGame::end() {
    // wait for a split second
    WaitTime(0.25);

    if (music) StopMusicStream(*music);
    music = nullptr;
}