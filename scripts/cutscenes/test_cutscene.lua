function execute()

    hideHUD()
    letterbox(getScreenWidth(), getScreenHeight(), 2.0)
    wait(1.0)

    local trigger_id = "some_event"
    triggerEvent(trigger_id)

    wait(1.0)

    letterbox(getScreenWidth(), getScreenHeight(), 2.0, true)
    showHUD()
    advanceRoomState()

end