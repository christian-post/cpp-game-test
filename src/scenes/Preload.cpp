#include "Preload.h"
#include "Game.h"
#include "raylib.h"
#include "json.hpp"


void Preload::startup() {
    // TODO: load asynchronous?

    // preload most of the assets that are persistent throughout the game

    // for an animated sprite, the keys have to contain the suffixes
    // _idle, _run, _hit, [...]
    game.loader.loadTextures({
        {
            "sprite_default_idle", {
                "./resources/textures/sprites/sprite_default_idle_anim_f0.png",
                "./resources/textures/sprites/sprite_default_idle_anim_f1.png"
            }
        },
        {
            "sprite_default_run", {
                "./resources/textures/sprites/sprite_default_idle_anim_f0.png",
                "./resources/textures/sprites/sprite_default_idle_anim_f1.png"
            }
        },
        {
            "player_idle", {
                "./resources/textures/sprites/knight_f_idle_anim_f0.png",
                "./resources/textures/sprites/knight_f_idle_anim_f1.png",
                "./resources/textures/sprites/knight_f_idle_anim_f2.png",
                "./resources/textures/sprites/knight_f_idle_anim_f3.png"
            }
        },
        { 
            "player_run", {
                "./resources/textures/sprites/knight_f_run_anim_f0.png",
                "./resources/textures/sprites/knight_f_run_anim_f1.png",
                "./resources/textures/sprites/knight_f_run_anim_f2.png",
                "./resources/textures/sprites/knight_f_run_anim_f3.png"
            }
        },
        {
            "player_hit", {
                "./resources/textures/sprites/knight_f_hit_anim_f0.png",
            }
        },
        {
            "elf_f_idle", {
                "./resources/textures/sprites/elf_f_idle_anim_f0.png",
                "./resources/textures/sprites/elf_f_idle_anim_f1.png",
                "./resources/textures/sprites/elf_f_idle_anim_f2.png",
                "./resources/textures/sprites/elf_f_idle_anim_f3.png"
            }
        },
        {
            "elf_f_run", {
                "./resources/textures/sprites/elf_f_run_anim_f0.png",
                "./resources/textures/sprites/elf_f_run_anim_f1.png",
                "./resources/textures/sprites/elf_f_run_anim_f2.png",
                "./resources/textures/sprites/elf_f_run_anim_f3.png"
            }
        },
        {
            "skelet_idle", {
                "./resources/textures/sprites/skelet_idle_anim_f0.png",
                "./resources/textures/sprites/skelet_idle_anim_f1.png",
                "./resources/textures/sprites/skelet_idle_anim_f2.png",
                "./resources/textures/sprites/skelet_idle_anim_f3.png",
            }
        },
        {
            "skelet_run", {
                "./resources/textures/sprites/skelet_run_anim_f0.png",
                "./resources/textures/sprites/skelet_run_anim_f1.png",
                "./resources/textures/sprites/skelet_run_anim_f2.png",
                "./resources/textures/sprites/skelet_run_anim_f3.png",
            }
        },
        {
            "big_demon_idle", {
                "./resources/textures/sprites/big_demon_idle_anim_f0.png",
                "./resources/textures/sprites/big_demon_idle_anim_f1.png",
                "./resources/textures/sprites/big_demon_idle_anim_f2.png",
                "./resources/textures/sprites/big_demon_idle_anim_f3.png",
            }
        },
        {
            "big_demon_run", {
                "./resources/textures/sprites/big_demon_run_anim_f0.png",
                "./resources/textures/sprites/big_demon_run_anim_f1.png",
                "./resources/textures/sprites/big_demon_run_anim_f2.png",
                "./resources/textures/sprites/big_demon_run_anim_f3.png",
            }
        },
        {
            "goblin_idle", {
                "./resources/textures/sprites/goblin_idle_anim_f0.png",
                "./resources/textures/sprites/goblin_idle_anim_f1.png",
                "./resources/textures/sprites/goblin_idle_anim_f2.png",
                "./resources/textures/sprites/goblin_idle_anim_f3.png",
            }
        },
        {
            "goblin_run", {
                "./resources/textures/sprites/goblin_run_anim_f0.png",
                "./resources/textures/sprites/goblin_run_anim_f1.png",
                "./resources/textures/sprites/goblin_run_anim_f2.png",
                "./resources/textures/sprites/goblin_run_anim_f3.png",
            }
        },
        // inventory sprites
        {
            "hearts", {
                "./resources/textures/sprites/ui_heart_empty.png",
                "./resources/textures/sprites/ui_heart_half.png",
                "./resources/textures/sprites/ui_heart_full.png",
            }
        },
        { "inventory_item_frame", { "./resources/textures/sprites/inventory_item_frame.png" }},
        { "inventory_cursor", { "./resources/textures/sprites/inventory_cursor.png" }},

        { "weapon_sword", { "./resources/textures/sprites/weapon_regular_sword.png" }},
        { "weapon_bow", { "./resources/textures/sprites/weapon_bow.png" }},        
        { "weapon_hammer", { "./resources/textures/sprites/weapon_hammer.png" }},
        { "weapon_baton_with_spikes", { "./resources/textures/sprites/weapon_baton_with_spikes.png" }},
        { "weapon_double_axe", { "./resources/textures/sprites/weapon_double_axe.png" }},
        { "weapon_mace", { "./resources/textures/sprites/weapon_mace.png" }},
        { "weapon_spear", { "./resources/textures/sprites/weapon_spear.png" }},
        { "weapon_arrow", { "./resources/textures/sprites/weapon_arrow.png" }},
        { "flask_big_red", { "./resources/textures/sprites/flask_big_red.png" }},
        { "itemDropHeart_idle", { "./resources/textures/sprites/item_drop_heart.png" } },
        { "itemDropCoin_idle", { "./resources/textures/sprites/item_drop_coin.png" } },
        {
            "dungeon_door_idle", {
                "./resources/textures/sprites/doors_leaf_closed.png",
                "./resources/textures/sprites/doors_leaf_open.png",
            }
        },
        // background images
        { "title_image", { "./resources/textures/images/title.png" }},
    });

    // load the tileset (the textures)
    game.loader.LoadTilesetFromTiled("./resources/tilemaps/test.tsj");
    game.loader.LoadTilesetFromTiled("./resources/tilemaps/dungeon.tsj");
    game.loader.LoadTilesetFromTiled("./resources/tilemaps/fields.tsj");
    // load the tile maps from text files
    game.loader.LoadTileMapFromTiled("./resources/tilemaps/test_map_small.json");
    game.loader.LoadTileMapFromTiled("./resources/tilemaps/test_map_big.json");
    game.loader.LoadTileMapFromTiled("./resources/tilemaps/test_dungeon.json");
    game.loader.LoadTileMapFromTiled("./resources/tilemaps/test_dungeon1.json");
    game.loader.LoadTileMapFromTiled("./resources/tilemaps/test_dungeon2.json");
    game.loader.LoadTileMapFromTiled("./resources/tilemaps/test_dungeon3.json");
    game.loader.LoadTileMapFromTiled("./resources/tilemaps/test_fields.json");
    // load the font
    game.loader.LoadFont("./resources/fonts/slkscr.ttf");
    // load shaders
    game.loader.LoadShaderFile("./resources/shaders/crumble.fs");
    // JSON data
    game.loader.loadSpriteData("./resources/enemies.json");
    game.loader.loadSpriteData("./resources/npcs.json");
    game.loader.loadSpriteData("./resources/weapons.json");
    game.loader.loadTextData("./resources/texts.json");
    // music and sfx
    // second argument is for adjusting the volume
    game.loader.LoadMusicFile("./resources/sound/music/Escape the Dungeon- Dubious Dungeon.mp3", 1.0f, "dungeon01");
    game.loader.LoadMusicFile("./resources/sound/music/Dungeon 02.ogg", 0.7f, "dungeon02");
    game.loader.LoadMusicFile("./resources/sound/music/title.wav", 1.0f);
    game.loader.LoadMusicFile("./resources/sound/music/Adventure.mp3", 1.0f, "field01");
    game.loader.LoadMusicFile("./resources/sound/music/Retro_No hope.mp3", 1.0f, "gameover");

    game.loader.LoadSoundFile("./resources/sound/sfx/slash.wav", 0.1f);
    game.loader.LoadSoundFile("./resources/sound/sfx/heart.wav", 0.6f);
    game.loader.LoadSoundFile("./resources/sound/sfx/rupee.wav", 0.8f);
    game.loader.LoadSoundFile("./resources/sound/sfx/doorOpen_2.ogg");
    game.loader.LoadSoundFile("./resources/sound/sfx/creature_hurt_02.ogg");
    game.loader.LoadSoundFile("./resources/sound/sfx/creature_die_01.ogg");
    game.loader.LoadSoundFile("./resources/sound/sfx/hit14.mp3", 0.5f, "hit01");
    game.loader.LoadSoundFile("./resources/sound/sfx/bookClose.ogg");
    game.loader.LoadSoundFile("./resources/sound/sfx/bookPlace1.ogg");
    game.loader.LoadSoundFile("./resources/sound/sfx/powerUp1.wav");
    game.loader.LoadSoundFile("./resources/sound/sfx/powerUp2.wav");
    game.loader.LoadSoundFile("./resources/sound/sfx/powerUp3.wav");
    game.loader.LoadSoundFile("./resources/sound/sfx/powerUp4.wav", 0.5f); // used for Textboxes
    game.loader.LoadSoundFile("./resources/sound/sfx/hurt1.wav"); 
    game.loader.LoadSoundFile("./resources/sound/sfx/gameover.wav"); 
    game.loader.LoadSoundFile("./resources/sound/sfx/menuOpen.wav"); 
    game.loader.LoadSoundFile("./resources/sound/sfx/menuClose.wav"); 
    game.loader.LoadSoundFile("./resources/sound/sfx/menuCursor.wav"); 
    game.loader.LoadSoundFile("./resources/sound/sfx/menuSelect.wav"); 

    // advance to title after loading
    game.startScene("TitleScreen");
    game.stopScene("Preload");
}

void Preload::update(float dt) {
    // TODO: calculate loading progress
}

void Preload::draw() {
    // TODO: draw loading bar, then finish
    ClearBackground(BLACK);
}

void Preload::end() {
    // wait for a split second
    WaitTime(0.25f);
}
