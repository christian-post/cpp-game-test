#include "InGame.h"
#include "Controls.h"
#include "Events.h"
#include "Utils.h"
#include "TileMap.h"
#include "Savegame.h"
#include "Emitter.h"
#include "Behavior.h"
#include "DeathBehavior.h"
#include "WeaponBehavior.h"
#include "raymath.h"
#include "LuaEventManager.h"


InGame::InGame(Game& game, const std::string& name) : Scene(game, name), tileMap(nullptr), tilemapRenderer(game), cameraController(game), luaEventManager(std::make_unique<LuaEventManager>(game, *this)) {}

void InGame::startup()
{
    // create the player sprite
    // the "spriteName" argument has to match the texture keys (the part before the "_")
    player = std::make_shared<Sprite>(game, 0.0f, 0.0f, 14.0f, 12.0f, "player");

    game.spriteMap["player"] = player;
    player->health = game.getSetting<uint32_t>("playerStartingHealth");
    player->maxHealth = game.getSetting<uint32_t>("playerStartingMaxHealth");
    player->persistent = true;
    game.sprites.emplace_back(player);  // add to the sprites vector
    player->setTextures({ "player_idle", "player_run", "player_hit" });
    player->emitsLight = true; // TODO: for debugging, until I program the lamp item

    // instantiate a hit effect emitter
    wpnHitEffect = createEmitter(game, "hitEffect");
    wpnHitEffect->persistent = true;
    wpnHitEffect->stop();
    game.emitters.push_back(wpnHitEffect);

    // check for existing loaded savegame data here
    auto saveData = game.getSaveData();
    if (saveData)
    {
        // TODO broken
        loadWorldFromSave(saveData);
    }
    else
    {
        // start in the overworld
        std::string worldKey = "overworld";
        game.createWorld(worldKey, false);
    }
    // retrieve the tilemap
    // and set the player's position in the first room
    loadTilemap();
    player->moveTo(game.currentWorld->startingPosition.x, game.currentWorld->startingPosition.y);

    // setup the camera
    cameraController.initialize(
        static_cast<float>(game.gameScreenWidth),
        static_cast<float>(game.gameScreenHeight)
    );
    cameraController.setTarget(player.get());

    // assign button functions
    setupInputCallbacks();
     
    // Event listeners specific to the InGame scene
    setupEventListeners();
    setupConditionalEvents(*this);
}

void InGame::setupEventListeners()
{
    game.eventManager.addListener(SET_MUSIC_VOLUME, [this](std::any data) {
        if (music) 
            SetMusicVolume(*music, std::any_cast<float>(data));
        });

    game.eventManager.addListener(WEAPON_SET, [this](const std::any& data) {
        // assign a weapon to one of two buttons
        if (data.has_value())
        {
            auto [weapon, index] = std::any_cast<std::pair<std::string, size_t>>(data);
            if (index < currentWeapon.size())
            {
                currentWeapon[index] = weapon.empty() ? std::nullopt : std::make_optional(std::make_pair(weapon, false));
                size_t otherIdx = (index + 1) % 2;
                if (currentWeapon[otherIdx].has_value() && currentWeapon[otherIdx]->first == weapon) {
                    currentWeapon[otherIdx] = std::nullopt;
                }
            }
        }
        else
        {
            for (auto& w : currentWeapon) 
                w = std::nullopt;
        }
        });

    game.eventManager.addListener(LAMP_ON, [this](const std::any& data) {
        lampIsOn = true;
        });

    game.eventManager.addListener(LAMP_OFF, [this](const std::any& data) {
        lampIsOn = false;
        });

    game.eventManager.addListener(LOCK_PLAYER_MOVEMENT, [this](const std::any&) {
        playerMovementLocked = true;
        });

    game.eventManager.addListener(UNLOCK_PLAYER_MOVEMENT, [this](const std::any&) {
        playerMovementLocked = false;
        });

    game.eventManager.addListener(RELOAD_ROOM, [this](const std::any&) {
        loadTilemap();
        });

    game.eventManager.addListener(TELEPORT, [this](const std::any& data) {
        auto [targetWorld, targetLevel, targetIndex, targetPos] = std::any_cast<TeleportEvent>(data);
        if (game.currentWorld->name != targetWorld)
        {
            // go to different world
            // this loading callback will be handled by the WorldTransition scene
            game.startScene("WorldTransition");

            // save the current world for later
            game.loader.loadQueue.emplace("Saving World", [this, data]() {
                if (!game.savegame)
                    // create an empty SaveGame object where there currently is none
                    game.savegame = std::make_shared<SaveGame>();

                saveWorld(*game.savegame, *game.currentWorld);
                });

            game.loader.loadQueue.emplace("Loading World", [this, data]() {
                auto [targetWorld, targetLevel, targetIndex, targetPos] = std::any_cast<TeleportEvent>(data);

                // check if the last loaded saveGame contains data
                if (!game.savegame || game.savegame->worldData.find(targetWorld) == game.savegame->worldData.end())
                {
                    game.createWorld(targetWorld, (targetWorld != "overworld"));
                }
                else
                {
                    // savegame contains this world's data
                    // TODO this always regenerates the minimap textures. maybe cache them, or some of them?
                    loadWorld(*game.savegame, game, targetWorld);
                }

                game.currentWorld->currentLevel = targetLevel;
                game.currentWorld->currentRoomIndex = targetIndex;
                loadTilemap();
                player->moveTo(targetPos.x, targetPos.y);
                // check for companions
                for (const auto& sprite : game.sprites)
                {
                    if (sprite->followsPlayer) 
                        sprite->moveTo(player->position.x, player->position.y);
                }
                });
        }
        else
        {
            game.currentWorld->currentLevel = targetLevel;
            game.currentWorld->currentRoomIndex = targetIndex;
            loadTilemap();
            player->moveTo(targetPos.x, targetPos.y);
        }
        });
}

