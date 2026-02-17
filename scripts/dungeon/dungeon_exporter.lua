-- exports generated dungeon to JSON format matching the required structure

local DungeonExporter = {}

local function get_node_at(layout, level, row, col)
    -- reverse lookup: find node name at given position
    for name, pos in pairs(layout.positions) do
        if pos.level == level and pos.row == row and pos.col == col then
            return name
        end
    end
    return nil
end

function DungeonExporter.export(layout, edges, graph, item_pool, stairway_rooms, DungeonGenerator)
    -- converts dungeon to JSON structure
    -- returns table ready for json encoding
    
    -- find starting room position
    local start_pos = layout.positions["Start"]
    if not start_pos then
        error("Start position not found!")
    end
    
    -- find boss room position
    local boss_pos = layout.positions["BossRoom"]
    if not boss_pos then
        error("BossRoom position not found!")
    end
    
    -- calculate dungeon dimensions
    local max_row = 0
    local max_col = 0
    for _, pos in pairs(layout.positions) do
        if pos.row > max_row then
            max_row = pos.row
        end
        if pos.col > max_col then
            max_col = pos.col
        end
    end
    
    local dungeon_data = {
        display_name = "Clifftop Fortress", -- TODO define this elsewhere
        seed = dungeon_seed,
        starting_level = start_pos.level,
        starting_room = {start_pos.row, start_pos.col},
        boss_level = boss_pos.level,
        boss_room = {boss_pos.row, boss_pos.col},
        level_connections = {},
        overworld_entrances = {
            { room = {0, 0}, level = 0 } -- where this dungeon is in the overworld, TODO make dynamic
        },
        item_pool = {},
        levels = {},
        rooms_w = max_col + 1,
        rooms_h = max_row + 1,
        num_levels = layout.levels
    }
    
    -- copy item pool
    for _, item in ipairs(item_pool) do
        table.insert(dungeon_data.item_pool, item)
    end
    
    -- build level connections (edges between levels)
    for _, edge in ipairs(edges) do
        local from_pos = layout.positions[edge.from]
        local to_pos = layout.positions[edge.to]
        
        if from_pos and to_pos and from_pos.level ~= to_pos.level then
            local connection = {
                from = from_pos.level,
                to = to_pos.level,
                room = {from_pos.row, from_pos.col}
            }
            
            if #edge.requirements > 0 then
                connection.required_items = {}
                for _, req in ipairs(edge.requirements) do
                    table.insert(connection.required_items, req)
                end
            end
            
            table.insert(dungeon_data.level_connections, connection)
        end
    end
    
    -- build levels data
    for level = 0, layout.levels - 1 do
        local level_data = {
            rooms = {},
            edges = {},
            item_nodes = {}
        }
        
        -- collect all rooms for this level
        for row = 0, layout.rows - 1 do
            for col = 0, layout.cols - 1 do
                local node_name = get_node_at(layout, level, row, col)
                if node_name then
                    local doors = DungeonGenerator.calculate_doors(layout, edges, level, row, col)
            
                    local room_data = {
                        row = row,
                        column = col,
                        tilemap = "room_" .. doors,
                        doors = doors
                    }
            
                    -- add item placement info (excluding special rooms)
                    if node_name ~= "Start" and node_name ~= "BossRoom" and not stairway_rooms[node_name] then
                        local node = graph.nodes[node_name]
                        if node and node.value then
                            room_data.item = node.value
                            table.insert(level_data.item_nodes, {row, col})
                        end
                    end
            
                    table.insert(level_data.rooms, room_data)
                end
            end
        end
        
        -- collect edges within this level that have requirements
        for _, edge in ipairs(edges) do
            local from_pos = layout.positions[edge.from]
            local to_pos = layout.positions[edge.to]
            
            if from_pos and to_pos and from_pos.level == level and to_pos.level == level and #edge.requirements > 0 then
                local edge_data = {
                    from = {from_pos.row, from_pos.col},
                    to = {to_pos.row, to_pos.col},
                    required_items = {}
                }
                
                for _, req in ipairs(edge.requirements) do
                    table.insert(edge_data.required_items, req)
                end
                
                table.insert(level_data.edges, edge_data)
            end
        end
        
        table.insert(dungeon_data.levels, level_data)
    end
    
    return dungeon_data
end

function DungeonExporter.save_to_file(dungeon_data, filepath)
    -- saves this dungeon's data to a JSON file
    -- note: json.encode is provided by C++ (LuaDungeonGenerator)
    
    local encoded = json.encode(dungeon_data, 2)
    
    local file = io.open(filepath, "w")
    if not file then
        error("Could not open file for writing: " .. filepath)
    end
    
    file:write(encoded)
    file:close()
    
    print(string.format("Dungeon saved to %s", filepath))
end

function DungeonExporter.append_to_dungeons(dungeon_data, filepath, dungeon_name)
    -- appends dungeon to dungeons.json object
    -- creates file with empty object if it doesn't exist
    -- note: json.encode and json.decode are provided by C++ (LuaDungeonGenerator)
    
    dungeon_name = dungeon_name or ("dungeon_" .. os.date("!%Y%m%d_%H%M%S"))
    
    local dungeons = {}
    
    -- try to read existing file
    local file = io.open(filepath, "r")
    if file then
        local content = file:read("*all")
        file:close()
        
        if content and content ~= "" then
            local decoded = json.decode(content)
            if decoded and type(decoded) == "table" then
                dungeons = decoded
            end
        end
    end
    
    -- add dungeon with key name (overwrites completely if exists)
    dungeons[dungeon_name] = dungeon_data
    
    -- write back to file
    local encoded = json.encode(dungeons, 2)
    
    file = io.open(filepath, "w")
    if not file then
        error("Could not open file for writing: " .. filepath)
    end
    
    file:write(encoded)
    file:close()
    
    -- count total dungeons
    local count = 0
    for _ in pairs(dungeons) do
        count = count + 1
    end
    
    print(string.format("Dungeon '%s' saved to %s (total: %d)", dungeon_name, filepath, count))
end

return DungeonExporter