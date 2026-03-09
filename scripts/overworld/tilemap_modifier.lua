local TilemapModifier = {}

-- TODO maybe this has enough common code to be inside of the other tilemap_modifier script

-- base tilemap for each zone, loaded once and reused
local base_tilemaps = {
    field = "resources/tilemaps/base/overworld/fields_empty.json",
    forest = "resources/tilemaps/base/overworld/fields_empty.json", -- TODO
    lake = "resources/tilemaps/base/overworld/lake_base.json",
    mountain = "resources/tilemaps/base/overworld/mountain_base.json",
    town = "resources/tilemaps/base/overworld/mountain_base.json",
}

local border_tilemaps_inner = {
    forest = "resources/tilemaps/base/overworld/forest_base_inner_corners.json",
    lake = "resources/tilemaps/base/overworld/lake_base_inner_corners.json",
    mountain = "resources/tilemaps/base/overworld/mountain_base_inner_corners.json",
    lake_inverted = "resources/tilemaps/base/overworld/lake_base_inner_corners_inverted.json"
}

local border_tilemaps_outer = {
    forest = "resources/tilemaps/base/overworld/forest_base_outer_corners.json",
    mountain = "resources/tilemaps/base/overworld/mountain_base_outer_corners.json",
    lake_inverted = "resources/tilemaps/base/overworld/lake_base_outer_corners_inverted.json"
}

local boundary_tilemaps = {
    lake = "resources/tilemaps/base/overworld/lake_world_boundaries.json",
    mountain = "resources/tilemaps/base/overworld/mountain_world_boundaries.json",
    field = "resources/tilemaps/base/overworld/fields_world_boundaries.json"
    -- forest is the same as border_tilemaps_inner
}

-- transitions (free, or with requirement)
local transition_tilemaps = {
    field_to_lake = "resources/tilemaps/base/overworld/lake_base_transitions_inverted.json",
    field_to_mountain = "resources/tilemaps/base/overworld/mountain_base_transitions.json",
    field_to_forest = "resources/tilemaps/base/overworld/forest_base_transitions.json"
}

-- border_chunks[this_zone][neighbor_zone][side] defines what chunk to paste onto THIS room
-- when a side is closed off toward a given neighbor zone (or "boundary").
-- borders are asymmetric: e.g. a cliff is only pasted onto the field side, not the mountain side.
-- a nil entry means this room gets nothing pasted on that side (the neighbor handles it, or no border needed).
local CHUNK_SIZE = 20

local CENTERS = {
    north = { src_x = 10, src_y = 0,  dst_positions = { {dst_x = 0,  dst_y = 0},  {dst_x = 20, dst_y = 0}  } },
    south = { src_x = 10, src_y = 20, dst_positions = { {dst_x = 0,  dst_y = 20}, {dst_x = 20, dst_y = 20} } },
    east  = { src_x = 20, src_y = 10, dst_positions = { {dst_x = 20, dst_y = 0},  {dst_x = 20, dst_y = 20} } },
    west  = { src_x = 0,  src_y = 10, dst_positions = { {dst_x = 0,  dst_y = 0},  {dst_x = 0,  dst_y = 20} } }
}

local CORNERS = {
    NW = { src_x = 0,  src_y = 0,  dst_x = 0,  dst_y = 0  },
    NE = { src_x = 20, src_y = 0,  dst_x = 20, dst_y = 0  },
    SW = { src_x = 0,  src_y = 20, dst_x = 0,  dst_y = 20 },
    SE = { src_x = 20, src_y = 20, dst_x = 20, dst_y = 20 }
}

-- which corner covers which center dst position
local corner_covers = {
    north = { [0] = "NW", [20] = "NE" },
    south = { [0] = "SW", [20] = "SE" },
    east  = { [0] = "NE", [20] = "SE" },
    west  = { [0] = "NW", [20] = "SW" }
}

