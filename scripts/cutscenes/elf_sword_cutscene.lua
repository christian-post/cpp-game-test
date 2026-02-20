function execute()
    if not spriteExists("elfCompanion2") then
        return
    end
    
    local tileSize = getTileSize()
    local npcX = 12.0 * tileSize
    local npcY = 8.0 * tileSize
    
    hideHUD()
    letterbox(getScreenWidth(), getScreenHeight(), 1.0)
    wait(1.0)
    moveSpriteTo("elfCompanion2", npcX, npcY, 2.0)
    wait(0.5)
    showTextbox("elfCutscene1", "powerUp4")
    
    onCutsceneComplete(function()
        showHUD()
        advanceRoomState()
        
        setSpriteProperty("elfCompanion2", "persistent", true)
        setSpriteProperty("elfCompanion2", "followsPlayer", true)
        setSpriteProperty("elfCompanion2", "speed", getSetting("npcFollowSpeed"))
        
        addSpriteBehavior("elfCompanion2", "ChaseBehavior", {
            target = "player",
            distance = 20.0
        })
    end)
end