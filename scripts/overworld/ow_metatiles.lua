local OW_Metatiles = {}

local atlases = {
    overworld = "resources/tilemaps/base/overworld/ow_metatiles.json",
}

local atlas_cache = {} -- cache loaded tilemaps

local TILESIZE = 16
local TILESET_GID_GAP = 1000 -- arbitrary gid difference between two tilesets that are used in the same tilemap

-- n,s,w,e = north, south, west, east

local metatiles = {
    -- river
    river_vertical = {atlas = "overworld", src_x = 0, src_y = 0, w = 8, h = 10},
    river_horizontal = {atlas = "overworld", src_x = 8, src_y = 0, w = 10, h = 8},
    river_corner_ne = {atlas = "overworld", src_x = 18, src_y = 0, w = 8, h = 8},
    river_corner_nw = {atlas = "overworld", src_x = 26, src_y = 0, w = 8, h = 8},
    river_corner_se = {atlas = "overworld", src_x = 34, src_y = 0, w = 8, h = 8},
    river_corner_sw = {atlas = "overworld", src_x = 42, src_y = 0, w = 8, h = 8},
    waterfall = {atlas = "overworld", src_x = 50, src_y = 0, w = 6, h = 4},
    river_origin = {atlas = "overworld", src_x = 56, src_y = 0, w = 10, h = 5},
    bridge_horizontal = {atlas = "overworld", src_x = 0, src_y = 10, w = 8, h = 4},
    bridge_vertical = {atlas = "overworld", src_x = 8, src_y = 8, w = 4, h = 10},
    bridge_broken_horizontal = {atlas = "overworld", src_x = 12, src_y = 8, w = 8, h = 4},
    bridge_broken_vertical = {atlas = "overworld", src_x = 20, src_y = 8, w = 4, h = 10},
    -- lake
    lake_boundary_corner_nw = {atlas = "overworld", src_x = 12, src_y = 12, w = 5, h = 5},
    lake_boundary_corner_ne = {atlas = "overworld", src_x = 1, src_y = 14, w = 5, h = 5},
    lake_boundary_corner_sw = {atlas = "overworld", src_x = 12, src_y = 13, w = 5, h = 5},
    lake_boundary_corner_se = {atlas = "overworld", src_x = 1, src_y = 16, w = 5, h = 5},
    lake_boundary_n = {atlas = "overworld", src_x = 13, src_y = 12, w = 5, h = 5},
    lake_boundary_w = {atlas = "overworld", src_x = 12, src_y = 13, w = 5, h = 5},
    lake_boundary_s = {atlas = "overworld", src_x = 0, src_y = 16, w = 5, h = 5},
    lake_boundary_e = {atlas = "overworld", src_x = 1, src_y = 15, w = 5, h = 5},
    lake_inner_corner_inverted_nw = {atlas = "overworld", src_x = 29, src_y = 3, w = 5, h = 5},
    lake_inner_corner_inverted_ne = {atlas = "overworld", src_x = 18, src_y = 3, w = 5, h = 5},
    lake_inner_corner_inverted_sw = {atlas = "overworld", src_x = 45, src_y = 0, w = 5, h = 5},
    lake_inner_corner_inverted_se = {atlas = "overworld", src_x = 34, src_y = 0, w = 5, h = 5},
    lake_inner_corner_nw = {atlas = "overworld", src_x = 26, src_y = 0, w = 5, h = 5},
    lake_inner_corner_ne = {atlas = "overworld", src_x = 21, src_y = 0, w = 5, h = 5},
    lake_inner_corner_sw = {atlas = "overworld", src_x = 42, src_y = 3, w = 5, h = 5},
    lake_inner_corner_se = {atlas = "overworld", src_x = 37, src_y = 3, w = 5, h = 5},
    lake_edge_straight_n = {atlas = "overworld", src_x = 8, src_y = 0, w = 5, h = 5},
    lake_edge_straight_s = {atlas = "overworld", src_x = 8, src_y = 3, w = 5, h = 5},
    lake_edge_straight_w = {atlas = "overworld", src_x = 0, src_y = 0, w = 5, h = 5},
    lake_edge_straight_e = {atlas = "overworld", src_x = 3, src_y = 0, w = 5, h = 5},
    -- lake edge is traversable if player has the boat
    lake_landing_bridge_w = {atlas = "overworld", src_x = 0, src_y = 10, w = 4, h = 4},
    lake_landing_bridge_e = {atlas = "overworld", src_x = 4, src_y = 10, w = 4, h = 4},
    lake_landing_bridge_n = {atlas = "overworld", src_x = 24, src_y = 8, w = 4, h = 4},
    lake_landing_bridge_s = {atlas = "overworld", src_x = 24, src_y = 12, w = 4, h = 5},
    -- field
    field_boundary_corner_nw = {atlas = "overworld", src_x = 39, src_y = 8, w = 5, h = 5},
    field_boundary_corner_ne = {atlas = "overworld", src_x = 44, src_y = 8, w = 5, h = 5},
    field_boundary_corner_sw = {atlas = "overworld", src_x = 39, src_y = 13, w = 5, h = 5},
    field_boundary_corner_se = {atlas = "overworld", src_x = 44, src_y = 13, w = 5, h = 5},
    field_boundary_n = {atlas = "overworld", src_x = 49, src_y = 9, w = 5, h = 5},
    field_boundary_s = {atlas = "overworld", src_x = 49, src_y = 8, w = 5, h = 5},
    field_boundary_w = {atlas = "overworld", src_x = 55, src_y = 8, w = 5, h = 5},
    field_boundary_e = {atlas = "overworld", src_x = 54, src_y = 8, w = 5, h = 5},
    -- mountain
    mountain_boundary_corner_nw = {atlas = "overworld", src_x = 66, src_y = 0, w = 5, h = 5},
    mountain_boundary_corner_ne = {atlas = "overworld", src_x = 71, src_y = 0, w = 5, h = 5},
    mountain_boundary_corner_sw = {atlas = "overworld", src_x = 66, src_y = 5, w = 5, h = 5},
    mountain_boundary_corner_se = {atlas = "overworld", src_x = 71, src_y = 5, w = 5, h = 5},
    mountain_boundary_n = {atlas = "overworld", src_x = 76, src_y = 5, w = 5, h = 5},
    -- mountain has no southern boundary bc it's always in the north of the map
    mountain_boundary_w = {atlas = "overworld", src_x = 77, src_y = 0, w = 5, h = 5},
    mountain_boundary_e = {atlas = "overworld", src_x = 76, src_y = 0, w = 5, h = 5},
    -- forest
    -- (a lot of metatiles are the same 5x5 tiles, just offset by half a tree)
    forest_inner_corner_nw = {atlas = "overworld", src_x = 34, src_y = 14, w = 5, h = 5},
    forest_inner_corner_ne = {atlas = "overworld", src_x = 34, src_y = 8, w = 5, h = 5},
    forest_inner_corner_sw = {atlas = "overworld", src_x = 34, src_y = 8, w = 5, h = 5},
    forest_inner_corner_se = {atlas = "overworld", src_x = 34, src_y = 14, w = 5, h = 5},
    forest_edge_straight_n = {atlas = "overworld", src_x = 34, src_y = 8, w = 5, h = 5},
    forest_edge_straight_s = {atlas = "overworld", src_x = 34, src_y = 13, w = 5, h = 6}, -- note that this has an extra row
    forest_edge_straight_w = {atlas = "overworld", src_x = 29, src_y = 15, w = 5, h = 5},
    forest_edge_straight_e = {atlas = "overworld", src_x = 28, src_y = 15, w = 5, h = 5},
    -- decorations
    cloud = {atlas = "overworld", src_x = 50, src_y = 4, w = 3, h = 2},
    tent = {atlas = "overworld", src_x = 53, src_y = 4, w = 2, h = 2},
    tree_small = {atlas = "overworld", src_x = 50, src_y = 6, w = 1, h = 2},
    tree_large = {atlas = "overworld", src_x = 51, src_y = 6, w = 2, h = 2},
}

