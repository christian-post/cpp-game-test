#include "InGame.h"
#include "raymath.h"
#include "Behavior.h"
#include "Controls.h"
#include "Events.h"


void InGame::startup() {
    // create the player sprite
    // the "spriteName" argument has to match the texture keys (the part before the "_")
    player = std::make_shared<Sprite>(
        game, 0.0f, 0.0f, 14.0f, 12.0f, "player"
    );

    spriteMap["player"] = player;
    player->persistent = true;
    game.sprites.emplace_back(player);  // add to the sprites vector
    player->setTextures({ "player_idle", "player_run", "player_hit" });
    // retrieve the tilemap
    // and set the player's position in the first room
    game.createDungeon(4, 4);
    loadTilemap();
    player->moveTo(7.5f * float(tileSize), float(8 * tileSize));

    // setup the camera
    camera.target = Vector2{ player->rect.x, player->rect.y };
    camera.offset = Vector2{ game.gameScreenWidth / 2.0f, game.gameScreenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    // event listeners for the InGame scene

    game.eventManager.addListener("moveCamera", [&](std::any data) {
        auto [targetX, targetY] = std::any_cast<std::pair<float, float>>(data);
        // clamp to world boundaries
        // TODO: make this a function
        // TODO: take into account whether the HUD is visible or not
        float HudHeight = game.getSetting("HudHeight");
        float minX = game.gameScreenWidth / 2.0f;
        //float minY = game.gameScreenHeight / 2.0f - HudHeight;
        float minY = game.gameScreenHeight / 2.0f;
        float maxX = worldWidth - game.gameScreenWidth / 2.0f;
        float maxY = worldHeight - game.gameScreenHeight / 2.0f;
        //targetY -= HudHeight * 0.5f;
        targetX = Clamp(targetX, minX, maxX);
        targetY = Clamp(targetY, minY, maxY);
        camera.target = Vector2{ targetX, targetY };
        });

    /*game.eventManager.addListener("teleport", [this](std::any data) {
        const auto& teleportData = std::any_cast<const TeleportEvent&>(data);
        loadTilemap(teleportData.targetMap);
        player->moveTo(teleportData.targetPos.x * tileSize, teleportData.targetPos.y * tileSize);
        });*/

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

    game.eventManager.addListener("screenShake", [this](std::any value) {
        if (value.has_value() && value.type() == typeid(std::tuple<float, float, float>)) {
            auto [duration, xMag, yMag] = std::any_cast<std::tuple<float, float, float>>(value);
            cameraShake.start(duration, xMag, yMag);
        }
        });

    // ##### Events that progress the game ####
    // // TODO: comment out during debugging
    setupConditionalEvents(*this);

    // TODO: adding some items for testing
    //game.eventManager.pushDelayedEvent("testItemsForStart", 0.1f, nullptr, [this]() {
    //    // give the player the sword for starters
    //    game.eventManager.pushEvent("addItem", std::make_any<std::pair<std::string, uint32_t>>("weapon_double_axe", 1));
    //    game.eventManager.pushEvent("weaponSet", std::string("weapon_double_axe"));
    //    });
}

Sprite* InGame::getSprite(const std::string& name) {
    auto it = spriteMap.find(name);
    if (it != spriteMap.end() && it->second) {
        return it->second.get();
    }
    return nullptr;
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
                std::string voice = behaviorData.value("voice", "tone");
                sprite->addBehavior(std::make_unique<DialogueBehavior>(game, sprite, player, texts, voice));
            }
        }
        else if (key == "Shoot") {
            std::string targetName = behaviorData.value("shootTarget", "");
            sprite->addBehavior(std::make_unique<ShootBehavior>(game, sprite, spriteMap[targetName]));
        }
        else if (key == "Emitter") {
            // TODO: make the emitter and particle more customizable
            std::unique_ptr<Emitter> emitter = std::make_unique<Emitter>(20);
            emitter->spawnInterval = 0.2f;
            emitter->lifetimeVariance = 0.05f;
            emitter->velocityVariance = { 20.0f, 20.0f };

            std::unique_ptr<Particle> proto = std::make_unique<Particle>();
            proto->velocity = { 0.0f, 0.0f };
            proto->lifetime = 1.6f;
            proto->startAlpha = 0.5f;
            proto->endSize = 0.2f;
            proto->setAnimationFrames(game.loader.getTextures(behaviorData.value("particle", "")));

            emitter->prototype = *proto;
            sprite->addBehavior(std::make_unique<EmitterBehavior>(game, sprite, std::move(emitter), std::move(proto)));
        }
    }
}

