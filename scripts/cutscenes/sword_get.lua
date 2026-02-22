function execute()

    -- player got the sword from the dungeon chest, which advances the state of the room where the elf NPC is
    -- TODO this might not always be the starting room, but this would get modified by the dungeon generation script via a context object
    idx = getStartingRoomIndex()
    advanceRoomState(0, idx)

    print("TEST TEST TEST")

end