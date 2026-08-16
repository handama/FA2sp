-- 建筑围墙 → 覆盖物围墙 转换脚本
-- 功能：把地图上以"建筑"形式存在的围墙（如 GAWALL、NAWAL 等）转换为"覆盖物"形式的围墙。
-- 修改前已保存撤销点，执行后可用 Ctrl+Z 回退。

-- ====== 1. 读取有序注册表（rules+map） ======
local building_types = get_ordered_values("BuildingTypes", "rules+map")
local overlay_types  = get_ordered_values("OverlayTypes",  "rules+map")

-- ====== 2. 找出同名类型，记录覆盖物有序索引 ======
local building_set = {}
for _, id in pairs(building_types) do
    building_set[id] = true
end

local wall_overlay = {}   -- 类型ID -> 覆盖物索引
for idx, id in pairs(overlay_types) do
    if building_set[id] then
        wall_overlay[id] = idx
    end
end

if next(wall_overlay) == nil then
    message_box("在 BuildingTypes 与 OverlayTypes 中没有找到同名的类型，无法转换。", "提示", 8)
    end_script()
end

-- ====== 3. 统计地图上的围墙建筑并确认 ======
local buildings = get_buildings()
local wall_count = 0
for _, b in ipairs(buildings) do
    if wall_overlay[b.type] ~= nil then
        wall_count = wall_count + 1
    end
end

if wall_count == 0 then
    print("地图上没有检测到建筑围墙。")
    end_script()
end

-- ====== 修改前保存撤销点 ======
save_undo_all()

-- ====== 4. 倒序删除围墙建筑，记录位置与对应覆盖物索引 ======
local walls = {}
for i = #buildings, 1, -1 do
    local b = buildings[i]
    local ovl = wall_overlay[b.type]
    if ovl ~= nil then
        table.insert(walls, {
            x       = b.x,
            y       = b.y,
            overlay = ovl,
        })
        remove_building(i - 1)   -- 0 基索引
    end
end

-- ====== 5. 用 place_wall 在对应位置放置覆盖物围墙 ======
for i, w in ipairs(walls) do
    place_wall(w.x, w.y, w.overlay, 1)
    if i % 200 == 0 then
        avoid_time_out()
    end
end

-- ====== 刷新与汇总 ======
update_building()
update_overlay()
redraw_window()

print(string.format("完成：已将 %d 座建筑围墙转换为覆盖物围墙。", #walls))
for id, idx in pairs(wall_overlay) do
    print(string.format("  类型 %s → 覆盖物索引 %d", id, idx))
end
print("如需回退，请使用 Ctrl+Z。")
