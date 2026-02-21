function execute()

    hideHUD()
    letterbox(getScreenWidth(), getScreenHeight(), 5.0)
    wait(1.0)

    local trigger_id = roomID .. "_enemies_defeated"
    triggerEvent(trigger_id)

    wait(1.0)

    letterbox(getScreenWidth(), getScreenHeight(), 5.0, true)
    showHUD()
    advanceRoomState()

end