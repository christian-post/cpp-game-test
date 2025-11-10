#include "Game.h"
#include "TestWorldGraph.h"

//#define RUN_TESTS

int main() {
    //SetTraceLogLevel(LOG_WARNING);
    SetTraceLogLevel(LOG_INFO);

    // optional tests that don't require the game loop
    // if all tests pass, the game will start
#ifdef RUN_TESTS
    if (runWorldGraphTest())
        return 1;
#endif

    // Game loop
    while (true) {
        Game game;
        game.run();
        if (!game.isRestartRequested()) 
            break;
    }
    return 0;
}