void InGame::handleDeadSprites()
{
    // process sprites that are dead (from last frame)
    for (const auto& sprite : game.sprites)
    {
        if (sprite && sprite->health < 1 && !sprite->dying)
        {
            sprite->dying = true;
            // a sprite with a state machine is handled differently
            if (sprite->stateMachine)
            {
                sprite->stateMachine->addBehavior("death", std::make_unique<DeathBehavior>(game, sprite, 2.0f));
                auto state = std::make_unique<State>("dying");
                state->activeBehaviorKeys.push_back("death");
                sprite->stateMachine->addState(std::move(state));
                sprite->stateMachine->transitionTo("dying");
            }
            else
            {
                sprite->removeAllBehaviors();
                sprite->addBehavior(std::make_unique<DeathBehavior>(game, sprite, 2.0f));
            }

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
}

void InGame::setupInputCallbacks()
{
    // Bind callbacks to specific buttons
    buttonCallbacks[CONTROL_ACTION2] = [this]() { onActionButton2(); };
    buttonCallbacks[CONTROL_ACTION3] = [this]() { onActionButton3(); };
    buttonCallbacks[CONTROL_CONFIRM] = [this]() { onInventoryButton(); };
    buttonCallbacks[CONTROL_CANCEL] = [this]() { onMenuButton(); };
    // debug functions
    buttonCallbacks[CONTROL_DEBUG2] = [this]() { onDebugMenuButton(); };
    buttonCallbacks[CONTROL_DEBUG_K1] = [this]() { onDebugButton1(); };
    buttonCallbacks[CONTROL_DEBUG_K3] = [this]() { onDebugButton3(); };
}

void InGame::onActionButton2()
{
    // primary weapon
    if (currentWeapon[0] && !getSprite(currentWeapon[0]->first))
    {
        // spawn the weapon next to the player if not already there
        // This needs to be inside of a delayed event because of the quirks of the button polling...
        game.eventManager.pushDelayedEvent(UNNAMED, 0.0f, nullptr, [&]() {
            spawnWeapon(0);
            currentWeapon[0]->second = true; // weapon active
            });
    }
}

void InGame::onActionButton3()
{
    // secondary weapon
    if (currentWeapon[1] && !getSprite(currentWeapon[1]->first))
    {
        game.eventManager.pushDelayedEvent(UNNAMED, 0.0f, nullptr, [&]() {
            spawnWeapon(1);
            currentWeapon[1]->second = true;
            });
    }
}

void InGame::onInventoryButton()
{
    game.pauseScene(this->getName());
    game.startScene("InventoryUI");
    // TODO: make this a single-use event
    game.eventManager.addListener(INVENTORY_DONE, [this](std::any) {
        // return to this scene
        this->game.resumeScene(this->getName());
        });
    game.eventManager.pushEvent(SET_MUSIC_VOLUME, 0.3f);
}

void InGame::onMenuButton()
{
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

void InGame::onDebugMenuButton()
{
    game.pauseScene(this->getName());
    game.sleepScene("HUD");
    game.startScene("DebugMenu");
    game.eventManager.addListener(SELECT_MENU_DONE, [this](std::any) {
        this->game.resumeScene(this->getName());
        game.wakeScene("HUD");
        });
}

void InGame::onDebugButton1()
{
    if (!game.debug)
        return;
    // advance room the index and immediately change the room 
    size_t maxIndex = game.currentWorld->getSize().first * game.currentWorld->getSize().second;
    size_t newIndex = (game.currentWorld->currentRoomIndex + 1) % maxIndex;
    game.currentWorld->currentRoomIndex = newIndex;
    game.eventManager.pushDelayedEvent(UNNAMED, 0.0f, nullptr, [&]() {
        loadTilemap();
        });
}

void InGame::onDebugButton3()
{
    if (!game.debug)
        return;
    cameraHasBounds = !cameraHasBounds;
    player->isColliding = !player->isColliding;
    TraceLog(LOG_INFO, "Toggled camera bounds, %d", cameraHasBounds);
    TraceLog(LOG_INFO, "Toggled no clip, %d", player->isColliding);
}

void InGame::handlePlayerInput(float deltaTime)
{
    if (game.cutsceneManager.isActive())
        return;

    for (const auto& [button, callback] : buttonCallbacks)
    {
        if (game.buttonsPressed & button)
            callback();
    }

    // Don't allow movement if player is locked
    if (!playerMovementLocked)
        player->getControls();
}

void InGame::takeScreenshot()
{
    if (game.lastScreenshot)
    {
        UnloadImage(*game.lastScreenshot);
        game.lastScreenshot.reset();
    }

    game.lastScreenshot = std::make_unique<Image>(LoadImageFromTexture(game.target.texture));
}

void InGame::loadWorldFromSave(std::shared_ptr<SaveGame> save)
{
    player->maxHealth = save->playerMaxHealth;
    player->health = std::max(static_cast<uint32_t>(6), save->playerHealth);

    // Delayed event that adds the items and equips the weapons
    game.eventManager.pushDelayedEvent(UNNAMED, 0.1f, nullptr, [this, save]() {
        for (const auto& itemPair : save->items) {
            this->game.eventManager.pushEvent(ADD_ITEM,
                std::make_any<std::pair<std::string, uint32_t>>(itemPair.first, itemPair.second));
        }
        this->game.eventManager.pushEvent(WEAPON_SET, std::pair<std::string, size_t>(save->currentWeapons[0], 0));
        this->game.eventManager.pushEvent(WEAPON_SET, std::pair<std::string, size_t>(save->currentWeapons[1], 1));
        });

    std::string& name = save->lastWorld;
    loadWorld(*save, game, name);

    // add NPCs that follow the player to the current room's data
    // TODO: is it worth it to give the TileMap a mutable member?
    tileMap = game.currentWorld->getCurrentTileMap();

    for (auto& sName : save->spritesFollowingPlayer)
    {
        TileObject npc = TileObject();
        npc.name = "npc";
        npc.type = "sprite";
        // TODO: are width and height even important?
        npc.width = 16.0;
        npc.height = 16.0;
        npc.visible = true;
        npc.id = 100; // TODO: make a getNextFreeID() function of TileMap

        nlohmann::json props = nlohmann::json::object();

        props["spriteName"] = sName;
        props["roomState"] = 0;
        props["persistent"] = true;

        npc.properties = props;

        tileMap->dynamicObjects.emplace_back(npc);

        // position the npc next to the player after they were repositioned
        game.eventManager.pushDelayedEvent(UNNAMED, 0.1f, nullptr, [this, sName]() {
            getSprite(sName)->moveTo(player->position.x, player->position.y);
            });
    }
}

Sprite* InGame::getSprite(const std::string& name) {
    auto it = game.spriteMap.find(name);
    if (it != game.spriteMap.end() && it->second)
        return it->second.get();
    return nullptr;
}

void InGame::spawnWeapon(size_t index)
{
    if (index >= currentWeapon.size() || !currentWeapon[index])
    {
        TraceLog(LOG_WARNING, "No weapon equipped in slot %zu", index);
        return;
    }

    std::string weaponKey = currentWeapon[index]->first;

    // Get pre-built item data
    auto& itemData = game.inventory.getItemData();
    auto it = itemData.find(weaponKey);
    if (it == itemData.end() || !it->second.weaponBehavior.has_value())
    {
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
    wpn->isColliding = false;
    wpn->damage = wpnData.damage;

    wpn->addBehavior(std::make_unique<WeaponBehavior>(game, wpn, player, wpnData, index));

    // create a listener for when the weapon is finished
    int eventKey = EventKeyRegistry::getIndexedEventKey(KILL_WEAPON, index);
    game.eventManager.addListener(eventKey, [this, wpn, index, eventKey](std::any data) {
        game.spriteMap.erase(currentWeapon[index]->first);
        wpn->markForDeletion();
        currentWeapon[index]->second = false;
        game.eventManager.pushDelayedEvent(UNNAMED, 0.0f, nullptr, [this, eventKey]() {
            game.eventManager.removeListeners(eventKey);
            });
        });

    game.playSound(wpnData.soundKey); // TODO put this in WeaponBehavior
}

void InGame::checkRoomTransition()
{
    // tests if the player rect is outside of the world bounds
    // returns an offset which can be used to displace the map index
    int8_t offset = 0;
    Vector2 newPlayerPos = player->position; // store the new position

    auto [cols, _] = game.currentWorld->getSize();
    if (player->rect.x < 0.0f)
    {
        offset = -1;
        newPlayerPos = { tilemapRenderer.getWorldWidth() * tilemapRenderer.getTileSize() - player->rect.width * 1.5f, player->position.y };
    }
    else if (player->rect.x + player->rect.width > tilemapRenderer.getWorldWidth() * tilemapRenderer.getTileSize())
    {
        offset = 1;
        newPlayerPos = { player->rect.width * 0.5f, player->position.y };
    }
    else if (player->rect.y < 0.0f)
    {
        offset = int8_t(cols) * -1;
        newPlayerPos = { player->position.x, tilemapRenderer.getWorldHeight() * tilemapRenderer.getTileSize() - player->rect.height * 1.5f };
    }
    else if (player->rect.y + player->rect.height > tilemapRenderer.getWorldHeight() * tilemapRenderer.getTileSize())
    {
        offset = int8_t(cols);
        newPlayerPos = { player->position.x, player->rect.height * 0.5f };
    }

    // change the room if the offset is some value
    if (offset != 0)
    {
        // load the new room, also make sure that the new index is not negative
        size_t newIndex = std::max(0, static_cast<int8_t>(game.currentWorld->currentRoomIndex) + offset);
        game.currentWorld->currentRoomIndex = newIndex;

        game.eventManager.pushDelayedEvent(UNNAMED, 0.0f, nullptr, [&, newPlayerPos]() {
            loadTilemap();
            player->moveTo(newPlayerPos.x, newPlayerPos.y);
            // check for companions
            for (const auto& sprite : game.sprites)
            {
                if (sprite->followsPlayer)
                    sprite->moveTo(player->position.x, player->position.y);
            }
            });
    }
}

void InGame::loadTilemap()
{
    tileMap = game.currentWorld->getCurrentTileMap();
    // remove static and dynamic (non-persistent) objects
    game.walls.clear();
    game.clearEmitters();
    game.clearSprites(); // sprites get flagged for deletion, but live until the end of the frame
    // check if there even is a valid tile map
    if (!tileMap)
        return;

    tilemapRenderer.loadTilemap(tileMap);

    game.setWorldBounds(
        tilemapRenderer.getWorldWidth() * tilemapRenderer.getTileSize(),
        tilemapRenderer.getWorldHeight() * tilemapRenderer.getTileSize()
    );

    Room* room = game.currentWorld->getCurrentRoom();
    room->visited = true;
    // the room state controls how objects are spawned
    // room states start with 1
    uint8_t currentState = room->state;
    auto& objectStates = room->objectStates;
    const auto& spriteData = game.loader.getSpriteData();
    size_t spritesLen = tileMap->getObjects().size();
    game.sprites.reserve(spritesLen);

    // build objects from map data
    for (const auto& obj : tileMap->getObjects())
    {
        processTileObject(game, obj, currentState, objectStates, spriteData);
    }
    // build objects that were placed in the data later on
    for (const auto& obj : tileMap->dynamicObjects)
    {
        processTileObject(game, obj, currentState, objectStates, spriteData);
    }

    // check if a different music track should be played
    const std::string musicKey = tileMap->getMusicKey();
    if (!musicKey.empty() && musicKey != currentMusicKey)
    {
        currentMusicKey = tileMap->getMusicKey();
        music = &const_cast<Music&>(game.loader.getMusic(currentMusicKey));
        PlayMusicStream(*music);
    }
    // check for NPCs that follow the player
    for (const auto& sprite : game.sprites)
    {
        if (sprite->followsPlayer)
            sprite->moveTo(player->position.x, player->position.y);
    }
    // TODO check if the player holds a weapon
    for (size_t wpnIdx = 0; wpnIdx < 2; wpnIdx++)
    {
        if (currentWeapon[wpnIdx].has_value() && currentWeapon[wpnIdx]->second == true)
            spawnWeapon(wpnIdx);
    }
}


void InGame::update(float deltaTime)
{
    // control the sprites and apply physics
    handleDeadSprites();
    // if a cutscene is active, it takes control over the player
    // otherwise, the player is controled by input
    game.cutsceneManager.update(deltaTime); // checks if a cutscene should be played
    handlePlayerInput(deltaTime);

    // update the sprites
    for (const auto& sprite : game.sprites)
    {
        if (sprite)
        {
            if (!game.cutsceneManager.isActive())
                sprite->executeBehavior(deltaTime);
            sprite->update(deltaTime);
        }
    }

    // light overlay
    // TODO make this more modular
    size_t currentLightIndex = 0;
    // draw a much bigger radius if the lamp is equipped
    // TODO: put these in the config
    //const float lightRadius = (lampIsOn) ? 180.0f : 24.0f;

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        lights[i].active = false;
    }

    Room* rm = game.currentWorld->getCurrentRoom();
    bool isDark = (rm && rm->dark);

    // animate always, regardless of cutscene
    for (const auto& sprite : game.sprites)
    {
        // progress the animation index and change the textures if necessary
        sprite->animate(deltaTime);
        // check if the sprite emits light in dark rooms
        // and give it a light cone
        if (isDark && sprite->emitsLight && currentLightIndex < MAX_LIGHTS)
        {
            lights[currentLightIndex].center = GetWorldToScreen2D(GetRectCenter(sprite->rect), cameraController.getCamera());
            lights[currentLightIndex].center.y += sprite->z; // apply jump height
            if (lampIsOn)
            {
                if (sprite == player)
                    lights[currentLightIndex].radius = 200.0f; // TODO get values from settings
                else
                    lights[currentLightIndex].radius = 0.0f;
            }
            else 
            {
                lights[currentLightIndex].radius = 24.0f;
            }
            lights[currentLightIndex].active = true;
            currentLightIndex++;
        }
    }
    for (const auto& sprite : game.sprites)
    {
        // collision of sprites with static objects (walls)
        // TODO: make this a method of Sprite?
        // 
        // resolve collision in the X direction
        sprite->rect.x = sprite->position.x;
        for (const auto& wall : game.walls)
        {
            resolveAxisX(sprite, wall->getRect());
        }
        for (const auto& other : game.sprites)
        {
            if (other != sprite && other->staticCollision)
                resolveAxisX(sprite, other->rect);
        }

        sprite->rect.y = sprite->position.y;
        for (const auto& wall : game.walls)
        {
            resolveAxisY(sprite, wall->getRect());
        }
        for (const auto& other : game.sprites)
        {
            if (other != sprite && other->staticCollision)
                resolveAxisY(sprite, other->rect);
        }

        // hurtbox centering midbottom
        sprite->hurtbox.x = sprite->rect.x + (sprite->rect.width - sprite->hurtbox.width) / 2 + sprite->hurtboxOffset.x;
        sprite->hurtbox.y = sprite->rect.y + (sprite->rect.height - sprite->hurtbox.height) + sprite->hurtboxOffset.y;

        // player damage
        if (sprite->canHurtPlayer && player->iFrameTimer < 0.001f && CheckCollisionRecs(sprite->hurtbox, player->rect))
        {
            if (sprite->damage < player->health)
                player->health -= sprite->damage;
            else
                player->health = 0;

            player->iFrameTimer = game.getSetting<float>("PlayeriFrames");
            applyKnockback(*sprite, *player, sprite->knockback);
            game.eventManager.pushEvent(SCREEN_SHAKE, std::make_tuple(0.2f, 4.0f, 0.0f));
            game.playSound("hurt1");
        }

        // damage enemies
        if (sprite->canHurtEnemies)
        {
            for (const auto& target : game.sprites)
            {
                if (target->isEnemy && target != sprite && target->iFrameTimer < 0.001f &&
                    target->health > 0 && CheckCollisionRecs(sprite->hurtbox, target->rect))
                {

                    target->health = (sprite->damage > target->health) ? 0 : target->health - sprite->damage;
                    target->iFrameTimer = 0.5f;

                    if (target->weight > 0)
                        applyKnockback(*sprite, *target, 8.0f / target->weight);

                    game.eventManager.pushEvent(SCREEN_SHAKE, std::make_tuple(0.2f, 4.0f, 0.0f));
                    game.playSound("creature_hurt_02");

                    // Mark projectile as done if it should disappear on hit
                    sprite->markForDeletion();
                    break;  // Projectile only hits one enemy
                }
            }
        }

        // weapon damage (TODO obsolete with projectile damage?)
        for (size_t wpnIdx = 0; wpnIdx < 2; wpnIdx++)
        {
            if (sprite->isEnemy && currentWeapon[wpnIdx].has_value())
            {
                Sprite* weapon = getSprite(currentWeapon[wpnIdx]->first);
                if (weapon && sprite->iFrameTimer < 0.001f && sprite->health > 0 &&
                    CheckCollisionRecs(weapon->hurtbox, sprite->rect))
                {
                    sprite->health = (weapon->damage > sprite->health) ? 0 : sprite->health - weapon->damage;
                    sprite->iFrameTimer = 0.5f;
                    
                    // calculate the knockback
                    if (sprite->weight > 0)
                        applyKnockback(*weapon, *sprite, 8.0f / sprite->weight); // TODO get knockback from weapon data?
                    
                    // screen shake
                    // TODO: get shake intensity from data
                    game.eventManager.pushEvent(SCREEN_SHAKE, std::make_tuple(0.2f, 4.0f, 0.0f));
                    game.playSound("creature_hurt_02");

                    // trigger particle effect
                    wpnHitEffect->position = GetRectCenter(sprite->rect);
                    wpnHitEffect->explode();
                }
            }
        }
    }

    // handle the particle effects
    for (auto& emitter : game.emitters)
    {
        emitter->update(deltaTime);
    }
    // clean up finished emitters
    game.emitters.erase(
        std::remove_if(game.emitters.begin(), game.emitters.end(),
            [](const std::shared_ptr<Emitter>& e) { return e->isDone(); }),
        game.emitters.end()
    );

    // check if the player is outside of the map bounds
    if (player->isColliding)
        checkRoomTransition();

    // update the camera controller to follow the player
    if (cameraHasBounds)
    {
        cameraController.setWorldBounds(
            tilemapRenderer.getWorldWidth() * tilemapRenderer.getTileSize(),
            tilemapRenderer.getWorldHeight() * tilemapRenderer.getTileSize()
        );
    }
    else
    {
        // only used in debugging
        cameraController.setWorldBounds(-1.0f, -1.0f);
    }
    cameraController.update(deltaTime);

    // player dies, GameOver scene starts
    if (player->health < 1)
    {
        game.pauseScene(getName());
        if (music) StopMusicStream(*music);
        game.stopScene("HUD");
        game.startScene("GameOver");
    }
}

void InGame::draw()
{
    ClearBackground(BLACK);
 
    // draw the textures that are affected by the camera
    BeginMode2D(cameraController.getCamera());

    // Draw the sprites after sorting them by their bottom y position, also respect the drawing layer of each sprite (fixed)
    // TODO add a flag to sprite that makes an exception from this sorting
    std::vector<Sprite*> drawOrder;
    drawOrder.reserve(game.sprites.size());
    for (const auto& sprite : game.sprites)
    {
        drawOrder.push_back(sprite.get());
    }
    std::sort(drawOrder.begin(), drawOrder.end(), [](Sprite* a, Sprite* b) {
        if (a->drawLayer != b->drawLayer)
            return a->drawLayer < b->drawLayer;
        return (a->rect.y + a->rect.height) < (b->rect.y + b->rect.height);
        });

    // draw each tilemap layer except the top one
    auto& cam = cameraController.getCamera();
    if (tileMap)
    {
        tilemapRenderer.drawLayer(0, cam); // floor
        // now draw the shadows
        for (Sprite* sprite : drawOrder)
        {
            sprite->drawShadow();
        }
        // TODO make this dynamic
        tilemapRenderer.drawLayer(1, cam); // walls
        tilemapRenderer.drawLayer(2, cam); // walls2
    }

    // draw the sprites
    for (Sprite* sprite : drawOrder)
    {
        sprite->draw();
        sprite->drawBehavior();
    }

    // draw particles
    for (auto& emitter : game.emitters)
    {
        emitter->draw();
    }

    // now draw the top layer above the sprites
    if (tileMap)
        tilemapRenderer.drawLayer(3, cam); // top

    // draw debug information that is affected by the camera (hitboxes etc)
    if (game.debug)
    {
        for (const auto& wall : game.walls)
        {
            DrawRectangleLines((int)wall->x, (int)wall->y, (int)wall->width, (int)wall->height, BLUE);
        }
        for (const auto& sprite : game.sprites)
        {
            DrawRectangleLines((int)sprite->rect.x, (int)sprite->rect.y, (int)sprite->rect.width, (int)sprite->rect.height, GREEN);
            DrawRectangleLines((int)sprite->hurtbox.x, (int)sprite->hurtbox.y, (int)sprite->hurtbox.width, (int)sprite->hurtbox.height, RED);
        }
    }
    EndMode2D();

    // draw lighting in dark rooms
    Room* r = game.currentWorld->getCurrentRoom();
    if (r && r->dark)
        DrawLightOverlay(game.target.texture, game.loader.getShader("light_mask_flicker"), lights, lightCount, static_cast<float>(game.gameScreenWidth), static_cast<float>(game.gameScreenHeight));

    // draw a vignette (only if player is in a dungeon)
    if (game.currentWorld->isDungeon)
        DrawVignette(game.target.texture, game.loader.getShader("vignette"), game.getSetting<float>("vignetteIntensity"), game.getSetting<float>("vignetteSmoothness"), static_cast<float>(game.gameScreenWidth), static_cast<float>(game.gameScreenHeight));

    // draw an effect when the player if low on health
    // TODO hardcoded values
    if (player->health < 5)
    {
        float modifier = (1.0f - static_cast<float>(player->health) / 4);
        float freq = 0.6f + modifier;  // gets faster with lower health
        float intensity = 0.2f + modifier;
        float softness = 0.5f;
        DrawLowHealthEffect(game.target.texture, game.loader.getShader("heartbeat"), freq, intensity, softness, static_cast<float>(game.gameScreenWidth), static_cast<float>(game.gameScreenHeight));
    }

    // cutscene stuff (textboxes etc) gets drawn relative to window position
    game.cutsceneManager.draw();
}

void InGame::end()
{
    takeScreenshot();

    // wait for a split second
    WaitTime(0.25);
    // stop the ingame music track
    if (music) 
        StopMusicStream(*music);
    music = nullptr;
}

void InGame::onPause()
{
    takeScreenshot();
}