-- helper functions that modify the tilemaps

local function merge_tile_layer(dst_layer, src_layer, dst_x, dst_y, src_x, src_y, w, h, src_map, dst_map)
    -- merges the tiles from one tilemap to the other
    -- src overwrites the dst, except when the tile index of src is 0
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

local function merge_object_layer(dst_layer, src_layer, dst_x, dst_y, src_x, src_y, w, h, tile_w, tile_h, dst_map)
    -- merges the object layers of the two tilemaps
    -- clips object dimensions to the cutout region (mostly used for collision objects)
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

local function basename(path)
    return path:match("([^/\\]+)$") or path
end

local function load_atlas(key)
    if atlas_cache[key] then
        return atlas_cache[key]
    end
    local path = atlases[key]
    if not path then
        error("ow_metatiles: unknown atlas key: " .. key)
    end
    local file = io.open(path, "r")
    if not file then
        error("ow_metatiles: could not open atlas: " .. path)
    end
    local map = json.decode(file:read("*a"))
    file:close()
    atlas_cache[key] = map
    return map
end

local function find_layer(map, name)
    for _, layer in ipairs(map.layers) do
        if layer.name == name then
            return layer
        end
    end
    return nil
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
            max_gid = ts.firstgid + TILESET_GID_GAP
        end
    end
    return max_gid
