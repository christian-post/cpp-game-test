#pragma once

#include <sol.hpp>

class Game;
class InGame;

void bindCutsceneCommands(sol::state& lua, Game& game, InGame& inGame);
void bindGameQueries(sol::state& lua, Game& game, InGame& inGame);
void bindSpriteOperations(sol::state& lua, Game& game, InGame& inGame);