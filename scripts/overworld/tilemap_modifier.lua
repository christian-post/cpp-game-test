local TilemapModifier = {}

-- TODO maybe this has enough common code to be inside of the other tilemap_modifier script

-- base tiles. the name is [zone_name] ... _ ... doors (connections to adjacent rooms)
local baseTilemaps = {
    field = "fields_empty",
    forest = "forest_middle",
    lake = "lake_middle",
    mountain = "mountain_middle"
    -- "town" has no base tile, it's always the same tilemap
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


function TilemapModifier.process_overworld()

    -- TODO stitch tilemaps together based on the entrance config
    -- example:
    -- fields_0011 gets walled off to the right and up
    -- Walls are chosen based on the adjacent zone (trees, rock or water)

    -- TODO just a quick test
    local tilemap_src_inner = TilemapModifier.load("resources/tilemaps/base/overworld/forest_base_inner_corners.json")
    local tilemap_src_outer = TilemapModifier.load("resources/tilemaps/base/overworld/forest_base_outer_corners.json")
    local tilemap_dst = TilemapModifier.load("resources/tilemaps/base/overworld/fields_empty.json")

    if tilemap_src_inner == nil or tilemap_src_outer == nil or tilemap_dst == nil then
        print("source or destination is nil")
        return
    end

    local w = 20
    local h = 20

    -- top half: copy inner corners
    local src_x = 0
    local src_y = 0
    TilemapModifier.copy_region(tilemap_dst, tilemap_src_inner, 0, 0, src_x, src_y, w, h)
    TilemapModifier.copy_region(tilemap_dst, tilemap_src_inner, 20, 0, src_x + w, src_y, w, h)
    -- bottom half: copy outer corners
    TilemapModifier.copy_region(tilemap_dst, tilemap_src_outer, 0, 20, src_x, src_y, w, h)
    TilemapModifier.copy_region(tilemap_dst, tilemap_src_outer, 20, 20, src_x + w, src_y, w, h)

    TilemapModifier.save(tilemap_dst, "resources/tilemaps/fields_test_output.json")
    print("done")

end

return TilemapModifier