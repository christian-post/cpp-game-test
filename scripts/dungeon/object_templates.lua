-- scripts/tilemap/object_templates.lua
local ObjectTemplates = {}

-- deep copy helper
local function deep_copy(obj)
    if type(obj) ~= "table" then
        return obj
    end
    
    local copy = {}
    for k, v in pairs(obj) do
        copy[k] = deep_copy(v)
    end
    return copy
end

-- template definitions
local templates = {
    chest = {
        height = 16,
        id = 12,
        name = "chest",
        properties = {
            {
                name = "spriteName",
                type = "string",
                value = "chest"
            }
        },
        rotation = 0,
        type = "sprite",
        visible = true,
        width = 16,
        x = 128,
        y = 80
    },
    
    stairs = {
        height = 16,
        id = 56,
        name = "stairs",
        properties = {
            {
                name = "level",
                type = "int",
                value = 0
            },
            {
                name = "spriteName",
                type = "string",
                value = nil
            }
        },
        rotation = 0,
        type = "sprite",
        visible = true,
        width = 16,
        x = 208,
        y = 144
    },

    stairs = {
        height = 16,
        id = 56,
        name = "stairs",
        properties = {
            {
                name = "level",
                type = "int",
                value = 0  -- overwrite this
            },
            {
                name = "spriteName",
                type = "string",
                value = nil
            }
        },
        rotation = 0,
        type = "sprite",
        visible = true,
        width = 16,
        x = 208,
        y = 144
    },

    locked_door = {
        height = 32,
        id = 69,
        name = "door",
        properties = {
            {
                name = "castsShadow",
                type = "bool",
                value = false
            },
            {
                name = "locked",
                type = "bool",
                value = true
            },
            {
                name = "spriteName",
                type = "string",
                value = "locked_door"
            },
            {
                name = "direction",
                type = "int",
                value = 0 -- 0: right, 1: top, 2: left, 3: down
            }
        },
        rotation = 0,
        type = "sprite",
        visible = true,
        width = 32,
        x = 0,
        y = 0
    },

    enemy = {
        height = 16,
        id = 0,
        name = "enemy",
        properties = {
            {
                name = "roomState",
                type = "int",
                value = 1
            },
            {
                name = "spriteName",
                type = "string",
                value = "skelet"
            }
        },
        rotation = 0,
        type = "sprite",
        visible = true,
        width = 16,
        x = 104,
        y = 100
    },

    elf_companion_1 = {
        height = 16,
        id = 21,
        name = "npc",
        properties = {
            {
                name = "roomState",
                type = "int",
                value = 1
            },
            {
                name = "spriteName",
                type = "string",
                value = "elfCompanion1"
            }
        },
        rotation = 0,
        type = "sprite",
        visible = true,
        width = 16,
        x = 144,
        y = 96
    },

    elf_companion_2 = {
        height = 16,
        id = 24,
        name = "npc",
        properties = {
            {
                name = "roomState",
                type = "int",
                value = 2
            },
            {
                name = "spriteName",
                type = "string",
                value = "elfCompanion2"
            }
        },
        rotation = 0,
        type = "sprite",
        visible = true,
        width = 16,
        x = 112,
        y = 96
    },

    teleport = {
        height = 16,
        id = 78,
        name = "teleport",
        properties = {
            {
                name = "targetIndex",
                type = "int",
                value = 0
            },
            {
                name = "targetLevel",
                type = "int",
                value = 0
            },
            {
                name = "targetWorld",
                type = "string",
                value = "overworld"
            },
            {
                name = "targetX",
                type = "float",
                value = 456
            },
            {
                name = "targetY",
                type = "float",
                value = 244
            }
        },
        rotation = 0,
        type = "sprite",
        visible = true,
        width = 32,
        x = 112,
        y = 240
    }

}

-- generic factory function
function ObjectTemplates.create(object_type)
    local template = templates[object_type]
    if not template then
        error("Unknown object type: " .. tostring(object_type))
    end
    
    return deep_copy(template)
end

return ObjectTemplates