end

local function remap_gid(gid, src_tilesets, dst_tilesets)
    -- A remapping function in case the gids between the two tilesets don't match.
    -- This is the case when a tilemap has multiple tilesets. They might not be in the same order either.
    if gid == 0 then
        return 0
    end
    local src_ts, local_id = find_owning_tileset(src_tilesets, gid)
    if not src_ts then
        error("ow_metatiles: could not find owning tileset for gid " .. gid)
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
        dst_ts = { firstgid = next_available_firstgid(dst_tilesets), source = src_ts.source }
        table.insert(dst_tilesets, dst_ts)
    end
    return dst_ts.firstgid + local_id
end

function OW_Metatiles.place_metatiles(dst_map, list)
    -- batch placement
    -- list: array of { key, dst_x, dst_y }
    local dst_layers = {}
    for _, layer in ipairs(dst_map.layers) do
        dst_layers[layer.name] = layer
    end

    -- group by atlas to save iterations
    local by_atlas = {}
    for _, entry in ipairs(list) do
        local mt = metatiles[entry.key]
        if not mt then
            error("ow_metatiles: unknown metatile key: " .. entry.key)
        end
        if not by_atlas[mt.atlas] then
            by_atlas[mt.atlas] = {}
        end
        table.insert(by_atlas[mt.atlas], { mt = mt, dst_x = entry.dst_x, dst_y = entry.dst_y })
    end

    for atlas_key, entries in pairs(by_atlas) do
        local src_map = load_atlas(atlas_key)
        for _, src_layer in ipairs(src_map.layers) do
            local dst_layer = dst_layers[src_layer.name]
            if dst_layer then
                for _, entry in ipairs(entries) do
                    local mt = entry.mt
                    if src_layer.type == "tilelayer" then
                        merge_tile_layer(dst_layer, src_layer, entry.dst_x, entry.dst_y, mt.src_x, mt.src_y, mt.w, mt.h, src_map, dst_map)
                    elseif src_layer.type == "objectgroup" then
                        merge_object_layer(dst_layer, src_layer, entry.dst_x, entry.dst_y, mt.src_x, mt.src_y, mt.w, mt.h, src_map.tilewidth, src_map.tileheight, dst_map)
                    end
                end
            end
        end
    end
end

function OW_Metatiles.place_metatile(key, dst_map, dst_x, dst_y)
    -- wrapper for a single metatile
    OW_Metatiles.place_metatiles(dst_map, { { key = key, dst_x = dst_x, dst_y = dst_y } })
end

return OW_Metatiles