local border_chunks = {
    field = {
        forest = border_tilemaps_inner.forest,
        mountain = border_tilemaps_inner.mountain,
        lake = border_tilemaps_inner.lake_inverted,
        town = nil,
        boundary = boundary_tilemaps.field
    },
    forest = {
        field = border_tilemaps_inner.forest,
        mountain = border_tilemaps_inner.mountain,
        town = border_tilemaps_inner.forest,
        lake = border_tilemaps_inner.forest,
        boundary = border_tilemaps_inner.forest
    },
    mountain = {
        field = nil, -- cliff is on the field map
        town = nil,
        lake = nil,
        forest = nil,
        mountain = border_tilemaps_inner.mountain,
        boundary = boundary_tilemaps.mountain
    },
    lake = {
        field = nil, -- water edge is on the field map
        town = nil,
        forest = nil,
        mountain = nil,
        boundary = boundary_tilemaps.lake
    },
    town = {
        field = border_tilemaps_inner.forest,
        forest = border_tilemaps_inner.forest,
        mountain = border_tilemaps_inner.mountain,
        lake = border_tilemaps_inner.lake,
        boundary = boundary_tilemaps.field
    }
}

-- transition_chunks[my_zone][neighbor_zone] = source path, or nil for no transition tile
local transition_chunks = {
    field = {
        lake     = transition_tilemaps.field_to_lake,
        mountain = transition_tilemaps.field_to_mountain,
        forest   = transition_tilemaps.field_to_forest,
    },
    forest = {
        field    = transition_tilemaps.field_to_forest, -- symmetric, trees on both sides
        mountain = nil,
        lake     = nil,
    },
    mountain = {
        field    = nil, -- transition is on the field map only
        forest   = nil,
    },
    lake = {
        field    = nil, -- transition is on the field map only
        town     = nil,
    },
    town = {
        field    = nil,
        forest   = nil,
        mountain = nil,
        lake     = nil,
    }
}

-- outer corners 
local outer_corner_chunks = {
    field = {
        lake = border_tilemaps_outer.lake_inverted,
        mountain = border_tilemaps_outer.mountain,
        forest = border_tilemaps_outer.forest,
    },
    mountain = {
        lake = border_tilemaps_outer.lake_inverted,
        forest = border_tilemaps_outer.forest,
        field = nil
    }
}

local TRANSITION_DST = {
    north = { dst_x = 10, dst_y = 0  },
    south = { dst_x = 10, dst_y = 20 },
    east  = { dst_x = 20, dst_y = 10 },
    west  = { dst_x = 0,  dst_y = 10 },
}

-- load a tilemap from a json file
function TilemapModifier.load(path)
    local file = io.open(path, "r")
    if not file then
        error("tilemap.load: could not open file: " .. path)
    end

    local contents = file:read("*a")
    file:close()

    local map = json.decode(contents)
    if not map then
        error("tilemap.load: failed to decode json from: " .. path)
    end

    return map
end

local function neighbor_has_border_or_transition(room_states, row, col, grid_width, grid_height, side)
    if row < 0 or row >= grid_height or col < 0 or col >= grid_width then
        return false
    end
    return room_states[row][col][side] ~= false and room_states[row][col][side] ~= nil
end

-- save a tilemap to a json file
function TilemapModifier.save(map, path)
    local contents = json.encode(map)
    if not contents then
        error("tilemap.save: failed to encode map to json")
    end

    local file = io.open(path, "w")
    if not file then
        error("tilemap.save: could not open file for writing: " .. path)
    end

    file:write(contents)
    file:close()
end

-- find a layer by name, returns nil if not found
local function find_layer(map, name)
    for _, layer in ipairs(map.layers) do
        if layer.name == name then
            return layer
        end
    end
    return nil
end

-- extract just the filename from a path, e.g. "../../foo/bar.tsj" -> "bar.tsj"
local function basename(path)
    return path:match("([^/\\]+)$") or path
end

-- find which tileset in a map owns a given gid.
-- returns the tileset entry and the local tile index within it.
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

    local local_id = gid - owner.firstgid
    return owner, local_id
end

