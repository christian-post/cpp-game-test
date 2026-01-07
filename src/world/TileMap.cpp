#include "TileMap.h"
#include "Game.h"
#include "TeleportBehavior.h"
#include "StairsBehavior.h"
#include "TradeItemBehavior.h"
#include "CollectItemBehavior.h"
#include "OpenLockBehavior.h"
#include "ChestBehavior.h"

Level::Level(size_t roomsW, size_t roomsH) : roomsW{ roomsW }, roomsH{ roomsH }
{
    rooms.resize(roomsW * roomsH);
}

std::vector<std::optional<Room>>& Level::getRooms()
{
    return rooms;
}

Room* Level::getRoomAt(size_t index)
{
    if (index >= rooms.size())
        return nullptr;

    return rooms[index].has_value() ? &*rooms[index] : nullptr;
}

void Level::insertRoom(size_t index, Room&& room)
{
    rooms[index] = std::move(room);
}

TileLayer::TileLayer(const nlohmann::json& layerJson)
{
    name = layerJson["name"];
    width = layerJson["width"];
    height = layerJson["height"];
    visible = layerJson["visible"];

    // Get the flat 1D array from JSON
    std::vector<int> flatData = layerJson["data"].get<std::vector<int>>();

    // Resize the 2D vector to fit width x height
    data.resize(height, std::vector<int>(width));

    // Convert 1D array into 2D
    for (int y = 0; y < height; ++y) 
    {
        for (int x = 0; x < width; ++x) 
        {
            data[y][x] = flatData[y * width + x];
        }
    }

    // Handle properties
    if (layerJson.contains("properties"))
    {
        for (const auto& prop : layerJson["properties"])
        {
            properties[prop["name"]] = prop["value"].dump();
        }
    }
}

TileMap::TileMap(const nlohmann::json& jsonMap, std::string mapName) 
    : mapName(mapName)
{
    width = jsonMap["width"];
    height = jsonMap["height"];
    tileWidth = jsonMap["tilewidth"];
    tileHeight = jsonMap["tileheight"];

    music = "";

    if (jsonMap.contains("properties") && jsonMap["properties"].is_array())
    {
        for (const auto& prop : jsonMap["properties"])
        {
            if (prop.contains("name") && prop.contains("value"))
            {
                // overrides the dungeon music
                if (prop["name"] == "music")
                    music = prop["value"].get<std::string>();
                else if (prop["name"] == "dark")
                    dark = prop["value"].get<bool>();
                else if (prop["name"] == "roomID")
                    roomID = prop["value"].get<std::string>();
            }
        }
    }

    for (auto& t : jsonMap["tilesets"])
    {
        // strip the ".tsj" from the filename
        std::string srcName = t["source"];
        tilesetNames.push_back(std::make_pair(srcName.substr(0, srcName.size() - 4), t["firstgid"])); 
    }
    //std::string srcName = jsonMap["tilesets"][0]["source"];
    //tilesetName = srcName.substr(0, srcName.size() - 4); // strip the ".tsj"

    if (jsonMap.contains("layers"))
    {
        for (const auto& layer : jsonMap["layers"])
        {
            if (layer["type"] == "tilelayer")
            {
                layers.emplace_back(layer);
            }
            else if (layer["type"] == "objectgroup")
            {
                for (const auto& obj : layer["objects"])
                {
                    objects.emplace_back(obj);
                }
            }
        }
    }
}

const TileLayer& TileMap::getLayer(size_t index) const
{
    if (index >= layers.size()) 
        throw std::out_of_range("Layer index out of bounds");
    return layers[index];
}


