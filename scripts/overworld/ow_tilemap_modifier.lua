local utils = require("lib.utils")
local WorldUtils = require("lib.world_utils")
local TilemapModifier = {}

-- town tilemaps (assumes one large town (2 rooms) and a small one (1 room))
local zone_content = {
    town = {
        north = "resources/tilemaps/town_n.json",
        south = "resources/tilemaps/town_s.json",
        single = "resources/tilemaps/town_single.json",
    }
}

-- pool of interior rooms assignable to town teleports.
-- each entry: { filename, index, x, y } where x/y are the spawn coords inside the interior room.
local room_pool = {
    { world_key = "interior", index = 0, x = 158, y = 172 },
    { world_key = "interior", index = 1, x = 158, y = 172 },
    { world_key = "interior", index = 2, x = 158, y = 172 },
    { world_key = "interior", index = 3, x = 158, y = 172 },
    { world_key = "interior", index = 4, x = 158, y = 172 },
    { world_key = "interior", index = 5, x = 158, y = 172 },
    { world_key = "interior", index = 6, x = 158, y = 172 },
    { world_key = "interior", index = 7, x = 110, y = 172 },
    { world_key = "interior", index = 8, x = 158, y = 172 },
    { world_key = "interior", index = 9, x = 158, y = 172 },
    -- TODO add more rooms
}

local function pick_from_pool(pool)
    -- picks a raandom entry from a list
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

-- find a layer by name, returns nil if not found
local function find_layer(map, name)
    for _, layer in ipairs(map.layers) do
        if layer.name == name then
            return layer
        end
    end
    return nil
end

local function basename(path)
    return path:match("([^/\\]+)$") or path
end

local function find_owning_tileset(tilesets, gid)
    local owner = nil
    for _, ts in ipairs(tilesets) do
        if ts.firstgid <= gid then
            if owner == nil or ts.firstgid > owner.firstgid then
                owner = ts
            end
        end
    end

    if not owner then
        return nil, nil
    end

    return owner, gid - owner.firstgid
end

local function next_available_firstgid(tilesets)
    local max_gid = 1
    for _, ts in ipairs(tilesets) do
        if ts.firstgid >= max_gid then
            max_gid = ts.firstgid + 1000
        end
    end
    return max_gid
end

local function remap_gid(gid, src_tilesets, dst_tilesets)
    if gid == 0 then
        return 0
    end

    local src_ts, local_id = find_owning_tileset(src_tilesets, gid)
    if not src_ts then
        error("tilemap_modifier: could not find owning tileset for gid " .. gid)
    end

    local src_name = basename(src_ts.source)

    local dst_ts = nil
    for _, ts in ipairs(dst_tilesets) do
        if basename(ts.source) == src_name then
            dst_ts = ts
            break
        end
    end

    if not dst_ts then
        dst_ts = {
            firstgid = next_available_firstgid(dst_tilesets),
            source = src_ts.source
        }
        table.insert(dst_tilesets, dst_ts)
    end

    return dst_ts.firstgid + local_id
end

local function merge_tile_layer(dst_layer, src_layer, dst_x, dst_y, src_x, src_y, w, h, src_map, dst_map, mask)
    for row = 0, h - 1 do
        for col = 0, w - 1 do
            local src_index = (src_y + row) * src_map.width + (src_x + col) + 1
            local dst_index = (dst_y + row) * dst_map.width + (dst_x + col) + 1

            local src_tile = src_layer.data[src_index]
            local dst_tile = dst_layer.data[dst_index]

            if src_tile and src_tile ~= 0 then
                dst_layer.data[dst_index] = remap_gid(src_tile, src_map.tilesets, dst_map.tilesets)
            elseif mask and mask[row * w + col] then
                dst_layer.data[dst_index] = 0
            elseif dst_tile == nil then
                dst_layer.data[dst_index] = 0
            end
        end
    end
end