void InGame::loadTilemap() {
    // TODO: this gets big, put this somewhere else
    tileMap = game.currentDungeon->loadCurrentTileMap();
    if (!tileMap) return;
    auto& objectStates = game.currentDungeon->getCurrentRoomObjectStates();
    // remove static and dynamic (non-persistent) sprites
    game.walls.clear();
    game.clearSprites();
    // the room state controls how objects are spawned
    // states start with 1
    uint8_t currentState = game.currentDungeon->getCurrentRoomState();

    const auto& spriteData = game.loader.getSpriteData();
    size_t spritesLen = tileMap->getObjects().size();
    game.sprites.reserve(spritesLen);
    // build static collision objects from map data
    for (auto& obj : tileMap->getObjects()) {
        if (!obj.visible) 
            continue;
        // first, check if the object should exist in this room state
        uint8_t objectState = obj.properties.value("roomState", 0); // objects spawn in every state by default
        TraceLog(LOG_INFO, "creating %s - <%s>, id: %d, objectState: %d",
            obj.type.c_str(), 
            obj.name.empty() ? "unnamed" : obj.name.c_str(), 
            obj.id, objectState
        );
        if (objectState != 0 && (objectState & currentState) == 0)
            // object does not spawn in the currentState
            continue;
        // object type-specific code
        if (obj.type == "wall") {
            game.walls.push_back(std::make_unique<Rectangle>(
                Rectangle{ obj.x, obj.y, obj.width, obj.height })
            );
        }
        else if (obj.type == "sprite") {
            if (objectStates[obj.id].isDefeated) {
                // this sprite is dead, skip it
                continue;
            }
            std::string spriteName = obj.properties.value("spriteName", "sprite_default");
            // get the data for this sprite from the JSON
            const auto& data = spriteData.contains(spriteName)
                ? spriteData.at(spriteName)
                : spriteData.at("sprite_default");
            if (!spriteData.contains(spriteName)) {
                TraceLog(LOG_WARNING, "Missing sprite data for %s, falling back to sprite_default", spriteName.c_str());
            }
            // store default data seperately to replace individual attributes
            const auto& defaultData = spriteData.at("sprite_default");
            auto textureKeys = data.contains("textures") ? data.at("textures").get<std::vector<std::string>>() : defaultData.at("textures").get<std::vector<std::string>>();
            // instanciate the sprite
            auto sprite = std::make_shared<Sprite>(
                game, obj.x, obj.y, obj.width, obj.height, obj.name
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
            sprite->tileMapID = obj.id;
            sprite->drawLayer = obj.properties.value("drawLayer", 0);
            float hurtboxW = obj.properties.value("hurtboxW", 0.0f);
            float hurtboxH = obj.properties.value("hurtboxH", 0.0f);
            if (hurtboxW != 0.0f && hurtboxH != 0.0f) {
                sprite->setHurtbox(-1.0f, -1.0f, hurtboxW, hurtboxH);
            }
            if (data.contains("collides")) {
                sprite->isColliding = static_cast<bool>(data.at("collides").get<int>());
            }
            // TODO: is this still needed?
            //if (obj.properties.contains("dialogue")) {
            //    data["behaviorData"]["dialogue"] = obj.properties["dialogue"].get<std::string>();
            //}

            // specific sprite attributes
            // TODO: for persistent sprites, check if they exist in the spriteMap
            if (obj.name == "teleport") {
                sprite->isColliding = false;
                sprite->visible = false;
                std::string targetMap = obj.properties.value("targetMap", "");
                float targetX = obj.properties.value("targetPosX", 0.0f);
                float targetY = obj.properties.value("targetPosY", 0.0f);
                sprite->addBehavior(std::make_unique<TeleportBehavior>(
                    game, sprite, player, targetMap,
                    Vector2{ targetX, targetY }
                ));
            }
            else if (obj.name == "npc") {
                if (!spriteMap[spriteName]) {
                    // // TODO: handle this differently, this might create empty references
                    spriteMap[spriteName] = sprite;
                }
                sprite->setTextures(textureKeys);
            }
            else if (obj.name == "tradeItem") {
                sprite->setTextures(std::vector<std::string>{ spriteName });
                sprite->doesAnimate = false;
                uint32_t cost = obj.properties.value("cost", 999);
                std::string name = obj.properties.value("name", "error"); // TODO switch spriteName and Name
                sprite->addBehavior(std::make_unique<TradeItemBehavior>(game, sprite, player, name, cost));
            }
            else if (obj.name == "enemy") {
                sprite->canHurtPlayer = true;
                sprite->isEnemy = true;
                sprite->setTextures(textureKeys);
                // spawn the item drops if the enemy is defeated
                if (data.contains("itemDrops")) {
                    std::weak_ptr<Sprite> weakSprite = sprite;
                    std::string eventName = "killSprite_" + std::to_string(reinterpret_cast<uintptr_t>(sprite.get()));
                    game.eventManager.addListener(eventName, [this, weakSprite, data](std::any) {
                        auto s = weakSprite.lock();
                        if (!s) 
                            return;
                        float rand = static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f;
                        float accum = 0.0f;
                        for (const auto& drop : data["itemDrops"]) {
                            std::string itemId = drop.at(0);
                            float chance = drop.at(1);
                            accum += chance;
                            if (rand < accum) {
                                auto item = std::make_shared<Sprite>(
                                    game, s->position.x, s->position.y, 12.0f, 12.0f, itemId
                                );
                                item->setTextures(std::vector<std::string>{ itemId });
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
                                game.sprites.emplace_back(item);
                                break;
                            }
                        }
                        });
                }
            }
            else if (obj.name == "door") {
                sprite->spriteName = spriteName;
                sprite->setTextures(textureKeys);
                sprite->doesAnimate = false;
                // TODO: set the open state in Tiled Data
                uint8_t openState = obj.properties.value("openState", 0);
                if (currentState < openState) {
                    sprite->staticCollision = true;
                    bool locked = obj.properties.value("locked", false);
                    if (locked) {
                        sprite->currentFrame = 2;
                        sprite->addBehavior(std::make_unique<OpenLockBehavior>(game, sprite, player));
                    }
                }
                else {
                    sprite->currentFrame = 1;
                    sprite->staticCollision = false;
                }
                // door trigger
                std::string triggerKey = obj.properties.value("event", "");
                game.eventManager.removeListeners(triggerKey);
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
            else if (obj.name == "chest") {
                sprite->doesAnimate = false;
                sprite->staticCollision = true;
                sprite->setTextures({ "chest" });

                if (objectStates[obj.id].isOpened) {
                    sprite->currentFrame = 2;
                }
                else {
                    std::string eventKey = "chest_opened_" + std::to_string(obj.id);
                    game.eventManager.removeListeners(eventKey);
                    game.eventManager.addListener(eventKey, [&](std::any data) {
                        uint32_t eventId = std::any_cast<uint32_t>(data);
                        if (eventId == obj.id) {
                            objectStates[obj.id].isOpened = true;
                        }
                        });
                    sprite->addBehavior(std::make_unique<ChestBehavior>(game, sprite, player, static_cast<std::string>(obj.properties.value("item", "coin")), static_cast<uint32_t>(obj.properties.value("amount", 999))));
                }
            }
            // add an event that changes the isDefeated field for this sprite
            std::string eventKey = "defeated_" + std::to_string(obj.id);
            game.eventManager.removeListeners(eventKey);
            game.eventManager.addListener(eventKey, [&](std::any data) {
                uint32_t eventId = std::any_cast<uint32_t>(data);
                auto& currentRoomObjectStates = game.currentDungeon->getCurrentRoomObjectStates();
                if (eventId == obj.id) {
                    currentRoomObjectStates[obj.id].isDefeated = true;
                }
                });
            if (data.contains("behaviors")) {
                addBehaviorsToSprite(sprite, data.at("behaviors"), data.at("behaviorData"));
            }
            game.sprites.emplace_back(sprite);
        }
    }
    // calculate the map dimensions (to be used by the camera)
    tileSize = tileMap->tileWidth;
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
        if (!layer.visible) 
            continue;
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
    // check for NPCs that follow the player
    for (const auto& sprite : game.sprites) {
        if (sprite->followsPlayer) {
            sprite->moveTo(player->position.x, player->position.y);
        }
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
            // TODO: unify these two events
            std::string eventName = "killSprite_" + std::to_string(reinterpret_cast<uintptr_t>(sprite.get()));
            game.eventManager.pushDelayedEvent(eventName, 2.01f, nullptr, [this, sprite]() {
                sprite->markForDeletion();
                });
            std::string eventKey = "defeated_" + std::to_string(sprite->tileMapID);
            game.eventManager.pushEvent(eventKey, sprite->tileMapID);
        }
    }
    // if a cutscene is active, it takes control over the player
    // otherwise, the player is controled by input
    game.cutsceneManager.update(deltaTime);
    if (!game.cutsceneManager.isActive()) {
        player->getControls();
        // testing a weapon
        // TODO: needs to be in its own function
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

                float offsetX = data.at("HurtboxOffsetX");
                float offsetY = data.at("HurtboxOffsetY");

                auto wpn = std::make_shared<Sprite>(
                    game, 0.0f, 0.0f, 16.0f, 16.0f, weaponKey
                ); // hitbox doesn't really matter

                spriteMap[*currentWeapon] = wpn;
                game.sprites.emplace_back(wpn);

                wpn->setTextures({ weaponKey });
                wpn->setHurtbox(-1.0f, -1.0f, data.at("HurtboxWidth"), data.at("HurtboxHeight"));
                wpn->hurtboxOffset = { offsetX, offsetY };
                wpn->doesAnimate = false;
                wpn->isColliding = false;
                wpn->damage = data.at("damage");
                weaponType type = static_cast<weaponType>(data.at("type"));
                wpn->addBehavior(std::make_unique<WeaponBehavior>(game, wpn, player, data.at("lifetime"), type));

                // add an event listener that removes the sword
                // the "killWeapon" event is dispatched by WeaponBehavior once it's finished
                game.eventManager.addListener("killWeapon", [this, wpn](std::any) {
                    spriteMap.erase(*currentWeapon); // TODO: is this safe to do it here?
                    wpn->markForDeletion();
                    game.eventManager.removeListeners("killWeapon");
                    });
                game.playSound("slash");
            }
        }
        if (game.buttonsPressed & CONTROL_CONFIRM) {
            // TODO: bind events to all the button functionality
            game.pauseScene(this->getName());
            game.startScene("InventoryUI");
            game.eventManager.addListener("InventoryDone", [this](std::any) {
                this->game.resumeScene(this->getName());
                });
            game.eventManager.pushEvent("setMusicVolume", 0.3f);
        }
        for (const auto& sprite : game.sprites) {
            if (sprite) {
                sprite->executeBehavior(deltaTime);
                sprite->update(deltaTime);
            }
        }
    }
    // animate always, regardless of cutscene
    for (const auto& sprite : game.sprites) {
        sprite->animate(deltaTime);
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
        if (sprite->isEnemy && currentWeapon.has_value()) {
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

    // particles
    for (auto& emitter : game.emitters) {
        emitter.update(deltaTime);
    }

    // Camera follows the player (center)
    Vector2 target = {
        player->rect.x + player->rect.width / 2,
        player->rect.y + player->rect.height / 2
    };

    float HudHeight = game.getSetting("HudHeight");
    float minX = game.gameScreenWidth / 2.0f;
    float minY = game.gameScreenHeight / 2.0f - HudHeight;
    float maxX = worldWidth - game.gameScreenWidth / 2.0f;
    float maxY = worldHeight - game.gameScreenHeight / 2.0f;
    target.y -= HudHeight * 0.5f; // TODO: is this really correct?
    target.x = Clamp(target.x, minX, maxX);
    target.y = Clamp(target.y, minY, maxY);
    // apply a screen shake effect if the event was called
    if (cameraShake.isActive()) {
        cameraShake.update(deltaTime);
        target = cameraShake.apply(target);
    }
    if (!game.cutsceneManager.hasCameraControl()) {
        camera.target = target;
    }

    // check player out of map bounds
    // TODO: make a function for this
    int8_t offset = 0;
    auto [_, cols] = game.currentDungeon->getSize();
    if (player->rect.x < 0.0f) {
        offset = -1;
        player->moveTo(worldWidth - player->rect.width * 1.5f, player->position.y);
    }
    else if (player->rect.x + player->rect.width > worldWidth) {
        offset = 1;
        player->moveTo(player->rect.width * 0.5f, player->position.y);
    }
    else if (player->rect.y < 0.0f) {
        offset = int8_t(cols) * -1;
        player->moveTo(player->position.x, worldHeight - player->rect.height * 1.5f);
    }
    else if (player->rect.y + player->rect.height > worldHeight) {
        offset = int8_t(cols);
        player->moveTo(player->position.x, player->rect.height * 0.5f);
    }
    if (offset != 0) {
        uint8_t newIndex = (uint8_t)game.currentDungeon->getCurrentRoomIndex() + offset;
        game.currentDungeon->setCurrentRoomIndex(newIndex);
        loadTilemap();
    }

    // player dies, GameOver scene starts
    if (player->health < 1) {
        game.pauseScene(getName());
        if (music) StopMusicStream(*music);
        game.stopScene("HUD");
        game.startScene("GameOver");
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
            if (game.debug) {
                DrawCircle((int)camera.target.x, (int)camera.target.y, 8, WHITE);
                DrawRectangleLines((int)drawPos.x, (int)drawPos.y, tileChunkSize, tileChunkSize, RED);
            }
        }
    }
}

