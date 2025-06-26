#include "Preload.h"
#include "Game.h"
#include "raylib.h"
#include "json.hpp"


void Preload::startup() {
    auto& l = game.loader;
    // TODO: load asynchronous?

    // preload most of the assets that are persistent throughout the game

    // for an animated sprite, the keys have to contain the suffixes
    // _idle, _run, _hit, [...]
    loadQueue.emplace("Loading textures", [&]() {
        l.loadTextures({
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
            {
                "chest_idle", {
                    "./resources/textures/sprites/chest_empty_open_anim_f0.png",
                    "./resources/textures/sprites/chest_empty_open_anim_f1.png",
                    "./resources/textures/sprites/chest_empty_open_anim_f2.png"
                }
            },
            {
                "dwarf_m_idle", {
                    "./resources/textures/sprites/dwarf_m_idle_anim_f0.png",
                    "./resources/textures/sprites/dwarf_m_idle_anim_f1.png",
                    "./resources/textures/sprites/dwarf_m_idle_anim_f2.png",
                    "./resources/textures/sprites/dwarf_m_idle_anim_f3.png",
                }
            },
            {
                "wall_fountain_basin_idle", {
                    "./resources/textures/sprites/wall_fountain_basin_red_anim_f0.png",
                    "./resources/textures/sprites/wall_fountain_basin_red_anim_f1.png",
                    "./resources/textures/sprites/wall_fountain_basin_red_anim_f2.png",
                }
            },
            {
                "wall_fountain_mid_idle", {
                    "./resources/textures/sprites/wall_fountain_mid_red_anim_f0.png",
                    "./resources/textures/sprites/wall_fountain_mid_red_anim_f1.png",
                    "./resources/textures/sprites/wall_fountain_mid_red_anim_f2.png",
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
            { "flask_big_red_idle", { "./resources/textures/sprites/flask_big_red.png" }},
            { "flask_big_green_idle", { "./resources/textures/sprites/flask_big_green.png" }},
            { "flask_big_blue_idle", { "./resources/textures/sprites/flask_big_blue.png" }},
            { "itemDropHeart_idle", { "./resources/textures/sprites/item_drop_heart.png" } },
            { "itemDropCoin_idle", { "./resources/textures/sprites/item_drop_coin.png" } },
            { "bomb_idle", { "./resources/textures/sprites/bomb_f0.png" } },
            { "item_key", { "./resources/textures/sprites/item_key.png" } },
            {
                "dungeon_door_idle", {
                    "./resources/textures/sprites/doors_leaf_closed.png",
                    "./resources/textures/sprites/doors_leaf_open.png",
                    "./resources/textures/sprites/doors_leaf_locked.png",
                }
            },
            { "knight_map_mini", { "./resources/textures/sprites/knight_map_mini.png" }},
            // background images
            { "title_image", { "./resources/textures/images/title.png" }},
            });
            // spritesheets
            l.loadSpritesheet("./resources/textures/sprites/projectiles.png", 8, 8, "magic_ball_idle");
            l.loadSpritesheet("./resources/textures/sprites/projectiles.png", 8, 8, "magic_ball_run"); // TODO temporary fix
            l.loadSpritesheet("./resources/textures/sprites/fireball_16x4.png", 16, 16, "fireball_run");
            l.loadSpritesheet("./resources/textures/sprites/smoke_16x6.png", 16, 16, "smoke_idle");
            l.loadSpritesheet("./resources/textures/sprites/xbox_buttons_16x16.png", 16, 16, "xbox_buttons");
        });

    // load the tileset (the textures)
    loadQueue.emplace("Loading tilesets", [&]() {
        l.LoadtilesetFromTiled("./resources/tilemaps/test.tsj");
        l.LoadtilesetFromTiled("./resources/tilemaps/dungeon.tsj");
        l.LoadtilesetFromTiled("./resources/tilemaps/fields.tsj");
        });
    // load the tile maps from text files
    loadQueue.emplace("Loading tilemaps", [&]() {
        l.LoadtileMapFromTiled("./resources/tilemaps/dungeon_shop.json");
        l.LoadtileMapFromTiled("./resources/tilemaps/test_map_small.json");
        l.LoadtileMapFromTiled("./resources/tilemaps/test_map_big.json");
        l.LoadtileMapFromTiled("./resources/tilemaps/test_dungeon.json");
        l.LoadtileMapFromTiled("./resources/tilemaps/test_dungeon1.json");
        l.LoadtileMapFromTiled("./resources/tilemaps/test_dungeon2.json");
        l.LoadtileMapFromTiled("./resources/tilemaps/test_dungeon3.json");
        l.LoadtileMapFromTiled("./resources/tilemaps/test_fields.json");
        l.LoadtileMapFromTiled("./resources/tilemaps/dungeon001.json");
        l.LoadtileMapFromTiled("./resources/tilemaps/dungeon002.json");
        l.LoadtileMapFromTiled("./resources/tilemaps/dungeon003.json");
        l.LoadtileMapFromTiled("./resources/tilemaps/dungeon004.json");
        l.LoadtileMapFromTiled("./resources/tilemaps/dungeon005.json");
        });
    // load the font
    loadQueue.emplace("Loading fonts", [&]() {
        l.LoadFont("./resources/fonts/slkscr.ttf");
        });
    // load shaders
    loadQueue.emplace("Loading shaders", [&]() {
        l.LoadShaderFile("./resources/shaders/crumble.fs");
        });
    // JSON data
    loadQueue.emplace("Loading JSON data", [&]() {
        l.loadSpriteData("./resources/enemies.json");
        l.loadSpriteData("./resources/npcs.json");
        l.loadSpriteData("./resources/weapons.json");
        l.loadtextData("./resources/texts.json");
        });
    // music and sfx
    // second argument is for adjusting the volume
    loadQueue.emplace("Loading music", [&]() {
        l.LoadMusicFile("./resources/sound/music/Escape the Dungeon- Dubious Dungeon.mp3", 1.0f, "dungeon01");
        l.LoadMusicFile("./resources/sound/music/Dungeon 02.ogg", 0.7f, "dungeon02");
        l.LoadMusicFile("./resources/sound/music/title.wav", 1.0f);
        l.LoadMusicFile("./resources/sound/music/Adventure.mp3", 1.0f, "field01");
        l.LoadMusicFile("./resources/sound/music/Retro_No hope.mp3", 1.0f, "gameover");
        });
    loadQueue.emplace("Loading sound files", [&]() {
        l.LoadSoundFile("./resources/sound/sfx/slash.wav", 0.1f);
        l.LoadSoundFile("./resources/sound/sfx/heart.wav", 0.6f);
        l.LoadSoundFile("./resources/sound/sfx/rupee.wav", 0.8f);
        l.LoadSoundFile("./resources/sound/sfx/cash.wav");
        l.LoadSoundFile("./resources/sound/sfx/doorOpen_2.ogg");
        l.LoadSoundFile("./resources/sound/sfx/creature_hurt_02.ogg");
        l.LoadSoundFile("./resources/sound/sfx/creature_die_01.ogg");
        l.LoadSoundFile("./resources/sound/sfx/hit14.mp3", 0.5f, "hit01");
        l.LoadSoundFile("./resources/sound/sfx/bookClose.ogg");
        l.LoadSoundFile("./resources/sound/sfx/bookPlace1.ogg");
        l.LoadSoundFile("./resources/sound/sfx/powerUp1.wav");
        l.LoadSoundFile("./resources/sound/sfx/powerUp2.wav");
        l.LoadSoundFile("./resources/sound/sfx/powerUp3.wav");
        l.LoadSoundFile("./resources/sound/sfx/powerUp4.wav", 0.5f);
        l.LoadSoundFile("./resources/sound/sfx/powerUp5.wav"); 
        l.LoadSoundFile("./resources/sound/sfx/powerUp6.wav");
        l.LoadSoundFile("./resources/sound/sfx/hurt1.wav");
        l.LoadSoundFile("./resources/sound/sfx/gameover.wav");
        l.LoadSoundFile("./resources/sound/sfx/menuOpen.wav");
        l.LoadSoundFile("./resources/sound/sfx/menuClose.wav");
        l.LoadSoundFile("./resources/sound/sfx/menuCursor.wav", 0.5f);
        l.LoadSoundFile("./resources/sound/sfx/menuSelect.wav");
        l.LoadSoundFile("./resources/sound/sfx/heal.wav");
        l.LoadSoundFile("./resources/sound/sfx/hammer.wav");
        l.LoadSoundFile("./resources/sound/sfx/fireball.wav", 0.5f);
        l.LoadSoundFile("./resources/sound/sfx/Rise02.wav");
        l.LoadSoundFile("./resources/sound/sfx/Rise03.wav");
        l.LoadSoundFile("./resources/sound/sfx/tone.wav");
        });

    totalLoadSteps = loadQueue.size();
}

void Preload::update(float deltaTime) {
    // calculate loading progress
    if (!loadQueue.empty()) {
        currentMessage = loadQueue.front().first;
        loadQueue.front().second(); // callback
        loadQueue.pop();
    }
    else {
        currentMessage = "Loading finished";
        game.startScene("TitleScreen");
        game.stopScene("Preload");
    }
}

void Preload::draw() {
    ClearBackground(BLACK);
    int fontSize = 10;
    int textWidth = MeasureText(currentMessage.c_str(), fontSize);
    int x = (game.gameScreenWidth - textWidth) / 2;
    int y = (game.gameScreenHeight - fontSize) / 2 + 16;
    DrawText(currentMessage.c_str(), x, y, fontSize, WHITE);
    float progress = 1.0f - (float)loadQueue.size() / totalLoadSteps;
    int rectX = int(game.gameScreenWidth * 0.2);
    int rectY = int(game.gameScreenHeight * 0.4);
    int rectW = int(game.gameScreenWidth * 0.6);
    int rectH = 16;
    DrawRectangleLines(rectX, rectY, rectW, rectH, WHITE);
    DrawRectangle(rectX, rectY, static_cast<int>(static_cast<float>(rectW) * progress), rectH, WHITE);
}

void Preload::end() {
    // wait for a split second
    WaitTime(0.25f);
}