-- find the highest currently used firstgid in a tileset list,
-- used when appending a new tileset to the destination map.
local function next_available_firstgid(tilesets)
    local max_gid = 1
    for _, ts in ipairs(tilesets) do
        if ts.firstgid >= max_gid then
            -- we don't know tileset sizes here, so just leave a gap of 1000 to be safe
            max_gid = ts.firstgid + 1000
        end
    end
    return max_gid
end

-- remap a tile gid from src's tileset space into dst's tileset space.
-- if the owning tileset doesn't exist in dst yet, it will be appended.
-- returns the remapped gid, or 0 if the gid is empty (0).
local function remap_gid(gid, src_tilesets, dst_tilesets)
    if gid == 0 then
        return 0
    end

    local src_ts, local_id = find_owning_tileset(src_tilesets, gid)
    if not src_ts then
        error("tilemap: could not find owning tileset for gid " .. gid)
    end

    local src_name = basename(src_ts.source)

    -- find matching tileset in dst by filename
    local dst_ts = nil
    for _, ts in ipairs(dst_tilesets) do
        if basename(ts.source) == src_name then
            dst_ts = ts
            break
        end
    end

    -- tileset not present in dst yet, append it
    if not dst_ts then
        dst_ts = {
            firstgid = next_available_firstgid(dst_tilesets),
            source   = src_ts.source
        }
        table.insert(dst_tilesets, dst_ts)
    end

    return dst_ts.firstgid + local_id
end

-- merge tile layer data from src into dst for a given region.
-- tiles are OR'd: if both are non-zero, the source (pasted) tile wins.
-- gids are remapped from src tileset space into dst tileset space.
local function merge_tile_layer(dst_layer, src_layer, dst_x, dst_y, src_x, src_y, w, h, src_map, dst_map)
    for row = 0, h - 1 do
        for col = 0, w - 1 do
            local src_index = (src_y + row) * src_map.width + (src_x + col) + 1
            local dst_index = (dst_y + row) * dst_map.width + (dst_x + col) + 1

            local src_tile = src_layer.data[src_index]
            local dst_tile = dst_layer.data[dst_index]

            if src_tile and src_tile ~= 0 then
                dst_layer.data[dst_index] = remap_gid(src_tile, src_map.tilesets, dst_map.tilesets)
            elseif dst_tile == nil then
                dst_layer.data[dst_index] = 0
            end
        end
    end
end

-- copy and clip objects from src_layer that overlap the given region into dst_layer.
-- objects are offset to match the destination position.
local function merge_object_layer(dst_layer, src_layer, dst_x, dst_y, src_x, src_y, w, h, tile_w, tile_h, dst_map)
    -- region bounds in pixels
    local rx  = src_x * tile_w
    local ry  = src_y * tile_h
    local rx2 = (src_x + w) * tile_w
    local ry2 = (src_y + h) * tile_h

    -- pixel offset to apply to copied objects
    local offset_x = (dst_x - src_x) * tile_w
    local offset_y = (dst_y - src_y) * tile_h

    for _, obj in ipairs(src_layer.objects) do
        local ox2 = obj.x + obj.width
        local oy2 = obj.y + obj.height

        -- skip if fully outside the region
        if obj.x < rx2 and ox2 > rx and obj.y < ry2 and oy2 > ry then
            local clipped = {}
            for k, v in pairs(obj) do
                clipped[k] = v
            end

            -- clip to region bounds and offset to destination
            local clipped_x  = math.max(obj.x, rx)
            local clipped_y  = math.max(obj.y, ry)
            local clipped_x2 = math.min(ox2, rx2)
            local clipped_y2 = math.min(oy2, ry2)

            clipped.x      = clipped_x + offset_x
            clipped.y      = clipped_y + offset_y
            clipped.width  = clipped_x2 - clipped_x
            clipped.height = clipped_y2 - clipped_y

            -- assign a fresh id
            clipped.id = dst_map.nextobjectid
            dst_map.nextobjectid = dst_map.nextobjectid + 1

            table.insert(dst_layer.objects, clipped)
        end
    end
end

