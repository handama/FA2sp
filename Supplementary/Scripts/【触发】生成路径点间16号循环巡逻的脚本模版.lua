-- 生成路径点间16号循环巡逻脚本模版.lua

-- ========== 自动创建快照 ==========
create_snapshot()

-- ========== 获取地图已有路径点 ==========

local existing_waypoints = {}
local wp_keys = get_keys("Waypoints", "map")
for _, key in ipairs(wp_keys) do
    local num = tonumber(key)
    if num then
        table.insert(existing_waypoints, num)
    end
end
table.sort(existing_waypoints)

local wp_items = {}
for _, n in ipairs(existing_waypoints) do
    table.insert(wp_items, "路径点 #" .. n .. " (" .. waypoint_to_string(n) .. ")")
end

-- ========== 创建对话框 ==========

local dlg = LuaDialog:new("循环巡逻脚本生成器", false, 580, 300)
local y = 10

-- 路径点选择（多选列表框）
dlg:add_label("lbl_wp", "巡逻路径点（Ctrl+点击多选，2~13个）", 10, y + 18, 300, 20)
if #wp_items > 0 then
    dlg:add_multilistbox("waypoints", "", wp_items, 10, y + 22, 540, 150)
else
    dlg:add_label("lbl_no_wp", "⚠ 地图中暂无路径点，请先放置路径点", 10, y + 22, 400, 20)
end

-- 警戒时间
y = y + 22 + 156
dlg:add_label("lbl_guard", "警戒时间（秒）", 10, y + 18, 100, 22)
dlg:add_edit("guard", "", "5", 110, y, 60, 22)

-- ========== 显示对话框 ==========

local result = dlg:do_modal()
if not result then
    print("用户取消了操作，脚本已中止")
    return
end

-- ========== 解析输入 ==========

-- 解析路径点
local selected_wp_values = result.waypoints or {}
local wp_list = {}
local function extract_wp_num(str)
    if type(str) == "string" then
        local num = str:match("#(%d+)")
        if num then return tonumber(num) end
    end
    return nil
end
if type(selected_wp_values) == "table" then
    for _, val in ipairs(selected_wp_values) do
        local num = extract_wp_num(val)
        if num then table.insert(wp_list, num) end
    end
elseif type(selected_wp_values) == "string" and selected_wp_values ~= "" then
    local num = extract_wp_num(selected_wp_values)
    if num then table.insert(wp_list, num) end
end

if #wp_list < 2 then
    print("至少选择2个路径点才能生成巡逻路线，脚本已中止")
    return
end

if #wp_list > 13 then
    print("路径点过多（最多13个），脚本已中止")
    return
end

-- 解析警戒时间
local guard_str = (result.guard or ""):match("^%s*(.-)%s*$") or ""
local guard = tonumber(guard_str)
if not guard or guard < 0 then
    print("警戒时间无效，使用默认值 5 秒")
    guard = 5
end

-- ========== 创建巡逻脚本 ==========

-- 生成名称：Patrol #A-B-C-...
local name_parts = {}
for _, wp in ipairs(wp_list) do
    table.insert(name_parts, waypoint_to_string(wp))
end
local patrol_name = "Patrol #" .. table.concat(name_parts, "-")

local s = script:new()
s.name = patrol_name

-- 正向：起点 → 终点
for _, wp in ipairs(wp_list) do
    s:add_action(16, wp)   -- 移动到路径点
    s:add_action(5, guard) -- 警戒停留
end

-- 反向：终点 → 起点（跳过首尾）
for i = #wp_list - 1, 2, -1 do
    s:add_action(16, wp_list[i])
    s:add_action(5, guard)
end

-- 循环
s:add_action(6, 1)

s:apply()

-- ========== 输出结果 ==========

print("=======================")
print("循环巡逻脚本已生成：")
print("  名称: " .. s.name)
print("  ID: " .. s.id)
print("-----------------------")
print("巡逻路径（正向 → 反向 → 循环）：")
for _, wp in ipairs(wp_list) do
    print("  → 路径点 #" .. waypoint_to_string(wp) .. " (" .. wp .. ")  [移动+警戒" .. guard .. "秒]")
end
for i = #wp_list - 1, 2, -1 do
    print("  ← 路径点 #" .. waypoint_to_string(wp_list[i]) .. " (" .. wp_list[i] .. ")  [移动+警戒" .. guard .. "秒]")
end
print("  ↩ 循环")
print("-----------------------")
print(string.format("共 %d 个巡逻点，警戒 %d 秒", #wp_list, guard))
print("脚本行为：16（移动）+ 5（警戒）+ 6（循环）")
print("=======================")

message_box(string.format("循环巡逻脚本已成功生成！\n名称: %s\n路径点: %d 个\n警戒时间: %d 秒\n\n请将脚本分配给作战小队",
    s.name, #wp_list, guard), "执行成功", 1)