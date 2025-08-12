## Scenes




## Sprites
Sprites are used to represent basically every game object, aside from a few basic things like collision objects that don't need an update() or draw() method. They have a hitbox, which represents their position and size for collision, as well as an optional hurtbox that is used to afflict damage - this is used mainly for enemies and weapons. Sprites have an x, y and z coordinate, with z representing a "jumping" or "flying" state; currently only used for a jump animation. The sprite's movement is acceleration-based to make it appear smoother, and also to make it easier to incorporate other forces like knockback.
It is possible to instantiate a sprite completely in code, which is done for the player, but most other sprites are built using json data. See `npcs.json` and `enemies.json` for details. This data is utilized during tilemap creation.

## The Game World

### Tilemaps
I use the [Tiled](https://www.mapeditor.org/) editor to create all the rooms in the game. The data is in the .json format and located under `resources/tilemaps/`. The files with the .tsj expension are descriptions for a tileset and point to a PNG in `textures/tilesets`. Tilemap data contains info about the tile placement (split into multiple layers) and object placement (static collision and sprites). 

### Dungeons
Currently, the single dungeon accesible in game is hand-crafted, but the system would also allow procedural placement, which I will try out in the future. 
A dungeon is created like this:
```cpp
// define width and height of the dungeon (measured in rooms)
size_t roomsW = 4;
size_t roomsH = 4;
// create the dungeon and add some rooms
dungeon = std::make_unique<Dungeon>(game, roomsW, roomsH);
dungeon->insertRoom(3, 2, Room{ game.loader.getTilemap("dungeon001"), 0b1111 });
dungeon->insertRoom(2, 2, Room{ game.loader.getTilemap("dungeon002"), 0b0011 });
dungeon->insertRoom(2, 1, Room{ game.loader.getTilemap("dungeon003"), 0b1001 });

currentDungeon->setStartingRoomIndex(14); // start in R1
currentDungeon->makeMinimapTextures(); // creates downscaled versions of the rooms
```
`insertRoom` takes three arguments: row and column where the room is positioned, and a Room object, which itself needs a TileMap object (those are preloaded and stored in the AssetLoader instance) and an uint8_t that defines which of the four sides have entrances. Here I represent them as bitmasks to make it easier to read. They are currently only used to draw the mini map, but in the future they will make procedural dungeon construction easier.
`setStartingRoomIndex()` manually sets the room in which the game starts. This index is flattened, because Rooms are stored in a 1D-vector for performance reasons, and calculated as `index = row * roomsW + col`. By default, the startingRoomIndex is 0.
`makeMinimapTextures()` creates scaled-down versions of all rooms for the minimap in the InGame menu. It draws the tiles from the TileMap data to a texture, which is then scaled down to fit a 36x24 texture. In addition, I implemented a mode filter to make the mini map images look smoother. This is only done once during this routine, so it shouldn't be too performance-heavy.

### Sprite creation
The game world gets populated in the `InGame::loadTilemap()` method, which loads the data that the current room index points to. This is called at the start of the game, as well as whenever the player goes into a different room. Right now, the rooms are pretty small, so it's fine to load them as needed.
The routine starts with clearing out all walls and sprites (except for persistent ones like the player and npcs that follow him). Everything after this is loaded from the data defined in Tiled. At the moment, this method is pretty messy, and I might have to create a few subroutines to clear it up, as well as reduce the number of edge cases.

One thing that currently seems overkill, is that I split the tilemap textures into chunks, so that I can draw only the parts of the map that the player is currently seeing in-camera. Right now the rooms are barely even scrolling, but I plan to add an overworld and larger maps, so this will hopefully be worth it. If you turn on the debug view you can see the edges of the tilemap chunks marked in red if you change the value of "tileChunkSize" in settings.json to a smaller value. 



## Modifying the game
My plan is to have as little hard coded data in the cpp files as possible. I'm utilizing JSON since it's a format that I'm most familiar with, although it's probably not the best in terms of performance and type safety when combined with C++. When (or if) the game is finished someday, I might turn all this data back into a binary format so that it's not so tempting for players to just change any object in the game and break it this way.  

### npcs.json and enemies.json
These files contain blueprints for all Sprite objects. I split these into two files just to make it easier to find/add data, but during loading these get combined into one json object. The data has a hierarchical structure, defined by the "inherits" field. At the top is `sprite_default`, which each other sprite at least has to inherit from. Sprites are able to overwrite all fields, and everything else undefined is taken from the default sprite.
The Tiled data can also contain sprite data that is formatted similarly, and if a sprite in a tilemap file has data, the game will prioritize that data, but only for that particular instance. Whereas the data in the npcs.json and enemies.json is valid for all sprites with the corresponding `"spriteName"`.

### particles.json
Just like the sprite data, this file is used to provide a blueprint for the particle system that makes it easy to add a new kind of particle to the game. For what the values do, see `Particle.h` and `Emitter.h`. Currently I am not using this yet, so the few particles in game are hard-coded.

### weapons.json
This file contains data about the weapons that the player character can use. They control how the weapon is displayed, its hurtbox and damage, as well as a "type" field that modifies how the weapon acts. 0 is the standard swinging weapon, like a sword. 1 is a poking animation, 2 is shooting (currently unused), and 3 is a swinging/whacking-like animation. 

### texts.json
Here I store all the displayed texts for the game. They are being used in the `TextBox` class. As far as formatting goes, this can currently handle line breaks (`\n`) as well as the form feed (`\f`) to make the text continue on the next page. You can add dialogue to a sprite by adding the corresponding key to npcs.json in the "behaviorData.dialogue" field. The fields in texts.json contain lists of strings. If list for that key contains multiple strings, the NPC will use the first string once, and the second one after that indefinitely.

### settings.json
The file `resources/settings.json` contains a few settings that can be changed to alter the game. In the end, most of these fields should be modifiable from within the game, but right now you need to go into the file itself. Some of the fields are only used by me so that I can tweak game variables more quickly, without the need to go into the code. The key bindings at the bottom don't do anything yet, they are still hard-coded.

### item data
Items are a bit of a mess right now. They are added to the game in ItemData.cpp, but also given an optional usage callback in InventoryManager.cpp. So to add a functional item it's necessary to modify data in two places. Weapons are also a subset of ItemData, but they already have their data in weapons.json as well as a modular callback that works regardless of what's in the json file. Though this won't scale well with more complex, unique items, like a grappling hook or bombs.

## the TODO list

For refactoring and bugs, see todo.txt

- Pathfinding: sprites that follow a target currently get stuck on walls. A simple A* algorithm that uses the tiles as nodes isn't hard to implement.
- Procedural Dungeon generation: I have a few ideas for a graph-based approach that would randomize the available rooms based on their entrances and if they contain loot.
- Sprite behavior is a bit messy, especially since there is no state machine for the Behaviors yet. This is important especially for the enemy A.I., they are kinda dumb right now.
- NPC dialogue (textboxes in general) need a way to select different answers.
- The elf NPC needs a few spells so that she can participate in combat