-- copy a w x h tile region from src at (src_x, src_y) and merge it into dst at (dst_x, dst_y).
-- dst is modified in place.
-- tile layers are OR-merged with source winning on conflict, gids are remapped.
-- object layers are clipped to the region and appended with remapped ids.
function TilemapModifier.copy_region(dst, src, dst_x, dst_y, src_x, src_y, w, h)
    assert(dst, "tilemap.copy_region: dst is nil")
    assert(src, "tilemap.copy_region: src is nil")

    assert(src_x >= 0 and src_y >= 0, "tilemap.copy_region: source origin out of bounds")
    assert(src_x + w <= src.width and src_y + h <= src.height, "tilemap.copy_region: source region exceeds map bounds")
    assert(dst_x >= 0 and dst_y >= 0, "tilemap.copy_region: destination origin out of bounds")
    assert(dst_x + w <= dst.width and dst_y + h <= dst.height, "tilemap.copy_region: destination region exceeds map bounds")

    for _, src_layer in ipairs(src.layers) do
        local dst_layer = find_layer(dst, src_layer.name)

        if not dst_layer then
            print("tilemap.copy_region: warning: layer '" .. src_layer.name .. "' not found in dst, skipping")
        elseif src_layer.type == "tilelayer" then
            merge_tile_layer(dst_layer, src_layer, dst_x, dst_y, src_x, src_y, w, h, src, dst)
        elseif src_layer.type == "objectgroup" then
            merge_object_layer(dst_layer, src_layer, dst_x, dst_y, src_x, src_y, w, h, src.tilewidth, src.tileheight, dst)
        end
    end
end

local function node_name(row, col)
    return string.format("OW_%d_%d", row, col)
end

local function build_edge_set(edges)
    local set = {}
    for _, edge in ipairs(edges) do
        set[edge.from .. "->" .. edge.to] = true
    end
    return set
end

local function are_connected(edge_set, from_row, from_col, to_row, to_col)
    local key = node_name(from_row, from_col) .. "->" .. node_name(to_row, to_col)
    return edge_set[key] == true
end

local function get_neighbor_zone(zone_grid, row, col, dr, dc, grid_width, grid_height)
    local nr = row + dr
    local nc = col + dc
    if nr < 0 or nr >= grid_height or nc < 0 or nc >= grid_width then
        return "boundary"
    end
    return zone_grid[nr][nc]
end

-- cache of loaded source tilemaps to avoid redundant disk reads within a single room pass
local src_map_cache = {}

local function get_src_map(path)
    if not src_map_cache[path] then
        src_map_cache[path] = TilemapModifier.load(path)
    end
    return src_map_cache[path]
end

local function apply_center(dst_map, source_path, side)
    local center = CENTERS[side]
    local src_map = get_src_map(source_path)
    for _, dst_pos in ipairs(center.dst_positions) do
        TilemapModifier.copy_region(dst_map, src_map, dst_pos.dst_x, dst_pos.dst_y, center.src_x, center.src_y, CHUNK_SIZE, CHUNK_SIZE)
    end
end

local function apply_corner(dst_map, corner, sources)
    local c = CORNERS[corner]
    local half_h = CHUNK_SIZE / 2  -- 10

    if sources.source_h == sources.source_v then
        -- same source, single paste
        local src_map = get_src_map(sources.source_h)
        TilemapModifier.copy_region(dst_map, src_map, c.dst_x, c.dst_y, c.src_x, c.src_y, CHUNK_SIZE, CHUNK_SIZE)
    else
        -- split: top 10x5 from horizontal side, bottom 10x5 from vertical side
        if sources.source_h then
            local src_map = get_src_map(sources.source_h)
            TilemapModifier.copy_region(dst_map, src_map, c.dst_x, c.dst_y, c.src_x, c.src_y, half_h, 5)
        end
        if sources.source_v then
            local src_map = get_src_map(sources.source_v)
            TilemapModifier.copy_region(dst_map, src_map, c.dst_x, c.dst_y + 5, c.src_x, c.src_y + 5, half_h, 5)
        end
    end