local function merge_object_layer(dst_layer, src_layer, dst_x, dst_y, src_x, src_y, w, h, tile_w, tile_h, dst_map)
    local rx = src_x * tile_w
    local ry = src_y * tile_h
    local rx2 = (src_x + w) * tile_w
    local ry2 = (src_y + h) * tile_h

    local offset_x = (dst_x - src_x) * tile_w
    local offset_y = (dst_y - src_y) * tile_h

    for _, obj in ipairs(src_layer.objects) do
        local ox2 = obj.x + obj.width
        local oy2 = obj.y + obj.height

        if obj.x < rx2 and ox2 > rx and obj.y < ry2 and oy2 > ry then
            local clipped = {}
            for k, v in pairs(obj) do
                clipped[k] = v
            end

            local clipped_x = math.max(obj.x, rx)
            local clipped_y = math.max(obj.y, ry)
            local clipped_x2 = math.min(ox2, rx2)
            local clipped_y2 = math.min(oy2, ry2)

            clipped.x = clipped_x + offset_x
            clipped.y = clipped_y + offset_y
            clipped.width = clipped_x2 - clipped_x
            clipped.height = clipped_y2 - clipped_y

            clipped.id = dst_map.nextobjectid
            dst_map.nextobjectid = dst_map.nextobjectid + 1

            table.insert(dst_layer.objects, clipped)
        end
    end
end

local function copy_region(dst, src, dst_x, dst_y, src_x, src_y, w, h, overwrite)
    -- build a mask of positions that have content on any source layer
    local mask = nil
    if overwrite then
        mask = {}
        for _, src_layer in ipairs(src.layers) do
            if src_layer.type == "tilelayer" then
                for row = 0, h - 1 do
                    for col = 0, w - 1 do
                        local src_index = (src_y + row) * src.width + (src_x + col) + 1
                        if src_layer.data[src_index] and src_layer.data[src_index] ~= 0 then
                            mask[row * w + col] = true
                        end
                    end
                end
            end
        end
    end

    for _, src_layer in ipairs(src.layers) do
        local dst_layer = find_layer(dst, src_layer.name)
        if not dst_layer then
            utils.print_file("tilemap_modifier.copy_region: warning: layer '" .. src_layer.name .. "' not found in dst, skipping")
        elseif src_layer.type == "tilelayer" then
            merge_tile_layer(dst_layer, src_layer, dst_x, dst_y, src_x, src_y, w, h, src, dst, mask)
        elseif src_layer.type == "objectgroup" then
            merge_object_layer(dst_layer, src_layer, dst_x, dst_y, src_x, src_y, w, h, src.tilewidth, src.tileheight, dst)
        end
    end
end

