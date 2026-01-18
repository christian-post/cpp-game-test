#include "MapUI.h"
#include "Game.h"
#include "Controls.h"
#include "Utils.h"

MapUI::MapUI(Game& game, const std::string& name) : Scene(game, name)
{
}

void MapUI::startup()
{
    x = float(game.gameScreenWidth); // start the map screen on the right
    topY = game.getSetting<float>("HudHeight");
    y = topY;
    // recalculate size values
    width = game.gameScreenWidth;
    height = game.gameScreenHeight - game.getSetting<uint32_t>("HudHeight");
    // set the sliding speed so that it takes [slideDuration] seconds to expand the inventory
    slideDuration = game.getSetting<float>("menuSlidingDuration");
    speed = height / slideDuration;
    state = SLIDING_LEFT;

    currentLevel = game.currentWorld->currentLevel;  // Changed
}

void MapUI::update(float deltaTime)
{
    blinkTimer += deltaTime;
    if (blinkTimer >= blinkSpeed)
    {
        cursorOn = !cursorOn;
        blinkTimer -= blinkSpeed;
    }
    // state machine
    switch (state)
    {
    case OPENING:
        if (y > topY)
        {
            y = std::max(topY, y - deltaTime * speed);
        }
        else
        {
            state = OPENED;
        }
        break;
    case CLOSING:
        if (y < game.gameScreenHeight)
        {
            y = std::min(static_cast<float>(game.gameScreenHeight), y + deltaTime * speed);
        }
        else
        {
            game.eventManager.pushEvent(INVENTORY_DONE);
            game.stopScene("MapUI");
        }
        break;
    case SLIDING_LEFT:
        if (x > 0.0f)
        {
            x = std::max(0.0f, x - deltaTime * speed);
        }
        else
        {
            state = OPENED;
            blinkTimer = 0.0f;
        }
        break;
    case SLIDING_RIGHT:
        if (x < static_cast<float>(game.gameScreenWidth))
        {
            x = std::min(static_cast<float>(game.gameScreenWidth), x + deltaTime * speed);
        }
        else
        {
            state = NONE;
            game.pauseScene("MapUI");
        }
        break;
    case OPENED:
        if (game.buttonsPressed & CONTROL_CONFIRM)
        {
            state = CLOSING;
            game.playSound("menuClose");
        }
        if (game.buttonsPressed & CONTROL_ACTIONL)
        {
            state = SLIDING_RIGHT;
            game.playSound("menuOpen");
            game.resumeScene("InventoryUI");
        }
        if (game.buttonsPressed & CONTROL_UP)
        {
            currentLevel = (currentLevel + 1) % game.currentWorld->getNumLevels();  // Changed
        }
        if (game.buttonsPressed & CONTROL_DOWN)
        {
            currentLevel = (currentLevel - 1) % game.currentWorld->getNumLevels();  // Changed
        }
        break;
    case NONE:
    default:
        break;
    }
}

void MapUI::draw()
{
    DrawRectangle(int(x), int(y), int(width), int(height), DARKBURGUNDY);

    // draw the level indicators (bottom to top)
    size_t lvlRecW = offsetX - 8;
    size_t lvlRecH = 18;
    size_t lvlRecX = static_cast<size_t>(x) + 12;
    size_t numLevels = game.currentWorld->getNumLevels();  // Changed
    size_t lvlSpacing = 12;
    size_t totalHeight = numLevels * lvlRecH + (numLevels - 1) * lvlSpacing;
    size_t startY = static_cast<size_t>(y) + (static_cast<size_t>(height) - totalHeight) / 2;
    for (size_t lvl = 0; lvl < numLevels; lvl++)
    {
        size_t lvlRecY = startY + (numLevels - 1 - lvl) * (lvlRecH + lvlSpacing);
        Rectangle r = { (float)lvlRecX, (float)lvlRecY, (float)lvlRecW, (float)lvlRecH };
        DrawRectangleRounded(r, 0.2f, 4, LIGHTBURGUNDY);

        // text on rect
        static char lvlText[32];
        snprintf(lvlText, sizeof(lvlText), "Lvl %d", (int)lvl);

        int fontSize = 10;
        int textWidth = MeasureText(lvlText, fontSize);
        int textX = lvlRecX + (lvlRecW - textWidth) / 2;
        int textY = lvlRecY + (lvlRecH - fontSize) / 2;

        DrawText(lvlText, textX, textY, fontSize, LIGHTGRAY);

        // selection indicator with arrows
        if (lvl == currentLevel)
        {
            DrawRectangleRoundedLines(r, 0.2f, 4, LIGHTGRAY);

            float centerX = lvlRecX + lvlRecW / 2.0f;
            float arrowSize = 4.0f;

            if (lvl < numLevels - 1)
            {
                Vector2 v1 = { centerX, (float)lvlRecY - 8 };              // top point
                Vector2 v2 = { centerX - arrowSize, (float)lvlRecY - 3 };  // bottom left
                Vector2 v3 = { centerX + arrowSize, (float)lvlRecY - 3 };  // bottom right
                DrawTriangle(v1, v2, v3, LIGHTGRAY);
            }
            if (lvl > 0)
            {
                Vector2 v1 = { centerX, (float)lvlRecY + (float)lvlRecH + 8 };          // bottom point
                Vector2 v2 = { centerX - arrowSize, (float)lvlRecY + (float)lvlRecH + 3 }; // top left
                Vector2 v3 = { centerX + arrowSize, (float)lvlRecY + (float)lvlRecH + 3 }; // top right
                DrawTriangle(v1, v3, v2, LIGHTGRAY);
            }
        }
    }

    // TODO let the World handle some of the params
    MapRenderParams p;
    p.x = x;
    p.y = y;
    p.width = float(width);
    p.height = float(height);
    p.displayLevel = currentLevel;
    p.showCursor = cursorOn && state == OPENED;
    p.border = border;
    p.spacing = spacing;
    p.offsetX = offsetX;
    p.offsetY = offsetY;

    game.currentWorld->renderMapScreen(p);

    // help texts
    if (state == OPENED)
    {
        int fontSize = 6;

        static char helpText[32];
        snprintf(helpText, sizeof(helpText), "Level %d", (int)currentLevel);

        const char* textLeft = nullptr;
        if (WasGamepadUsedLast())
        {
            textLeft = "<<";

            // show the button texture
            int button = game.getGamepadButtonForControl(CONTROL_ACTIONL);
            const auto& buttonTex = game.loader.getTextures("xbox_buttons_sorted")[button];
            uint32_t buttonX = int(x) + 16;
            uint32_t buttonY = int(y) + int(game.gameScreenHeight - topY) - 20;
            DrawTexture(buttonTex, buttonX, buttonY, WHITE);
        }
        else
        {
            textLeft = "<< N";
        }
        uint32_t helpTextY = int(y) + int(game.gameScreenHeight - topY) - fontSize - 8;
        uint32_t txtL = int(x) + 4;
        DrawText(textLeft, txtL, helpTextY, fontSize, LIGHTGRAY);
        uint32_t helpTextX = int(x) + (int(game.gameScreenWidth) - MeasureText(helpText, fontSize)) / 2;
        DrawText(helpText, helpTextX, helpTextY, fontSize, LIGHTGRAY);
    }
}

void MapUI::end()
{
    game.eventManager.pushEvent(SET_MUSIC_VOLUME, 1.0f);
}