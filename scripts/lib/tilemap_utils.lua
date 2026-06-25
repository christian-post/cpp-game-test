local utils = require("lib.utils")
local TilemapUtils = {}

local TILESET_GID_GAP = 1000 -- arbitrary gid difference between two tilesets that are used in the same tilemap

function TilemapUtils.find_layer(map, name)
    for _, layer in ipairs(map.layers) do
        if layer.name == name then
            return layer
        end
    end
    return nil
end

function TilemapUtils.next_available_firstgid(tilesets)
    local max_gid = 1
    for _, ts in ipairs(tilesets) do
        if ts.firstgid >= max_gid then
            max_gid = ts.firstgid + TILESET_GID_GAP
        end
    end
    return max_gid
end

function TilemapUtils.find_owning_tileset(tilesets, gid)
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

function TilemapUtils.remap_gid(gid, src_tilesets, dst_tilesets)
    -- remaps a gid in case the gids between the two tilesets don't match.
    -- this happens when a tilemap has multiple tilesets, possibly in different order.
    if gid == 0 then
        return 0
    end
    local src_ts, local_id = TilemapUtils.find_owning_tileset(src_tilesets, gid)
    if not src_ts then
        error("tilemap_utils: could not find owning tileset for gid " .. gid)
    end
    local src_name = utils.basename(src_ts.source)
    local dst_ts = nil
    for _, ts in ipairs(dst_tilesets) do
        if utils.basename(ts.source) == src_name then
            dst_ts = ts
            break
        end
    end
    if not dst_ts then
        dst_ts = { firstgid = TilemapUtils.next_available_firstgid(dst_tilesets), source = src_ts.source }
        table.insert(dst_tilesets, dst_ts)
    end
    return dst_ts.firstgid + local_id
end

function TilemapUtils.merge_tile_layer(dst_layer, src_layer, dst_x, dst_y, src_x, src_y, w, h, src_map, dst_map, overwrite)
    -- merges the tiles from one tilemap to the other
    -- src overwrites the dst, except when the tile index of src is 0
    for row = 0, h - 1 do
        for col = 0, w - 1 do
            assert(dst_x + col < dst_map.width and dst_y + row < dst_map.height, "merge_tile_layer: metatile placement out of bounds at dst (" .. dst_x + col .. ", " .. dst_y + row .. ")")
            local src_index = (src_y + row) * src_map.width + (src_x + col) + 1
            local dst_index = (dst_y + row) * dst_map.width + (dst_x + col) + 1
            local src_tile = src_layer.data[src_index]
            local dst_tile = dst_layer.data[dst_index]
            if overwrite then
                dst_layer.data[dst_index] = TilemapUtils.remap_gid(src_tile or 0, src_map.tilesets, dst_map.tilesets)
            elseif src_tile and src_tile ~= 0 then
                dst_layer.data[dst_index] = TilemapUtils.remap_gid(src_tile, src_map.tilesets, dst_map.tilesets)
            elseif dst_tile == nil then
                dst_layer.data[dst_index] = 0
            end
        end
    end
end

function TilemapUtils.clip_object(obj, rx, ry, rx2, ry2)
    local ox2 = obj.x + obj.width
    local oy2 = obj.y + obj.height
    if obj.x >= rx2 or ox2 <= rx or obj.y >= ry2 or oy2 <= ry then
        return nil
    end
    local clipped = {}
    for k, v in pairs(obj) do
        clipped[k] = v
    end
    clipped.x = math.max(obj.x, rx)
    clipped.y = math.max(obj.y, ry)
    clipped.width = math.min(ox2, rx2) - clipped.x
    clipped.height = math.min(oy2, ry2) - clipped.y
    return clipped
end

function TilemapUtils.merge_object_layer(dst_layer, src_layer, dst_x, dst_y, src_x, src_y, w, h, tile_w, tile_h, dst_map)
    -- merges the object layers of the two tilemaps
    -- clips object dimensions to the cutout region (mostly used for collision objects)
    local rx = src_x * tile_w
    local ry = src_y * tile_h
    local rx2 = (src_x + w) * tile_w
    local ry2 = (src_y + h) * tile_h
    local offset_x = (dst_x - src_x) * tile_w
    local offset_y = (dst_y - src_y) * tile_h
    for _, obj in ipairs(src_layer.objects) do
        local clipped = TilemapUtils.clip_object(obj, rx, ry, rx2, ry2)
        if clipped ~= nil then
            clipped.x = clipped.x + offset_x
            clipped.y = clipped.y + offset_y
            clipped.id = dst_map.nextobjectid
            dst_map.nextobjectid = dst_map.nextobjectid + 1
            table.insert(dst_layer.objects, clipped)
        end
    end
end

function TilemapUtils.copy_region(dst_map, src_map, dst_x, dst_y, src_x, src_y, w, h, overwrite)
    local dst_layers = {}
    for _, layer in ipairs(dst_map.layers) do
        dst_layers[layer.name] = layer
    end
    for _, src_layer in ipairs(src_map.layers) do
        local dst_layer = dst_layers[src_layer.name]
        if dst_layer then
            if src_layer.type == "tilelayer" then
                -- floor is never cleared, only nonzero tiles written, so the base terrain stays intact
                local layer_overwrite = src_layer.name ~= "floor" and overwrite or false
                TilemapUtils.merge_tile_layer(dst_layer, src_layer, dst_x, dst_y, src_x, src_y, w, h, src_map, dst_map, layer_overwrite)
            elseif src_layer.type == "objectgroup" then
                TilemapUtils.merge_object_layer(dst_layer, src_layer, dst_x, dst_y, src_x, src_y, w, h, src_map.tilewidth, src_map.tileheight, dst_map)
            end
        end
    end
end

return TilemapUtils