local utils = require("lib.utils")
local WorldUtils = require("lib.world_utils")
local TilemapUtils = require("lib.tilemap_utils")
local OW_Metatiles = require("overworld.ow_metatiles")
local TilemapModifier = {} -- export

-- town tilemaps (assumes one large town (2 rooms) and a small one (1 room))
local zone_content = {
    town = {
        north = "resources/tilemaps/town_n.json",
        south = "resources/tilemaps/town_s.json",
        single = "resources/tilemaps/town_single.json",
    }
}

-- pool of interior rooms assignable to town teleports.
-- each entry: { key, index, x, y } where x/y are the spawn coords inside the interior room.
local room_pool = {
    --{ world_key = "interior", index = 0, x = 158, y = 172 }, -- test room
    { world_key = "interior", index = 1, x = 158, y = 172 },
    { world_key = "interior", index = 2, x = 158, y = 172 },
    { world_key = "interior", index = 3, x = 158, y = 172 },
    { world_key = "interior", index = 4, x = 158, y = 172 },
    { world_key = "interior", index = 5, x = 158, y = 172 },
    { world_key = "interior", index = 6, x = 158, y = 172 },
    { world_key = "interior", index = 7, x = 110, y = 172 },
    { world_key = "interior", index = 8, x = 158, y = 172 },
    { world_key = "interior", index = 9, x = 158, y = 172 },
    -- TODO add more rooms, the dungeons etc
}

-- transition features: stamp a metatile onto the "to" room at the edge facing the "from" cell.
-- keys maps orientation (w/e/n/s = side of the to-room that borders the from-cell) to a metatile key.
local transition_features = {
    {
        from = "field",
        to = "lake",
        requires = "boat",
        keys = { w = "lake_landing_bridge_w", e = "lake_landing_bridge_e", n = "lake_landing_bridge_n", s = "lake_landing_bridge_s" },
    },
}

-- geometry per orientation: axis is perpendicular to the bordering edge (the metatile insets along it),
-- side picks the near/far end of that axis, inset is the gap in tiles between edge and metatile.
-- TODO insets are tuned to the bridge art; move them into the feature entry once a second feature needs its own
local feature_orientations = {
    w = { axis = "x", side = "near", inset = 4 },
    e = { axis = "x", side = "far", inset = 4 },
    n = { axis = "y", side = "near", inset = 3 },
    s = { axis = "y", side = "far", inset = 3 },
}

local function orientation_from_dir(dir_row, dir_col)
    -- direction from the to-room toward the from-cell -> which edge the feature sits on
    if dir_col == -1 then
        return "w"
    elseif dir_col == 1 then
        return "e"
    elseif dir_row == -1 then
        return "n"
    elseif dir_row == 1 then
        return "s"
    end
    return nil
end


