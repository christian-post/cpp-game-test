#include "Preload.h"
#include "Game.h"
#include "raylib.h"
#include "json.hpp"


void Preload::startup(Game* game) {
    // TODO: load asynchronous?

    // preload most of the assets that are persistent throughout the game
    // for an animated sprite, the keys have to contain the suffixes
    // _idle, _run, _hit, [...]
    game->loader.loadTextures({
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
            "npc_idle", {
                "./resources/textures/sprites/elf_f_idle_anim_f0.png",
                "./resources/textures/sprites/elf_f_idle_anim_f1.png",
                "./resources/textures/sprites/elf_f_idle_anim_f2.png",
                "./resources/textures/sprites/elf_f_idle_anim_f3.png"
            }
        },
        {
            "npc_run", {
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
            "hearts", {
                "./resources/textures/sprites/ui_heart_empty.png",
                "./resources/textures/sprites/ui_heart_half.png",
                "./resources/textures/sprites/ui_heart_full.png",
            }
        },
        {
            "weapon_sword", {
                "./resources/textures/sprites/weapon_regular_sword.png",
            }
        }
    });

    //game->loader.LoadTileset("./resources/textures/tilesets/tiles_test.png", 16);
    game->loader.LoadTilesetFromTiled("./resources/tilemaps/dungeon.tsj");

    // load the tile maps from text files
    game->loader.LoadTileMapFromTiled("./resources/tilemaps/test_dungeon.json");
    game->loader.LoadTileMapFromTiled("./resources/tilemaps/test_dungeon2.json");

    // load the font
    game->loader.LoadFont("./resources/fonts/slkscr.ttf");

    // advance to title after loading
    game->startScene("TitleScreen");
    game->stopScene("Preload");
}

void Preload::update(Game* game, float dt) {
    // TODO: calculate loading progress
}

void Preload::draw(Game* game) {
    // TODO: draw loading bar, then finish
    ClearBackground(BLACK);
}

void Preload::end() {
    // wait for a split second
    WaitTime(0.2f);
}