end

local function apply_transition(dst_map, source_path, side)
    local pos = TRANSITION_DST[side]
    local center = CENTERS[side]
    local src_map = get_src_map(source_path)
    TilemapModifier.copy_region(dst_map, src_map, pos.dst_x, pos.dst_y, center.src_x, center.src_y, CHUNK_SIZE, CHUNK_SIZE)
end

local function trim_objects_in_region(dst_map, dst_x, dst_y, w, h)
    local tile_w = dst_map.tilewidth
    local tile_h = dst_map.tileheight
    local rx  = dst_x * tile_w
    local ry  = dst_y * tile_h
    local rx2 = (dst_x + w) * tile_w
    local ry2 = (dst_y + h) * tile_h

    for _, layer in ipairs(dst_map.layers) do
        if layer.type == "objectgroup" and layer.objects then
            local kept = {}
            for _, obj in ipairs(layer.objects) do
                local ox2 = obj.x + obj.width
                local oy2 = obj.y + obj.height

                -- fully outside: keep as-is
                if obj.x >= rx2 or ox2 <= rx or obj.y >= ry2 or oy2 <= ry then
                    table.insert(kept, obj)
                -- partially overlapping: clip it
                elseif obj.x < rx or ox2 > rx2 or obj.y < ry or oy2 > ry2 then
                    local new_x  = math.max(obj.x, rx)
                    local new_y  = math.max(obj.y, ry)
                    local new_x2 = math.min(ox2, rx2)
                    local new_y2 = math.min(oy2, ry2)
                    -- only keep if there's a meaningful remainder outside the region
                    if obj.x < rx then
                        local trimmed = {}
                        for k, v in pairs(obj) do trimmed[k] = v end
                        trimmed.width = rx - obj.x
                        table.insert(kept, trimmed)
                    end
                    if ox2 > rx2 then
                        local trimmed = {}
                        for k, v in pairs(obj) do trimmed[k] = v end
                        trimmed.x = rx2
                        trimmed.width = ox2 - rx2
                        table.insert(kept, trimmed)
                    end
                    if obj.y < ry then
                        local trimmed = {}
                        for k, v in pairs(obj) do trimmed[k] = v end
                        trimmed.height = ry - obj.y
                        table.insert(kept, trimmed)
                    end
                    if oy2 > ry2 then
                        local trimmed = {}
                        for k, v in pairs(obj) do trimmed[k] = v end
                        trimmed.y = ry2
                        trimmed.height = oy2 - ry2
                        table.insert(kept, trimmed)
                    end
                end
                -- fully inside: discard
            end
            layer.objects = kept
        end
    end
end


local directions = {
    north = { dr = -1, dc = 0 },
    south = { dr =  1, dc = 0 },
    east  = { dr =  0, dc = 1 },
    west  = { dr =  0, dc = -1 },
}

-- corner combinations: which two sides must both be closed, and which corner piece to use
local corner_pairs = {
    { corner = "NW", h_side = "north", v_side = "west" },
    { corner = "NE", h_side = "north", v_side = "east" },
    { corner = "SW", h_side = "south", v_side = "west" },
    { corner = "SE", h_side = "south", v_side = "east" },
}


