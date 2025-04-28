#include "Game.h"

int main() {
    //SetTraceLogLevel(LOG_WARNING);
    SetTraceLogLevel(LOG_INFO);

    Game game;
    game.run();
    return 0;
}
