#include "InGame.h"
#include "raymath.h"
#include "Behavior.h"
#include "Controls.h"
#include "Events.h"
#include "Utils.h"
#include "TileMap.h"


void InGame::startup() {
    // create the player sprite
    // the "spriteName" argument has to match the texture keys (the part before the "_")
    player = std::make_shared<Sprite>(
        game, 0.0f, 0.0f, 14.0f, 12.0f, "player"
    );

    game.spriteMap["player"] = player;
    player->persistent = true;
    game.sprites.emplace_back(player);  // add to the sprites vector
    player->setTextures({ "player_idle", "player_run", "player_hit" });
    player->emitsLight = true; // TODO: for debugging, until I program the lamp item

    // check for existing loaded savegame data here
    // TODO put this in seperate function for less spaghetti
    auto saveData = game.getSaveData();
    if (saveData) {
        player->maxHealth = saveData->playerMaxHealth;
        player->health = std::max(static_cast<uint32_t>(6), saveData->playerHealth);
        // add the items once the scenes have fully started
        game.eventManager.pushDelayedEvent(UNNAMED, 0.1f, nullptr, [this, saveData]() {
            for (const auto& itemPair : saveData->items) {
                this->game.eventManager.pushEvent(ADD_ITEM, std::make_any<std::pair<std::string, uint32_t>>(itemPair.first, itemPair.second));
            }
            // equip the weapons
            this->game.eventManager.pushEvent(WEAPON_SET, std::make_pair(saveData->currentWeapons[0], 0));
            this->game.eventManager.pushEvent(WEAPON_SET, std::make_pair(saveData->currentWeapons[1], 1));
            });
        game.currentDungeon = loadDungeon(*saveData, game);

        // add NPCs that follow the player to the current room's data
        // TODO: is it worth it to give the TileMap a mutable member?
        tileMap = game.currentDungeon->loadCurrentTileMap();

        for (auto& sName : saveData->spritesFollowingPlayer) {
            TileObject npc = TileObject();
            npc.name = "npc";
            npc.type = "sprite";
            npc.x = player->position.x;
            npc.y = player->position.y;
            // TODO: are width and height even important?
            npc.width = 16.0;
            npc.height = 16.0;
            npc.visible = true;
            npc.id = 100; // TODO: make a getNextFreeID() function of TileMap

            nlohmann::json props = nlohmann::json::object();

            props["spriteName"] = sName;
            props["roomState"] = 0;

            npc.properties = props;
            
            tileMap->dynamicObjects.emplace_back(npc);
        }

    }
    else {
        // generate a fresh dungeon
        game.createDungeon(5, 4);
    }
    // retrieve the tilemap
    // and set the player's position in the first room
    loadTilemap();
    player->moveTo(7.5f * float(tileSize), float(8 * tileSize));

    // setup the camera
    camera.target = Vector2{ player->rect.x, player->rect.y };
    camera.offset = Vector2{ game.gameScreenWidth / 2.0f, game.gameScreenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    // event listeners for the InGame scene

    game.eventManager.addListener(MOVE_CAMERA, [&](std::any data) {
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

    // TODO: this might be needed in the future
    /*game.eventManager.addListener(TELEPORT, [this](std::any data) {
        const auto& teleportData = std::any_cast<const TeleportEvent&>(data);
        loadTilemap(teleportData.targetMap);
        player->moveTo(teleportData.targetPos.x * tileSize, teleportData.targetPos.y * tileSize);
        });*/

    game.eventManager.addListener(SET_MUSIC_VOLUME, [this](std::any data) {
            if (music) SetMusicVolume(*music, std::any_cast<float>(data));
        });

    // event listener that changes the current weapon key
    game.eventManager.addListener(WEAPON_SET, [this](const std::any& data) {
        if (data.has_value()) {
            auto [weapon, index] = std::any_cast<std::pair<std::string, size_t>>(data);
            if (index < currentWeapon.size()) {
                currentWeapon[index] = weapon.empty() ? std::nullopt : std::optional<std::string>{ weapon };
                // unequip if the same weapon happens to be in the other slot
                if (currentWeapon[(index + 1) % 2] == weapon) {
                    currentWeapon[(index + 1) % 2] = std::nullopt;
                }
            }
        }
        else {
            // Removal of weapon
            for (auto& w : currentWeapon) w = std::nullopt;
        }
        });

    // handles switching the lamp on/off
    game.eventManager.addListener(LAMP_ON, [this](const std::any& data) {
        lampIsOn = true;
        });

    game.eventManager.addListener(LAMP_OFF, [this](const std::any& data) {
        lampIsOn = false;
        });


    game.eventManager.addListener(SCREEN_SHAKE, [this](std::any value) {
        if (value.has_value() && value.type() == typeid(std::tuple<float, float, float>)) {
            auto [duration, xMag, yMag] = std::any_cast<std::tuple<float, float, float>>(value);
            cameraShake.start(duration, xMag, yMag);
        }
        });

    // ##### Events that progress the game ####
    setupConditionalEvents(*this);
}

Sprite* InGame::getSprite(const std::string& name) {
    auto it = game.spriteMap.find(name);
    if (it != game.spriteMap.end() && it->second) {
        return it->second.get();
    }
    return nullptr;
}

void InGame::spawnWeapon(size_t index)
{
    if (index >= currentWeapon.size() || !currentWeapon[index]) {
        TraceLog(LOG_WARNING, "No weapon equipped in slot %zu", index);
        return;
    }

    std::string weaponKey = *currentWeapon[index];

    // Get pre-built item data
    auto& itemData = game.inventory.getItemData();
    auto it = itemData.find(weaponKey);
    if (it == itemData.end() || !it->second.weaponBehavior.has_value()) {
        TraceLog(LOG_WARNING, "Invalid weapon in slot %zu: %s", index, weaponKey.c_str());
        return;
    }

    const weaponData& wpnData = *it->second.weaponBehavior;

    // Create sprite with pre-computed data
    auto wpn = std::make_shared<Sprite>(
        game, 0.0f, 0.0f, 16.0f, 16.0f, weaponKey
    );
    game.spriteMap[weaponKey] = wpn;
    game.sprites.emplace_back(wpn);

    wpn->setTextures({ weaponKey });
    wpn->setHurtbox(-1.0f, -1.0f, wpnData.HurtboxWidth, wpnData.HurtboxHeight);
    wpn->hurtboxOffset = { wpnData.HurtboxOffsetX, wpnData.HurtboxOffsetY };
    wpn->doesAnimate = false;
    wpn->isColliding = false;
    wpn->damage = wpnData.damage;

    wpn->addBehavior(std::make_unique<WeaponBehavior>(game, wpn, player, wpnData, index));

    // create a listener for when the weapon is finished
    int eventKey = EventKeyRegistry::getIndexedEventKey(KILL_WEAPON, index);
    game.eventManager.addListener(eventKey, [this, wpn, index, eventKey](std::any data) {
        game.spriteMap.erase(*currentWeapon[index]);
        wpn->markForDeletion();
        game.eventManager.removeListeners(eventKey);
        });

    game.playSound("slash");
}

void InGame::loadTilemap() {
    tileMap = game.currentDungeon->loadCurrentTileMap();
    // remove static and dynamic (non-persistent) sprites
    game.walls.clear();
    game.clearSprites();
    // check if there even is a valid tile map
    if (!tileMap)
        return;
    // the room state controls how objects are spawned
    // room states start with 1
    uint8_t currentState = game.currentDungeon->getCurrentRoomState();
    auto& objectStates = game.currentDungeon->getCurrentRoomObjectStates();
    const auto& spriteData = game.loader.getSpriteData();
    size_t spritesLen = tileMap->getObjects().size();
    game.sprites.reserve(spritesLen);

    // build objects from map data
    for (const auto& obj : tileMap->getObjects()) {
        processTileObject(game, obj, currentState, objectStates, spriteData);
    }
    // build objects that were placed in the data later on
    for (const auto& obj : tileMap->dynamicObjects) {
        processTileObject(game, obj, currentState, objectStates, spriteData);
    }

    // calculate the map dimensions (to be used by the camera)
    tileSize = tileMap->tileWidth;
    worldWidth = tileMap->width * tileSize;
    worldHeight = tileMap->height * tileSize;
    // Tile map calculations, used for rendering
    const Tileset& tileset = game.loader.getTileset(tileMap->getTilesetName());
    const Texture2D& texture = game.loader.getTextures(tileset.name)[0];
    const size_t tilesPerRow = tileset.columns;
    tileChunkSize = game.getSetting("tileChunkSize").get<size_t>();
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
                RenderTexture2D chunk = LoadRenderTexture(static_cast<int>(tileChunkSize), static_cast<int>(tileChunkSize));
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


void InGame::update(float deltaTime) {
    // control the sprites and apply physics

    // handle sprites that are dead (from last frame)
    for (const auto& sprite : game.sprites) {
        if (sprite && sprite->health < 1 && !sprite->dying) {
            sprite->dying = true;
            sprite->removeAllBehaviors();
            sprite->addBehavior(std::make_unique<DeathBehavior>(game, sprite, 2.0f));
            // TODO: unify these two events
            std::string eventStr = "killSprite_" + std::to_string(reinterpret_cast<uintptr_t>(sprite.get()));
            int eventKey = EventKeyRegistry::getEventKey(eventStr);
            game.eventManager.pushDelayedEvent(eventKey, 2.01f, nullptr, [this, sprite]() {
                sprite->markForDeletion();
                });
            std::string eventStr2 = "defeated_" + std::to_string(sprite->tileMapID);
            int eventKey2 = EventKeyRegistry::getEventKey(eventStr2);
            game.eventManager.pushEvent(eventKey2, sprite->tileMapID);
        }
    }
    // if a cutscene is active, it takes control over the player
    // otherwise, the player is controled by input
    game.cutsceneManager.update(deltaTime);
    if (!game.cutsceneManager.isActive()) {
        player->getControls();

        // spawn a weapon if the action button is pressed
        // primary weapon
        if ((game.buttonsPressed & CONTROL_ACTION2) && currentWeapon[0] && !getSprite(*currentWeapon[0])) {
            // spawn the weapon next to the player if not already there
            // TODO: it needs to be inside of a delayed event because of the quirks of the button polling...
            game.eventManager.pushDelayedEvent(UNNAMED, 0.0f, nullptr, [&]() {
                spawnWeapon(0);
                });
        }
        // secondary weapon
        if ((game.buttonsPressed & CONTROL_ACTION4) && currentWeapon[1] && !getSprite(*currentWeapon[1])) {
            game.eventManager.pushDelayedEvent(UNNAMED, 0.0f, nullptr, [&]() {
                spawnWeapon(1);
                });
        }

        // go to the inventory menu
        if (game.buttonsPressed & CONTROL_CONFIRM) {
            game.pauseScene(this->getName());
            game.startScene("InventoryUI");
            // TODO: make this a single-use event
            game.eventManager.addListener(INVENTORY_DONE, [this](std::any) {
                // return to this scene
                this->game.resumeScene(this->getName());
                });
            game.eventManager.pushEvent(SET_MUSIC_VOLUME, 0.3f);
        }
        // "select" menu
        if (game.buttonsPressed & CONTROL_CANCEL) {
            game.pauseScene(this->getName());
            game.sleepScene("HUD");
            game.startScene("SelectMenu");
            game.eventManager.addListener(SELECT_MENU_DONE, [this](std::any) {
                // return to this scene
                this->game.resumeScene(this->getName());
                game.wakeScene("HUD");
                });
            game.eventManager.pushEvent(SET_MUSIC_VOLUME, 0.3f);
        }
        for (const auto& sprite : game.sprites) {
            if (sprite) {
                sprite->executeBehavior(deltaTime);
                sprite->update(deltaTime);
            }
        }
    }
    // animate always, regardless of cutscene
    // also handle lights
    size_t currentLightIndex = 0;
    // draw a much bigger radius if the lamp is equipped
    // TODO: put these in the config
    const float lightRadius = (lampIsOn) ? 180.0f : 24.0f;

    for (int i = 0; i < MAX_LIGHTS; i++) {
        lights[i].active = false;
    }

    for (const auto& sprite : game.sprites) {
        // progress the animation index and change the textures if necessary
        sprite->animate(deltaTime);
        // check if the sprite emits light in dark rooms
        // and give it a light cone
        if (game.currentDungeon->isRoomDark() && sprite->emitsLight && currentLightIndex < MAX_LIGHTS) {
            lights[currentLightIndex].center = GetWorldToScreen2D(GetRectCenter(sprite->rect), camera);
            lights[currentLightIndex].center.y += sprite->z; // apply jump height
            lights[currentLightIndex].radius = lightRadius; // TODO
            lights[currentLightIndex].active = true;
            currentLightIndex++;
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
        for (size_t wpnIdx = 0; wpnIdx < 2; wpnIdx++) {
            if (sprite->isEnemy && currentWeapon[wpnIdx].has_value()) {
                Sprite* weapon = getSprite(*currentWeapon[wpnIdx]);
                if (weapon && sprite->iFrameTimer < 0.001f && sprite->health > 0 &&
                    CheckCollisionRecs(weapon->hurtbox, sprite->rect)) {
                    sprite->health = (weapon->damage > sprite->health) ? 0 : sprite->health - weapon->damage;
                    sprite->iFrameTimer = 0.5f;
                    applyKnockback(*weapon, *sprite, 8.0f);
                    game.playSound("creature_hurt_02");
                }
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
    auto [cols, _] = game.currentDungeon->getSize();
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
        // load the new room
        size_t newIndex = (uint8_t)game.currentDungeon->getCurrentRoomIndex() + offset;
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

    // debug functions
    if (game.debug) {
        if (game.buttonsPressed & CONTROL_DEBUG_K1) {
            size_t maxIndex = game.currentDungeon->getSize().first * game.currentDungeon->getSize().second;
            size_t newIndex = (game.currentDungeon->getCurrentRoomIndex() + 1) % maxIndex;
            game.currentDungeon->setCurrentRoomIndex(newIndex);
            loadTilemap();
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
            Vector2 drawPos = { static_cast<float>(chunkWorldX), static_cast<float>(chunkWorldY) };

            // chunks are flipped, so the src rect has to be flipped to draw the chunk correctly
            Rectangle src = { 0, 0, static_cast<float>(tileChunkSize), -1.0f * static_cast<float>(tileChunkSize) };
            Rectangle dst = { drawPos.x, drawPos.y, static_cast<float>(tileChunkSize), static_cast<float>(tileChunkSize) };
            Vector2 origin = { 0, 0 };
            DrawTexturePro(tilemapChunks[layerIndex][idx].texture, src, dst, origin, 0.0f, WHITE);
            if (game.debug) {
                DrawRectangleLines(static_cast<int>(drawPos.x), static_cast<int>(drawPos.y), static_cast<int>(tileChunkSize), static_cast<int>(tileChunkSize), RED);
            }
        }
    }
}

void InGame::draw() {
    ClearBackground(BLACK);

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
        DrawCircle((int)player->position.x, (int)player->position.y, 2, BLUE);
    }
    EndMode2D();

    // draw lighting in dark rooms
    // TODO: should game.target be passed as an argument to scene.draw() instead of being indirectly accessible to the scenes?
    if (game.currentDungeon->isRoomDark())
        DrawLightOverlay(game.target.texture, game.loader.getShader("light_mask"), lights, lightCount, static_cast<float>(game.gameScreenWidth), static_cast<float>(game.gameScreenHeight));

    // cutscene stuff (textboxes etc) gets drawn relative to window position
    game.cutsceneManager.draw();
}

void InGame::end() {
    // wait for a split second
    WaitTime(0.25);

    if (music) StopMusicStream(*music);
    music = nullptr;
}