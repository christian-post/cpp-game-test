#include "Game.h"

int main() 
{
    //SetTraceLogLevel(LOG_WARNING);
    SetTraceLogLevel(LOG_INFO);

    // Game loop
    while (true)
    {
        Game game;
        game.run();
        if (!game.isRestartRequested()) 
            break;
    }
    return 0;
}
