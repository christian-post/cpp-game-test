#include "Game.h"

int main() {
    //SetTraceLogLevel(LOG_WARNING);
    SetTraceLogLevel(LOG_INFO);

    while (true) {
        Game game;
        game.run();
        if (!game.isRestartRequested()) 
            break;
    }
    return 0;
}
