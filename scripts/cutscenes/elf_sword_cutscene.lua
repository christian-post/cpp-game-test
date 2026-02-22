function execute()
    if not spriteExists("elfCompanion2") then
        return
    end

    hideHUD()
    letterbox(getScreenWidth(), getScreenHeight(), 1.0)
    wait(0.5)
    moveSpriteTo("player", 124, 64, 1.0)
    moveSpriteTo("elfCompanion2", 142, 64, 1.5)
    wait(0.5)
    showTextbox("elfCutscene1", "powerUp4")
    letterbox(getScreenWidth(), getScreenHeight(), 0.5, true)
    showHUD()
    advanceRoomState()

    onCutsceneComplete(function()
        
        setSpriteProperty("elfCompanion2", "persistent", true)
        setSpriteProperty("elfCompanion2", "followsPlayer", true)
        setSpriteProperty("elfCompanion2", "speed", getSetting("npcFollowSpeed"))
        
        addSpriteBehavior("elfCompanion2", "ChaseBehavior", {
            target = "player",
            distance = 20.0
        })
    end)

end