-- patch teleport objects in a town dst_map with rooms from the pool.
-- appends to assignments: { room, teleport_x, teleport_y } for each assigned teleport,
-- where teleport_x/y is the overworld return position (16px below the teleport rect).
local function patch_teleports(dst_map, cell, pool, assignments)
    for _, layer in ipairs(dst_map.layers) do
        if layer.type == "objectgroup" and layer.name == "sprites" then
            for _, obj in ipairs(layer.objects) do
                if obj.name == "teleport" then
                    local room = pick_from_pool(pool)
                    if not room then
                        goto continue_obj
                    end

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

                    table.insert(assignments, {
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

-- for each assigned interior room, patch its teleport to return to the overworld.
local function patch_interior_rooms(assignments)
    local dungeons_file = io.open("resources/dungeons.json", "r")
    if not dungeons_file then
        error("patch_interior_rooms: could not open dungeons.json")
    end
    local dungeons = json.decode(dungeons_file:read("*a"))
    dungeons_file:close()

    local ow_rooms = dungeons.overworld.levels[1].rooms

    for _, assignment in ipairs(assignments) do
        -- find the 0-based index of the overworld room by row/col
        local ow_index = nil
        for i, r in ipairs(ow_rooms) do
            if r.row == assignment.ow_cell.row and r.column == assignment.ow_cell.col then
                ow_index = i - 1 -- 0-based
                break
            end
        end
        if not ow_index then
            error(string.format("patch_interior_rooms: could not find overworld room [%d,%d] in dungeons.json", assignment.ow_cell.row, assignment.ow_cell.col))
        end
        local world = dungeons[assignment.room.world_key]
        if not world then
            error(string.format("patch_interior_rooms: unknown world key '%s'", assignment.room.world_key))
        end

        local rooms = world.levels[1].rooms
        local room_entry = rooms[assignment.room.index + 1] -- 1-indexed
        if not room_entry then
            error(string.format("patch_interior_rooms: no room at index %d in world '%s'", assignment.room.index, assignment.room.world_key))
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
        utils.print_file(string.format("  patched interior room %s/%s (index %d)", assignment.room.world_key, room_entry.tilemap, assignment.room.index))
    end
end

function TilemapModifier.process_overworld(zone_grid, edges, grid_width, grid_height, overlays)
    assert(zone_grid, "process_overworld: zone_grid is nil")
    assert(edges, "process_overworld: edges is nil")

    local edge_set = build_edge_set(edges)
    local clusters = find_zone_clusters(zone_grid, edge_set, grid_width, grid_height)

    -- copy pool so the original is not mutated across runs
    local pool = {}
    for _, entry in ipairs(room_pool) do
        table.insert(pool, entry)
    end

    local assignments = {}

    for _, cluster in ipairs(clusters) do
        local assigner = role_assigners[cluster.zone]
        if not assigner then
            goto continue
        end

        local roles = assigner(cluster)
        local content = zone_content[cluster.zone]

        for _, cell in ipairs(cluster.cells) do
            local role = roles[cell.row] and roles[cell.row][cell.col]
            local content_path = role and content[role]
            if not content_path then
                goto continue_cell
            end

            local room_path = string.format("resources/tilemaps/generated/overworld/%s.json", WorldUtils.node_name(cell.row, cell.col))
            local dst_map = utils.loadJSON(room_path)
            local src_map = utils.loadJSON(content_path)
            copy_region(dst_map, src_map, 0, 0, 0, 0, dst_map.width, dst_map.height)
            patch_teleports(dst_map, cell, pool, assignments)

            for _, ts in ipairs(dst_map.tilesets) do
                ts.source = basename(ts.source)
            end

            utils.saveJSON(room_path, dst_map)
            utils.print_file(string.format("  applied %s to %s (%s)", basename(content_path), WorldUtils.node_name(cell.row, cell.col), role))

            ::continue_cell::
        end

        ::continue::
    end

    -- apply feature overlays (e.g. river tiles) on top of zone content
    if overlays then
        for _, overlay in ipairs(overlays) do
            local room_path = string.format("resources/tilemaps/generated/overworld/%s.json",WorldUtils. node_name(overlay.row, overlay.col))
            local dst_map = utils.loadJSON(room_path)
            local overlay_paths_str = ""
            for _, entry in ipairs(overlay.tilemaps) do
                local src_map = utils.loadJSON(entry.path)
                overlay_paths_str = overlay_paths_str .. basename(entry.path) .. ", "
                if entry.right_cols then
                    local src_x = src_map.width - entry.right_cols
                    copy_region(dst_map, src_map, src_x, 0, src_x, 0, entry.right_cols, src_map.height, entry.overwrite)
                else
                    copy_region(dst_map, src_map, 0, 0, 0, 0, dst_map.width, dst_map.height, entry.overwrite)
                end
            end
            for _, ts in ipairs(dst_map.tilesets) do
                ts.source = basename(ts.source)
            end
            utils.saveJSON(room_path, dst_map)
            print(string.format("  applied %s overlay to %s", overlay_paths_str, WorldUtils.node_name(overlay.row, overlay.col)))
        end
    end

    patch_interior_rooms(assignments)

    print("tilemap_modifier: done")
end

return TilemapModifier