local function pick_from_pool(pool)
    -- picks a random entry from a list
    if #pool == 0 then
        return nil
    end
    local i = dungeon_random(#pool)
    local entry = pool[i]
    table.remove(pool, i)
    return entry
end

local function build_edge_set(edges)
    -- initialize the lookup table of edges (room connections)
    local set = {}
    for _, edge in ipairs(edges) do
        set[edge.from .. "->" .. edge.to] = true
    end
    return set
end

local function are_connected(edge_set, from_row, from_col, to_row, to_col)
    -- check if this edge is in the edge_set
    local key = WorldUtils.node_name(from_row, from_col) .. "->" .. WorldUtils.node_name(to_row, to_col)
    return edge_set[key] == true
end

-- find all connected clusters of cells sharing the same zone type.
-- two cells are in the same cluster if they are adjacent AND connected by an edge.
-- returns a list of clusters, each a list of {row, col} sorted by row then col.
local function find_zone_clusters(zone_grid, edge_set, grid_width, grid_height)
    local visited = {}
    local clusters = {}

    local directions = {
        -- direction_row, direction_column
        { dr = -1, dc = 0 },
        { dr = 1, dc = 0 },
        { dr = 0, dc = -1 },
        { dr = 0, dc = 1 },
    }

    for row = 0, grid_height - 1 do
        for col = 0, grid_width - 1 do
            if not (visited[row] and visited[row][col]) then
                local zone = zone_grid[row][col]
                if not zone_content[zone] then
                    goto continue
                end

                -- BFS to collect all connected same-zone cells
                local cluster = {}
                local queue = { {row = row, col = col} }
                local qi = 1

                while qi <= #queue do
                    local cell = queue[qi]
                    qi = qi + 1

                    if visited[cell.row] and visited[cell.row][cell.col] then
                        goto continue_bfs
                    end

                    if not visited[cell.row] then
                        visited[cell.row] = {}
                    end
                    visited[cell.row][cell.col] = true
                    table.insert(cluster, cell)

                    if not (zone == "town" and #cluster >= 2) then
                        for _, dir in ipairs(directions) do
                            local nr = cell.row + dir.dr
                            local nc = cell.col + dir.dc
                            if nr >= 0 and nr < grid_height and nc >= 0 and nc < grid_width then
                                if zone_grid[nr][nc] == zone and are_connected(edge_set, cell.row, cell.col, nr, nc) then
                                    if not (visited[nr] and visited[nr][nc]) then
                                        table.insert(queue, {row = nr, col = nc})
                                    end
                                end
                            end
                        end
                    end

                    ::continue_bfs::
                end

                table.sort(cluster, function(a, b)
                    if a.row ~= b.row then
                        return a.row < b.row
                    end
                    return a.col < b.col
                end)

                table.insert(clusters, { zone = zone, cells = cluster })

                ::continue::
            end
        end
    end

    return clusters
end

-- assign positional roles to cells in a town cluster.
-- assumes exactly 2 vertically stacked cells.
local function assign_town_roles(cluster)
    if #cluster.cells == 1 then
        local cell = cluster.cells[1]
        local roles = {}
        roles[cell.row] = { [cell.col] = "single" }
        return roles
    end

    if #cluster.cells ~= 2 then
        error(string.format("tilemap_modifier: town cluster has %d cells, expected 1 or 2", #cluster.cells))
    end

    local roles = {}
    roles[cluster.cells[1].row] = { [cluster.cells[1].col] = "north" }
    roles[cluster.cells[2].row] = { [cluster.cells[2].col] = "south" }
    return roles
end

local role_assigners = {
    town = assign_town_roles,
}

-- patch teleport objects in a town dst_map with rooms from the pool.
-- appends to assignments: { room, teleport_x, teleport_y } for each assigned teleport,
-- where teleport_x/y is the overworld return position (16px below the teleport rect).
local function patch_ow_teleports(dst_map, cell, pool, assigned_teleports)
    for _, layer in ipairs(dst_map.layers) do
        if layer.type == "objectgroup" and layer.name == "sprites" then
            for _, obj in ipairs(layer.objects) do
                if obj.name == "teleport" then
                    -- choose a random room
                    local room = pick_from_pool(pool)
                    if not room then
                        goto continue_obj
                    end

                    -- modify the overworld teleport's properties to match the room
                    for _, prop in ipairs(obj.properties) do
                        if prop.name == "targetWorld" then
                            prop.value = "interior"
                        elseif prop.name == "targetIndex" then
                            prop.value = room.index
                        elseif prop.name == "targetX" then
                            prop.value = room.x
                        elseif prop.name == "targetY" then
                            prop.value = room.y
                        end
                    end

                    table.insert(assigned_teleports, {
                        room = room,
                        ow_cell = cell,
                        return_x = obj.x + obj.width / 2,
                        return_y = obj.y + obj.height + 16,
                    })

                    ::continue_obj::
                end
            end
        end
    end
end

-- for each assigned interior room, patch its teleport to return to the correct overworld location.
local function patch_interior_teleports(assigned_teleports)
    local dungeons_file = io.open("resources/dungeons.json", "r")
    if not dungeons_file then
        error("patch_interior_teleports: could not open dungeons.json")
    end
    local dungeons = json.decode(dungeons_file:read("*a"))
    dungeons_file:close()

    local ow_rooms = dungeons.overworld.levels[1].rooms

    for _, assignment in ipairs(assigned_teleports) do
        -- find the 0-based index of the overworld room by row/col
        local ow_index = nil
        for i, r in ipairs(ow_rooms) do
            if r.row == assignment.ow_cell.row and r.column == assignment.ow_cell.col then
                ow_index = i - 1 -- 0-based
                break
            end
        end
        if not ow_index then
            error(string.format("patch_interior_teleports: could not find overworld room [%d,%d] in dungeons.json", assignment.ow_cell.row, assignment.ow_cell.col))
        end
        local world = dungeons[assignment.room.world_key]
        if not world then
            error(string.format("patch_interior_teleports: unknown world key '%s'", assignment.room.world_key))
        end

        local rooms = world.levels[1].rooms
        local room_entry = rooms[assignment.room.index + 1] -- 1-indexed
        if not room_entry then
            error(string.format("patch_interior_teleports: no room at index %d in world '%s'", assignment.room.index, assignment.room.world_key))
        end

        local path = string.format("resources/tilemaps/%s/%s.json", assignment.room.world_key, room_entry.tilemap)
        local map = utils.loadJSON(path)

        for _, layer in ipairs(map.layers) do
            if layer.type == "objectgroup" and layer.name == "sprites" then
                for _, obj in ipairs(layer.objects) do
                    if obj.name == "teleport" then
                        for _, prop in ipairs(obj.properties) do
                            if prop.name == "targetWorld" then
                                prop.value = "overworld"
                            elseif prop.name == "targetIndex" then
                                prop.value = ow_index
                            elseif prop.name == "targetX" then
                                prop.value = assignment.return_x
                            elseif prop.name == "targetY" then
                                prop.value = assignment.return_y
                            end
                        end
                        break
                    end
                end
            end
        end

        utils.saveJSON(path, map)
        utils.print_file(string.format("  patched interior teleport in room %s/%s (index %d)", assignment.room.world_key, room_entry.tilemap, assignment.room.index))
    end
end



local function node_zone(zone_grid, name)
    -- resolves a node name like "OW_3_2" back to its zone and grid position
    local row, col = name:match("OW_(%d+)_(%d+)")
    row, col = tonumber(row), tonumber(col)
    return zone_grid[row][col], row, col
end

local function edge_requires(edge, requirement)
    -- true if the edge's requirement list contains the given requirement
    if not edge.requirements then
        return false
    end
    for _, req in ipairs(edge.requirements) do
        if req == requirement then
            return true
        end
    end
    return false
end

local function place_transition_feature(feature, to_row, to_col, from_row, from_col)
    -- stamps the feature's metatile onto the to-room, on the edge facing the from-cell
    local orientation = orientation_from_dir(from_row - to_row, from_col - to_col)
    if not orientation then
        error("place_transition_feature: from and to cells are not adjacent")
    end

    local key = feature.keys[orientation]
    if not key then
        error(string.format("place_transition_feature: feature %s->%s has no key for orientation '%s'", feature.from, feature.to, orientation))
    end

    local room_path = string.format("resources/tilemaps/generated/overworld/%s.json", WorldUtils.node_name(to_row, to_col))
    local room_map = utils.loadJSON(room_path)
    local metatile = OW_Metatiles.get_metatile_data(key)
    local geom = feature_orientations[orientation]

    -- inset along the perpendicular axis, centered along the parallel axis
    local dst_x, dst_y
    if geom.axis == "x" then
        if geom.side == "near" then
            dst_x = geom.inset
        else
            dst_x = room_map.width - geom.inset - metatile.w
        end
        dst_y = math.floor((room_map.height - metatile.h) / 2)
    else
        if geom.side == "near" then
            dst_y = geom.inset
        else
            dst_y = room_map.height - geom.inset - metatile.h
        end
        dst_x = math.floor((room_map.width - metatile.w) / 2)
    end

    -- clear collision objects under the footprint so the feature is walkable on foot.
    -- erase_object_region works in pixels, so convert the tile-space footprint
    local tile_w = room_map.tilewidth
    local tile_h = room_map.tileheight
    OW_Metatiles.erase_object_region("static_collision", dst_x * tile_w, dst_y * tile_h, (dst_x + metatile.w) * tile_w, (dst_y + metatile.h) * tile_h, room_map)

    -- overwrite = false so the metatile overlays the terrain instead of clearing it
    OW_Metatiles.place_metatile(key, room_map, dst_x, dst_y, false)

    -- strip tileset paths down to filenames so the saved map references them locally
    for _, tileset in ipairs(room_map.tilesets) do
        tileset.source = utils.basename(tileset.source)
    end

    utils.saveJSON(room_path, room_map)
    utils.print_file(string.format("  placed %s on %s", key, WorldUtils.node_name(to_row, to_col)))
end

local function place_transition_features(zone_grid, edges)
    -- stamps a metatile for every edge that matches a transition feature's from/to/requires
    for _, edge in ipairs(edges) do
        local from_zone, from_row, from_col = node_zone(zone_grid, edge.from)
        local to_zone, to_row, to_col = node_zone(zone_grid, edge.to)
        for _, feature in ipairs(transition_features) do
            -- edges are bidirectional, so only the from->to ordering matches: each transition handled once
            if from_zone == feature.from and to_zone == feature.to and edge_requires(edge, feature.requires) then
                place_transition_feature(feature, to_row, to_col, from_row, from_col)
            end
        end
    end
end


function TilemapModifier.process_overworld(zone_grid, edges, grid_width, grid_height, overlays)
    assert(zone_grid, "process_overworld: zone_grid is nil")
    assert(edges, "process_overworld: edges is nil")

    -- group adjacent same-zone cells into clusters (e.g. a 2-room town)
    local edge_set = build_edge_set(edges)
    local zone_clusters = find_zone_clusters(zone_grid, edge_set, grid_width, grid_height)

    -- copy the room pool so the original is not mutated across runs
    local available_rooms = {}
    for _, room in ipairs(room_pool) do
        table.insert(available_rooms, room)
    end

    -- collects every teleport we assign so we can patch the return trips afterwards
    local assigned_teleports = {}

    for _, cluster in ipairs(zone_clusters) do
        -- only zones with a role assigner (currently just town) get content stamped in
        local assign_roles = role_assigners[cluster.zone]
        if not assign_roles then
            goto next_cluster
        end

        -- map each cell to a role (north/south/single) and look up that zone's tilemaps
        local cell_roles = assign_roles(cluster)
        local zone_tilemaps = zone_content[cluster.zone]

        for _, cell in ipairs(cluster.cells) do
            -- skip cells without a role or without a tilemap for that role
            local role = cell_roles[cell.row] and cell_roles[cell.row][cell.col]
            local content_path = role and zone_tilemaps[role]
            if not content_path then
                goto next_cell
            end

            -- stamp the role's tilemap onto the generated overworld room
            local room_path = string.format("resources/tilemaps/generated/overworld/%s.json", WorldUtils.node_name(cell.row, cell.col))
            local room_map = utils.loadJSON(room_path)
            local content_map = utils.loadJSON(content_path)
            TilemapUtils.copy_region(room_map, content_map, 0, 0, 0, 0, room_map.width, room_map.height)

            -- wire up the town's teleports to interior rooms from the pool
            patch_ow_teleports(room_map, cell, available_rooms, assigned_teleports)

            -- strip tileset paths down to filenames so the saved map references them locally
            for _, tileset in ipairs(room_map.tilesets) do
                tileset.source = utils.basename(tileset.source)
            end

            utils.saveJSON(room_path, room_map)
            utils.print_file(string.format("  applied %s to %s (%s)", utils.basename(content_path), WorldUtils.node_name(cell.row, cell.col), role))

            ::next_cell::
        end

        ::next_cluster::
    end

    patch_interior_teleports(assigned_teleports)

    -- stamp transitions onto the maps
    place_transition_features(zone_grid, edges)

    -- apply feature overlays (e.g. river tiles) on top of zone content
    if overlays then
        for _, overlay in ipairs(overlays) do
            local room_path = string.format("resources/tilemaps/generated/overworld/%s.json",WorldUtils. node_name(overlay.row, overlay.col))
            local dst_map = utils.loadJSON(room_path)
            local overlay_paths_str = ""
            for _, entry in ipairs(overlay.tilemaps) do
                local src_map = utils.loadJSON(entry.path)
                overlay_paths_str = overlay_paths_str .. utils.basename(entry.path) .. ", "
                if entry.right_cols then
                    local src_x = src_map.width - entry.right_cols
                    TilemapUtils.copy_region(dst_map, src_map, src_x, 0, src_x, 0, entry.right_cols, src_map.height, entry.overwrite)
                else
                    TilemapUtils.copy_region(dst_map, src_map, 0, 0, 0, 0, dst_map.width, dst_map.height, entry.overwrite)
                end
            end
            for _, ts in ipairs(dst_map.tilesets) do
                ts.source = utils.basename(ts.source)
            end
            utils.saveJSON(room_path, dst_map)
            print(string.format("  applied %s overlay to %s", overlay_paths_str, WorldUtils.node_name(overlay.row, overlay.col)))
        end
    end

    print("tilemap_modifier: done")
end

return TilemapModifier