function TilemapModifier.process_overworld(zone_grid, edges, grid_width, grid_height)
    assert(zone_grid, "process_overworld: zone_grid is nil")
    assert(edges, "process_overworld: edges is nil")

    filesystem.create_directory("resources/tilemaps/generated/overworld")

    local edge_set = build_edge_set(edges)

    -- make a room_states table that saved the adjacent zones
    local room_states = {}
    for row = 0, grid_height - 1 do
        room_states[row] = {}
        for col = 0, grid_width - 1 do
            local my_zone = zone_grid[row][col]
            local sides = {}
            for side, dir in pairs(directions) do
                local neighbor_zone = get_neighbor_zone(zone_grid, row, col, dir.dr, dir.dc, grid_width, grid_height)
                local is_closed = neighbor_zone == "boundary" or not are_connected(edge_set, row, col, row + dir.dr, col + dir.dc)
                local has_border = is_closed and (border_chunks[my_zone] and border_chunks[my_zone][neighbor_zone] or false)
                local has_transition = not is_closed and (transition_chunks[my_zone] and transition_chunks[my_zone][neighbor_zone] or false)
                sides[side] = has_border or has_transition
            end
            room_states[row][col] = sides
        end
    end

    local base_tilemap_cache = {}

    for row = 0, grid_height - 1 do
        for col = 0, grid_width - 1 do
            local my_zone = zone_grid[row][col]
            local base_path = base_tilemaps[my_zone]

            if not base_path then
                print(string.format("process_overworld: warning: no base tilemap for zone '%s' at (%d,%d), skipping", my_zone, row, col))
                goto continue
            end

            if not base_tilemap_cache[my_zone] then
                base_tilemap_cache[my_zone] = TilemapModifier.load(base_path)
            end

            local dst_map = json.decode(json.encode(base_tilemap_cache[my_zone]))

            -- collect closed sides and their source paths
            local closed = {} -- closed[side] = source_path, or nil if no border needed

            for side, dir in pairs(directions) do
                local neighbor_zone = get_neighbor_zone(zone_grid, row, col, dir.dr, dir.dc, grid_width, grid_height)
                local is_closed = neighbor_zone == "boundary" or not are_connected(edge_set, row, col, row + dir.dr, col + dir.dc)

                if is_closed then
                    local source_path = border_chunks[my_zone] and border_chunks[my_zone][neighbor_zone] or nil
                    -- use false as sentinel to distinguish "closed with no border" from "not closed"
                    closed[side] = source_path or false
                end
            end

            local active_corners = {}
            for _, pair in ipairs(corner_pairs) do
                local source_h = closed[pair.h_side]
                local source_v = closed[pair.v_side]
                if source_h ~= nil and source_v ~= nil then
                    local resolved_h = source_h or false
                    local resolved_v = source_v or false
                    if resolved_h or resolved_v then
                        active_corners[pair.corner] = {
                            source_h = resolved_h,
                            source_v = resolved_v
                        }
                    end
                end
            end

            -- pass 1: centers, skipping positions covered by active corners
            for side, source_path in pairs(closed) do
                if source_path then
                    apply_center(dst_map, source_path, side)
                end
            end

            -- pass 2: corners
            for corner, sources in pairs(active_corners) do
                apply_corner(dst_map, corner, sources)
            end

            -- pass 3: transition tiles on open sides
            for side, dir in pairs(directions) do
                if closed[side] == nil then
                    local neighbor_zone = get_neighbor_zone(zone_grid, row, col, dir.dr, dir.dc, grid_width, grid_height)
                    local transition_source = transition_chunks[my_zone] and transition_chunks[my_zone][neighbor_zone] or nil
                    if transition_source then
                        -- paste the border first as a base, then the transition on top
                        local border_source = border_chunks[my_zone] and border_chunks[my_zone][neighbor_zone] or nil
                        if border_source then
                            apply_center(dst_map, border_source, side)
                            local pos = TRANSITION_DST[side]
                            trim_objects_in_region(dst_map, pos.dst_x, pos.dst_y, CHUNK_SIZE, CHUNK_SIZE)
                        end
                        apply_transition(dst_map, transition_source, side)
                    end
                end
            end

            -- pass 4: outer corners
            local outer_corner_checks = {
                NW = {
                    { row = row - 1, col = col,     side = "west" },
                    { row = row,     col = col - 1, side = "north" },
                },
                NE = {
                    { row = row - 1, col = col,     side = "east" },
                    { row = row,     col = col + 1, side = "north" },
                },
                SW = {
                    { row = row + 1, col = col,     side = "west" },
                    { row = row,     col = col - 1, side = "south" },
                },
                SE = {
                    { row = row + 1, col = col,     side = "east" },
                    { row = row,     col = col + 1, side = "south" },
                },
            }

            -- check diagonally adjacent zones for correct outer corner placement
            local corner_diagonal = {
                NW = { row = row - 1, col = col - 1 },
                NE = { row = row - 1, col = col + 1 },
                SW = { row = row + 1, col = col - 1 },
                SE = { row = row + 1, col = col + 1 },
            }

            for corner, checks in pairs(outer_corner_checks) do
                local a = checks[1]
                local b = checks[2]
    
                local a_result = neighbor_has_border_or_transition(room_states, a.row, a.col, grid_width, grid_height, a.side)
                local b_result = neighbor_has_border_or_transition(room_states, b.row, b.col, grid_width, grid_height, b.side)
    
                print(string.format("  [%d,%d] corner %s: check a=[%d,%d].%s=%s, check b=[%d,%d].%s=%s",
                    row, col, corner,
                    a.row, a.col, a.side, tostring(a_result),
                    b.row, b.col, b.side, tostring(b_result)))

                if a_result and b_result then
                    local diag = corner_diagonal[corner]
                    local diag_zone = zone_grid[diag.row] and zone_grid[diag.row][diag.col] or nil
                    print(string.format("    diag=[%d,%d] zone=%s", diag.row, diag.col, tostring(diag_zone)))

                    local source_path = diag_zone and (outer_corner_chunks[my_zone] and outer_corner_chunks[my_zone][diag_zone] or nil) or nil
                    local neighbor_zone = diag_zone

                    if source_path then
                        apply_corner(dst_map, corner, { source_h = source_path, source_v = source_path })
                        print(string.format("  outer corner %s pasted at [%d,%d] (%s) neighbor zone: %s", corner, row, col, my_zone, tostring(neighbor_zone)))
                    end
                end
            end

            -- change the tileset path to only the basename
            for _, ts in ipairs(dst_map.tilesets) do
                ts.source = basename(ts.source)
            end

            src_map_cache = {}

            local out_path = string.format("resources/tilemaps/generated/overworld/%s.json", node_name(row, col))
            TilemapModifier.save(dst_map, out_path)
            print(string.format("  saved %s (%s)", out_path, my_zone))

            ::continue::
        end
    end

    -- build doors string for this room: [right][up][left][down]
    local side_to_door_bit = {
        east  = 1, -- right
        north = 2, -- up
        west  = 3, -- left
        south = 4, -- down
    }

    -- load and update dungeons.json
    local dungeons_path = "resources/dungeons.json"
    local dungeons_file = io.open(dungeons_path, "r")
    if not dungeons_file then
        error("process_overworld: could not open " .. dungeons_path)
    end
    local all_dungeons = json.decode(dungeons_file:read("*a"))
    dungeons_file:close()

    local ow_rooms = {}

    for row = 0, grid_height - 1 do
        for col = 0, grid_width - 1 do
            local my_zone = zone_grid[row][col]
            if not base_tilemaps[my_zone] then
                goto continue_save
            end

            local door_bits = {1, 1, 1, 1} -- default all open
            for side, dir in pairs(directions) do
                local nr = row + dir.dr
                local nc = col + dir.dc
                local in_bounds = nr >= 0 and nr < grid_height and nc >= 0 and nc < grid_width
                if in_bounds and not are_connected(edge_set, row, col, nr, nc) then
                    door_bits[side_to_door_bit[side]] = 0
                elseif not in_bounds then
                    door_bits[side_to_door_bit[side]] = 0
                end
            end

            table.insert(ow_rooms, {
                row     = row,
                column  = col,
                doors   = table.concat(door_bits),
                tilemap = node_name(row, col)
            })

            ::continue_save::
        end
    end

    all_dungeons.overworld.levels[1].rooms = ow_rooms
    all_dungeons.overworld.rooms_w = grid_width
    all_dungeons.overworld.rooms_h = grid_height
    all_dungeons.overworld.seed = dungeon_seed

    local out = io.open(dungeons_path, "w")
    out:write(json.encode(all_dungeons, 2))
    out:close()
    print("process_overworld: updated dungeons.json")

    print("process_overworld: done")
end

return TilemapModifier