void InGame::draw() {
    ClearBackground(RED);  // red just for camera debugging

    BeginMode2D(camera); // draw the textures that are affected by the camera
    // draw each tilemap layer except the top one
    int lastLayer = 0;
    if (tileMap) {
        int totalLayers = static_cast<int>(tileMap->layers.size());
        lastLayer = (totalLayers > 1) ? totalLayers - 1 : -1;
        for (int layerIndex = 0; layerIndex < totalLayers; ++layerIndex) {
            if (layerIndex == lastLayer || !tileMap->layers[layerIndex].visible) continue;
            drawTilemapChunks(layerIndex);
        }
    }
    // Draw the sprites after sorting them by their bottom y position, also respect the drawing layer of each sprite (fixed)
    // TODO add a flag to sprite that makes an exception from this sorting
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
    // particles
    for (auto& emitter : game.emitters) {
        emitter.draw();
    }
    if (tileMap) {
        // now draw the top layer above the sprites
        if (lastLayer >= 0 && tileMap->layers[lastLayer].visible) {
            drawTilemapChunks(lastLayer);
        }
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
        // show the player's z velocity
        debugText += "player z vel: " + std::to_string(player->vz);
        DrawText(debugText.c_str(), 4, game.gameScreenHeight - 22, 10, LIGHTGRAY);
    }
}

void InGame::end() {
    // wait for a split second
    WaitTime(0.25);

    if (music) StopMusicStream(*music);
    music = nullptr;
}