void processTileObject(Game& game, const TileObject& obj, uint8_t currentState, std::unordered_map<uint32_t, ObjectState>& objectStates, const nlohmann::json& spriteData)
{
    if (!obj.visible)
        return;

    uint8_t objectState = obj.properties.value("roomState", 0);
    if (objectState != 0 && (objectState & currentState) == 0)
        return;

    TraceLog(LOG_INFO, "creating %s - <%s>, id: %d, objectState: %d",
        obj.type.c_str(),
        obj.name.empty() ? "unnamed" : obj.name.c_str(),
        obj.id, objectState
    );

    if (objectState != 0 && (objectState & currentState) == 0)
        // object does not spawn in the currentState
        return;
    // object type-specific code
    if (obj.type == "wall")
    {
        int layer = obj.properties.value("layer", 0);
        game.walls.push_back(std::make_unique<CollisionObject>(
            CollisionObject{ layer, obj.x, obj.y, obj.width, obj.height })
        );
    }
    else if (obj.type == "sprite")
    {
        if (objectStates[obj.id].isDefeated)
            // this sprite is dead, skip it
            return;
        std::string spriteName = obj.properties.value("spriteName", "sprite_default");
        // get the data for this sprite from the JSON
        const auto& data = spriteData.contains(spriteName)
            ? spriteData.at(spriteName)
            : spriteData.at("sprite_default");
        if (!spriteData.contains(spriteName))
            TraceLog(LOG_WARNING, "Missing sprite data for %s, falling back to sprite_default", spriteName.c_str());
        // store default data separately to replace individual attributes
        const auto& defaultData = spriteData.at("sprite_default");
        std::vector<std::string> textureKeys;
        if (data.contains("textures") && data.at("textures").is_array())
        {
            for (const auto& item : data.at("textures"))
            {
                if (item.is_null())
                    textureKeys.push_back(""); // reads null as empty string (no animation frames for this state)
                else
                    textureKeys.push_back(item.get<std::string>());
            }
        }
        else
        {
            textureKeys = defaultData.at("textures").get<std::vector<std::string>>();
        }

        // get the hitbox dimensions for the constructor
        // if not specified in the JSON data, it takes the dimensions from the Tiled object data
        Vector2 hitbox = data.contains("hitbox") ?
            Vector2{ data.at("hitbox")[0].get<float>(), data.at("hitbox")[1].get<float>() } :
            Vector2{ obj.width, obj.height };
        // instanciate the sprite
        auto sprite = std::make_shared<Sprite>(game, obj.x, obj.y, hitbox.x, hitbox.y, obj.name);
        // generic attributes
        // from JSON data
        sprite->health = data.contains("health") ? data.at("health").get<int>() : defaultData.at("health").get<int>();
        sprite->damage = data.contains("damage") ? data.at("damage").get<int>() : defaultData.at("damage").get<int>();
        sprite->speed = data.contains("speed") ? data.at("speed").get<float>() : defaultData.at("speed").get<float>();
        sprite->knockback = data.contains("knockback") ? data.at("knockback").get<float>() : defaultData.at("knockback").get<float>();
        sprite->weight = data.contains("weight") ? data.at("weight").get<float>() : defaultData.at("weight").get<int>();
        sprite->hitboxOffset = data.contains("hitboxOffset") ?
            Vector2{ data.at("hitboxOffset")[0].get<float>(), data.at("hitboxOffset")[1].get<float>() } :
            Vector2{ 0.0f, 0.0f };
        sprite->emitsLight = data.contains("emitsLight") ? data.at("emitsLight").get<bool>() : false;
        // attributes from Tiled data (instance-specific, overwrite JSON data)
        sprite->spriteName = spriteName;
        sprite->speed = obj.properties.value("speed", sprite->speed);
        sprite->damage = obj.properties.value("damage", sprite->damage);
        sprite->knockback = obj.properties.value("knockback", sprite->knockback);
        sprite->tileMapID = obj.id;
        sprite->drawLayer = obj.properties.value("drawLayer", 0);
        sprite->emitsLight = obj.properties.value("emitsLight", false);
        sprite->castsShadow = obj.properties.value("castsShadow", true);
        float hurtboxW = obj.properties.value("hurtboxW", 0.0f);
        float hurtboxH = obj.properties.value("hurtboxH", 0.0f);
        if (hurtboxW != 0.0f && hurtboxH != 0.0f)
            sprite->setHurtbox(-1.0f, -1.0f, hurtboxW, hurtboxH);

        // collision
        if (data.contains("collides"))
            sprite->isColliding = static_cast<bool>(data.at("collides").get<int>()); //TODO shouldn't this also be a bool in the data?

        // specific sprite attributes
        // TODO: for persistent sprites, check if they exist in the spriteMap
        if (obj.name == "teleport")
        {
            // when touched, changes the current index
            // TODO currently unused
            sprite->isColliding = false;
            sprite->visible = false;
            std::string targetMap = obj.properties.value("targetMap", "");
            float targetX = obj.properties.value("targetPosX", 0.0f);
            float targetY = obj.properties.value("targetPosY", 0.0f);
            sprite->addBehavior(std::make_unique<TeleportBehavior>(game, sprite, game.spriteMap["player"], targetMap, Vector2{ targetX, targetY }));
        }
        else if (obj.name == "stairs")
        {
            // when touched, changes the dungeon level number
            sprite->isColliding = false;
            sprite->castsShadow = false;
            sprite->setTextures(textureKeys);
            int level = obj.properties.value("level", 0);
            sprite->addBehavior(std::make_unique<StairsBehavior>(game, sprite, game.spriteMap["player"], level));
        }
        else if (obj.name == "npc")
        {
            if (!game.spriteMap[spriteName])
                // TODO: handle this differently, this might create empty references
                game.spriteMap[spriteName] = sprite;
            sprite->setTextures(textureKeys);
        }
        else if (obj.name == "tradeItem")
        {
            sprite->setTextures(std::vector<std::string>{ spriteName });
            sprite->doesAnimate = false;
            uint32_t cost = obj.properties.value("cost", 999);
            std::string name = obj.properties.value("name", "error"); // TODO switch spriteName and Name
            sprite->addBehavior(std::make_unique<TradeItemBehavior>(game, sprite, game.spriteMap["player"], name, cost));
        }
        else if (obj.name == "enemy")
        {
            sprite->canHurtPlayer = true;
            sprite->isEnemy = true;
            sprite->setTextures(textureKeys);
            // spawn the item drops if the enemy is defeated
            if (data.contains("itemDrops"))
            {
                std::weak_ptr<Sprite> weakSprite = sprite;
                std::string eventName = "killSprite_" + std::to_string(reinterpret_cast<uintptr_t>(sprite.get()));
                int eventKey = EventKeyRegistry::getEventKey(eventName);
                game.eventManager.addListener(eventKey, [&, weakSprite, data](std::any) {
                    auto s = weakSprite.lock();
                    if (!s)
                        return;
                    float rand = static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f;
                    float accum = 0.0f;
                    for (const auto& drop : data["itemDrops"])
                    {
                        std::string itemId = drop.at(0);
                        float chance = drop.at(1);
                        accum += chance;
                        if (rand < accum) {
                            auto item = std::make_shared<Sprite>(
                                game, s->position.x, s->position.y, 12.0f, 12.0f, itemId
                            );
                            auto& itemData = game.inventory.getItemData();
                            auto it = itemData.find(itemId);
                            if (it != itemData.end())
                            {
                                const ItemData& data = it->second;
                                item->setTextures(std::vector<std::string>{ data.textureKey });
                            }
                            else
                            {
                                item->setTextures(std::vector<std::string>{ "sprite_default" }); // missing item data
                            }
                            item->drawLayer = 1;
                            item->doesAnimate = false;
                            item->isColliding = false;
                            item->addBehavior(std::make_unique<CollectItemBehavior>(game, item, game.spriteMap["player"], itemId, 1));
                            game.sprites.emplace_back(item);
                            break;
                        }
                    }
                    });
            }
        }
        else if (obj.name == "door")
        {
            sprite->spriteName = spriteName;
            sprite->setTextures(textureKeys);
            sprite->doesAnimate = false;
            // TODO: set the open state in Tiled Data
            uint8_t openState = obj.properties.value("openState", 0);

            std::string eventStr = obj.properties.value("event", "");
            int eventKey = EventKeyRegistry::getEventKey(eventStr);

            bool isAlreadyOpen = false; // check if this door already opened from another event
            for (auto& ev : game.eventManager.peekEvents())
            {
                if (ev.first == eventKey)
                    isAlreadyOpen = true;
            }

            if (currentState < openState && !objectStates[obj.id].isOpened && !isAlreadyOpen)
            {
                sprite->staticCollision = true;
                bool locked = obj.properties.value("locked", false);
                if (locked)
                {
                    sprite->currentFrame = 2;
                    sprite->addBehavior(std::make_unique<OpenLockBehavior>(game, sprite, game.spriteMap["player"], eventKey));
                }

                // external door trigger
                game.eventManager.addListener(eventKey, [&, sprite = sprite.get()](std::any) {
                    objectStates[obj.id].isOpened = true;
                    sprite->currentFrame = 1;
                    sprite->staticCollision = false;
                    // TODO open the door in the adjacent room
                    });
            }
            else
            {
                sprite->currentFrame = 1;
                sprite->staticCollision = false;
                objectStates[obj.id].isOpened = true;
            }

        }
        else if (obj.name == "hurt")
        {
            // invisible sprite with hurtbox (e.g. floor spikes)
            sprite->canHurtPlayer = true;
            sprite->visible = false;
            sprite->isColliding = false;
        }
        else if (obj.name == "chest")
        {
            sprite->doesAnimate = false;
            sprite->staticCollision = true;
            sprite->setTextures({ spriteName });

            // initialize objectStates from Tiled data if not already set
            if (objectStates[obj.id].itemName.empty())
            {
                objectStates[obj.id].itemName = obj.properties.value("item", "");
                objectStates[obj.id].itemAmount = obj.properties.value("amount", 0);
            }

            if (objectStates[obj.id].isOpened)
            {
                sprite->currentFrame = 2;
            }
            else
            {
                std::string eventStr = "chest_opened_" + std::to_string(obj.id);
                int eventKey = EventKeyRegistry::getEventKey(eventStr);
                game.eventManager.removeListeners(eventKey);
                game.eventManager.addListener(eventKey, [&](std::any data) {
                    uint32_t eventId = std::any_cast<uint32_t>(data);
                    if (eventId == obj.id)
                        objectStates[obj.id].isOpened = true;
                    });
                sprite->addBehavior(std::make_unique<ChestBehavior>(game, sprite, game.spriteMap["player"], objectStates[obj.id].itemName, objectStates[obj.id].itemAmount));
            }
        }
        // add an event that changes the isDefeated field for this sprite
        std::string eventStr = "defeated_" + std::to_string(obj.id);
        int eventKey = EventKeyRegistry::getEventKey(eventStr);
        game.eventManager.removeListeners(eventKey);
        game.eventManager.addListener(eventKey, [&](std::any data) {
            uint32_t eventId = std::any_cast<uint32_t>(data);
            auto& currentRoomObjectStates = game.currentWorld->getCurrentRoom()->objectStates;
            if (eventId == obj.id)
                currentRoomObjectStates[obj.id].isDefeated = true;
            });

        // Check if sprite has a state machine definition
        if (data.contains("stateMachine"))
        {
            auto stateMachine = StateMachine::createFromJSON(
                game, sprite, data["stateMachine"]);
            sprite->setStateMachine(std::move(stateMachine));
        }
        else if (data.contains("behaviors"))
        {
            // Use old behavior system as fallback
            addBehaviorsToSprite(game, sprite,
                data.at("behaviors"),
                data.at("behaviorData"));
        }

        game.sprites.emplace_back(sprite);
    }
}