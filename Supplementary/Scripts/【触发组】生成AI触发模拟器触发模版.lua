-- 生成AI触发模拟器_触发模版.lua

-- ========== 自动创建快照 ==========

create_snapshot()

-- ========== 选择所属方 ==========

local dlg = LuaDialog:new("AI触发模拟器生成", false, 380, 110)

local countries = {}
for i, house in pairs(get_values("Countries", "rules+map")) do
    table.insert(countries, {key = house, value = translate_house(house) .. " (" .. house .. ")"})
end
if is_multiplay() then
    for i = 0, 7 do
        local letter = string.char(string.byte("A") + i)
        table.insert(countries, {key = "<Player @ " .. letter .. ">", value = "<Player @ " .. letter .. ">"})
    end
end

local country_items = {}
for _, c in ipairs(countries) do
    table.insert(country_items, c.value)
end

dlg:add_label("lbl_house", "所属方", 10, 24, 50, 20)
dlg:add_combobox("house", "", country_items, country_items[1] or "", true, 65, 6, 280, 22)

local result = dlg:do_modal()
if not result then
    print("用户取消了操作，脚本已中止")
    return
end

local selected_house = ""
for _, c in ipairs(countries) do
    if c.value == result.house then
        selected_house = c.key
        break
    end
end
if selected_house == "" then
    print("未选择有效的所属方，脚本已中止")
    return
end

-- ========== 根据阵营确定建筑索引 ==========

-- 注意：这些是原版尤里的复仇建筑列表索引，请仔细核对
local side_str = get_string(selected_house, "Side", "0", "rules+map")
local building_indices
if side_str == "GDI" then
    building_indices = {infantry = 3, vehicle = 7, navy = 23, airforce = 105}
elseif side_str == "Nod" then
    building_indices = {infantry = 11, vehicle = 14, navy = 61, airforce = 105}
elseif side_str == "ThirdSide" then
    building_indices = {infantry = 302, vehicle = 303, navy = 304, airforce = 105}
else
    building_indices = {infantry = 3, vehicle = 7, navy = 23, airforce = 105}
end

print(string.format("所属方: %s (阵营: %s)", selected_house, side_str))
print(string.format("建筑索引: 步兵=%d 载具=%d 海军=%d 空军=%d",
    building_indices.infantry, building_indices.vehicle,
    building_indices.navy, building_indices.airforce))

-- ========== 创建4组 on/off 触发 ==========

local unit_types = {
    {key = "Infantry", name = "步兵",   building = building_indices.infantry},
    {key = "Vehicle",  name = "载具",   building = building_indices.vehicle},
    {key = "Navy",     name = "海军",   building = building_indices.navy},
    {key = "Airforce", name = "空军",   building = building_indices.airforce},
}

print("=======================")

for _, ut in ipairs(unit_types) do
    -- 创建变量
    local var_name = "[AIS]" .. selected_house .. "_" .. ut.key
    local variable_index = tonumber(get_free_key("VariableNames"))
    write_string("VariableNames", tostring(variable_index), var_name .. ",0")

    -- on 触发：建筑存在 且 变量=0 → 设置变量=1
    local on = trigger:new()
    on.name = "[AI触发模拟]" .. selected_house .. "_" .. ut.name .. "_on"
    on.country = selected_house
    on.easy = true; on.medium = true; on.hard = true; on.disabled = false
    on:add_event("32,0," .. ut.building)   -- 32: 建筑存在
    on:add_event("37,0," .. variable_index) -- 37: 变量>0
    on:add_action("56,0," .. variable_index .. ",0,0,0,0,A")  -- 56: 设置变量=1
    on:add_tag("", on.name .. " 1", 2)
    on:apply()

    -- off 触发：建筑摧毁 且 变量>0 → 设置变量=0
    local off = trigger:new()
    off.name = "[AI触发模拟]" .. selected_house .. "_" .. ut.name .. "_off"
    off.country = selected_house
    off.easy = true; off.medium = true; off.hard = true; off.disabled = false
    off:add_event("57,0," .. ut.building)   -- 57: 建筑不存在/摧毁
    off:add_event("36,0," .. variable_index) -- 36: 变量==0
    off:add_action("57,0," .. variable_index .. ",0,0,0,0,A")  -- 57: 设置变量=0
    off:add_tag("", off.name .. " 1", 2)
    off:apply()

    print(string.format("  √ [%s/%s] %s 变量[%d]=%s",
        on.id, off.id, ut.name, variable_index, var_name))
end

-- ========== 输出结果 ==========

print("=======================")
print("AI触发模拟器已生成：")
print("  为 " .. selected_house .. " 创建了 4 组 (8 个) 触发")
print("-----------------------")
print("使用说明：")
print("  在 AI触发 的条件中引用对应变量，即可模拟")
print("  建筑存在时 → 对应变量 = 1")
print("  建筑被摧毁 → 对应变量 = 0")
print("-----------------------")
print("注意：建筑索引基于原版尤里的复仇，")
print("若使用 mod 请自行核对索引！")
print("=======================")

message_box(string.format("AI触发模拟器已成功生成！\n为 %s 创建了 4 组触发\n\n建筑索引:\n步兵=%d 载具=%d 海军=%d 空军=%d\n\n请仔细核对建筑索引！",
    selected_house,
    building_indices.infantry, building_indices.vehicle,
    building_indices.navy, building_indices.airforce), "执行成功", 1)