#include "Preload.h"
#include "Game.h"
#include "json.hpp"


void Preload::startup()
{
    auto& l = game.loader;
    // > preload most of the assets that are persistent throughout the game.
    // > for an animated sprite, the keys have to contain the suffixes
    // _idle, _run, _hit, [...]

    game.loader.loadQueue.emplace("Loading textures", [&]() {
        l.loadTextures({
            {
                "sprite_default", {
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
                "skelet_charge", {
                    "./resources/textures/sprites/skelet_idle_anim_f3.png"
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
                "big_demon_charge", {
                    "./resources/textures/sprites/big_demon_idle_anim_f3.png",
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
                "chest", {
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
                "dwarf_f_idle", {
                    "./resources/textures/sprites/dwarf_f_idle_anim_f0.png",
                    "./resources/textures/sprites/dwarf_f_idle_anim_f1.png",
                    "./resources/textures/sprites/dwarf_f_idle_anim_f2.png",
                    "./resources/textures/sprites/dwarf_f_idle_anim_f3.png",
                }
            },
            {
                "dwarf_f_run", {
                    "./resources/textures/sprites/dwarf_f_run_anim_f0.png",
                    "./resources/textures/sprites/dwarf_f_run_anim_f1.png",
                    "./resources/textures/sprites/dwarf_f_run_anim_f2.png",
                    "./resources/textures/sprites/dwarf_f_run_anim_f3.png",
                }
            },
            {
                "wall_fountain_basin", {
                    "./resources/textures/sprites/wall_fountain_basin_red_anim_f0.png",
                    "./resources/textures/sprites/wall_fountain_basin_red_anim_f1.png",
                    "./resources/textures/sprites/wall_fountain_basin_red_anim_f2.png",
                }
            },
            {
                "wall_fountain_mid", {
                    "./resources/textures/sprites/wall_fountain_mid_red_anim_f0.png",
                    "./resources/textures/sprites/wall_fountain_mid_red_anim_f1.png",
                    "./resources/textures/sprites/wall_fountain_mid_red_anim_f2.png",
                }
            },
            {
                "turret_idle", {
                    "./resources/textures/sprites/turret_right_f0.png",
                    "./resources/textures/sprites/turret_right_f1.png",
                }
            },
            { "signpost", { "./resources/textures/sprites/signpost.png" }},
            { "blob", { "./resources/textures/sprites/blob.png" }},
            { "ladder_up", { "./resources/textures/sprites/ladder_up.png" }},
            { "ladder_down", { "./resources/textures/sprites/ladder_down.png" }},

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
            { "flask_big_green", { "./resources/textures/sprites/flask_big_green.png" }},
            { "flask_big_blue", { "./resources/textures/sprites/flask_big_blue.png" }},
            { "itemDropHeart", { "./resources/textures/sprites/item_drop_heart.png" } },
            { "itemDropCoin", { "./resources/textures/sprites/item_drop_coin.png" } },
            { "heart_1up", { "./resources/textures/sprites/item_drop_heart1up.png" } },
            { "bomb", { "./resources/textures/sprites/bomb_f0.png" } },
            { "item_key", { "./resources/textures/sprites/item_key.png" } },
            { "item_boss_key", { "./resources/textures/sprites/item_boss_key.png" } },
            //{ "item_lamp", { "./resources/textures/sprites/item_lamp_new_0.png" } },
            { "knight_map_mini", { "./resources/textures/sprites/knight_map_mini.png" }},
            // background images
            { "title_image", { "./resources/textures/images/title.png" }},
            });
            // spritesheets
            l.loadSpritesheet("./resources/textures/sprites/projectiles.png", 8, 8, "magic_ball");
            l.loadSpritesheet("./resources/textures/sprites/fireball_16x4.png", 16, 16, "fireball");
            l.loadSpritesheet("./resources/textures/sprites/smoke_16x6.png", 16, 16, "smoke");
            l.loadSpritesheet("./resources/textures/sprites/xbox_buttons_sorted.png", 16, 16, "xbox_buttons_sorted");
            l.loadSpritesheet("./resources/textures/sprites/lantern-Sheet.png", 16, 16, "item_lamp");
            l.loadSpritesheet("./resources/textures/sprites/simple_dungeon_doors_locked.png", 32, 32, "doors_locked");
            l.loadSpritesheet("./resources/textures/sprites/simple_dungeon_doors_closed.png", 32, 32, "doors_closed");
            l.loadSpritesheet("./resources/textures/sprites/boss_doors_locked.png", 32, 32, "boss_doors_locked");
            l.loadSpritesheet("./resources/textures/sprites/normal_dude_1_idle.png", 16, 24, "normal_dude_1_idle");
            l.loadSpritesheet("./resources/textures/sprites/normal_dude_1_run.png", 16, 24, "normal_dude_1_run");
            l.loadSpritesheet("./resources/textures/sprites/normal_dude_2_idle.png", 16, 24, "normal_dude_2_idle");
            l.loadSpritesheet("./resources/textures/sprites/normal_dude_2_run.png", 16, 24, "normal_dude_2_run");
        });

    // load the tileset (the textures)
    game.loader.loadQueue.emplace("Loading tilesets", [&]() {
        l.LoadtilesetFromTiled("./resources/tilemaps/test.tsj");
        l.LoadtilesetFromTiled("./resources/tilemaps/simple_grassland.tsj");
        l.LoadtilesetFromTiled("./resources/tilemaps/simple_dungeon.tsj");
        l.LoadtilesetFromTiled("./resources/tilemaps/simple_interior.tsj");

        // load minified versions for the minimap
        // key has to be [tileset_image_file]_mini
        l.loadTextures({ {"simple_dungeon_tiles_mini", { "./resources/textures/tilesets/simple_dungeon_tiles_mini.png"} } });
        l.loadTextures({ {"simple_grassland_tiles_mini", { "./resources/textures/tilesets/simple_grassland_tiles_mini.png"} } });
        });

    // load the tile maps from text files
    game.loader.loadQueue.emplace("Loading tilemaps", [&]() {
        l.loadTilemapsFromDirectory("./resources/tilemaps");
        l.loadTilemapsFromDirectory("./resources/tilemaps/generated/lua_dungeon"); // TODO load the subfolders into a hash table with the dungeon name as key
        });
    // load the font
    game.loader.loadQueue.emplace("Loading fonts", [&]() {
        l.LoadFont("./resources/fonts/slkscr.ttf");
        });
    // load shaders
    game.loader.loadQueue.emplace("Loading shaders", [&]() {
        l.LoadShadersFromDirectory("./resources/shaders");
        });
    // JSON data
    game.loader.loadQueue.emplace("Loading JSON data", [&]() {
        l.loadSpriteData("./resources/enemies.json");
        l.loadSpriteData("./resources/npcs.json");
        l.loadSpriteData("./resources/weapons.json");
        l.loadtextData("./resources/texts.json");
        l.loadParticleData("./resources/particles.json");
        l.loadDungeonData("./resources/dungeons.json");
        });
    // music and sfx
    // second argument is for adjusting the volume
    game.loader.loadQueue.emplace("Loading music", [&]() {
        l.LoadMusicFile("./resources/sound/music/Escape the Dungeon- Dubious Dungeon.mp3", 1.0f, "dungeon01");
        l.LoadMusicFile("./resources/sound/music/Dungeon 02.ogg", 0.7f, "dungeon02");
        l.LoadMusicFile("./resources/sound/music/title.wav", 1.0f);
        l.LoadMusicFile("./resources/sound/music/Adventure.mp3", 1.0f, "field01");
        l.LoadMusicFile("./resources/sound/music/Retro_No hope.mp3", 1.0f, "gameover");
        });
    game.loader.loadQueue.emplace("Loading sound files", [&]() {
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
        l.LoadSoundFile("./resources/sound/sfx/spring.wav");
        l.LoadSoundFile("./resources/sound/sfx/magic1.wav");
        });

    game.loader.loadQueue.emplace("Loading thumbnails", [&]() {
        l.LoadSavegameThumbnails("./savegames/thumbs");
        });

    totalLoadSteps = game.loader.loadQueue.size();
}

void Preload::update(float deltaTime)
{
    // loading progress
    if (!game.loader.loadQueue.empty()) 
    {
        currentMessage = game.loader.loadQueue.front().first;
        game.loader.loadQueue.front().second(); // callback
        game.loader.loadQueue.pop();
    }
    else
    {
        currentMessage = "Loading finished";
        game.stopScene("Preload");
        game.startScene("TitleScreen");
    }
}

void Preload::draw()
{
    ClearBackground(BLACK);

    int fontSize = 10;
    int textWidth = MeasureText(currentMessage.c_str(), fontSize);
    int x = (game.gameScreenWidth - textWidth) / 2;
    int y = (game.gameScreenHeight - fontSize) / 2 + 16;
    DrawText(currentMessage.c_str(), x, y, fontSize, WHITE);

    float progress = 1.0f - (float)game.loader.loadQueue.size() / totalLoadSteps;
    int rectX = int(game.gameScreenWidth * 0.2);
    int rectY = int(game.gameScreenHeight * 0.4);
    int rectW = int(game.gameScreenWidth * 0.6);
    int rectH = 16;
    DrawRectangleLines(rectX, rectY, rectW, rectH, WHITE);
    DrawRectangle(rectX, rectY, static_cast<int>(static_cast<float>(rectW) * progress), rectH, WHITE);
}

void Preload::end()
{
    game.loader.postprocessSpriteData(); // for the JSON sprite data

    // process item data
    game.inventory.initialize();

    // wait a split second, just in case
    WaitTime(0.25f);
}
