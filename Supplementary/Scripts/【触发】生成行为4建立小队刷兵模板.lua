-- 生成建立小队刷兵模板.lua

-- ========== 自动创建快照 ==========
create_snapshot()

-- ========== 获取所有国家列表 ==========

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

-- ========== 获取所有单位列表 ==========

local all_units = {}
for k, v in pairs(get_values("InfantryTypes", "rules+map")) do
    if v ~= "" then table.insert(all_units, {id = v, name = get_uiname(v), cat = "步兵"}) end
end
for k, v in pairs(get_values("VehicleTypes", "rules+map")) do
    if v ~= "" then table.insert(all_units, {id = v, name = get_uiname(v), cat = "车辆"}) end
end
for k, v in pairs(get_values("AircraftTypes", "rules+map")) do
    if v ~= "" then table.insert(all_units, {id = v, name = get_uiname(v), cat = "飞行器"}) end
end
table.sort(all_units, function(a, b)
    if a.cat ~= b.cat then return a.cat < b.cat end
    return a.name < b.name
end)

local unit_items = {}
for _, u in ipairs(all_units) do
    table.insert(unit_items, "[" .. u.cat .. "] " .. u.name .. " (" .. u.id .. ")")
end

-- ========== 创建对话框 ==========

local dlg = LuaDialog:new("刷兵模板生成器", false, 580, 320)
local y = 10

-- 所属方选择
dlg:add_label("lbl_house", "所属方", 10, y + 18, 50, 20)
local country_items = {}
for _, c in ipairs(countries) do
    table.insert(country_items, c.value)
end
dlg:add_combobox("house", "", country_items, country_items[1] or "", true, 65, y, 490, 22)

-- 单位列标题
y = y + 28
dlg:add_label("col_idx", "#", 15, y + 18, 20, 16)
dlg:add_label("col_num", "数量", 40, y + 18, 40, 16)
dlg:add_label("col_unit", "单位类型（支持搜索）", 90, y + 18, 460, 16)

-- 6个单位输入行
for i = 1, 6 do
    y = y + 24
    dlg:add_label("idx_" .. i, tostring(i) .. ".", 15, y + 18, 20, 22)
    dlg:add_edit("num_" .. i, "", "", 40, y, 40, 22)
    dlg:add_combobox("unit_" .. i, "", unit_items, "", false, 90, y, 460, 22)
end

-- 重复类型 + 间隔
y = y + 24 + 8
dlg:add_label("lbl_repeat", "重复类型", 10, y + 18, 60, 22)
local repeat_items = {"0 - 任一满足，单次", "1 - 所有满足，单次", "2 - 任一满足，重复"}
dlg:add_combobox("repeat_type", "", repeat_items, repeat_items[1], true, 75, y, 180, 22)

dlg:add_label("lbl_interval", "间隔(秒)", 290, y + 18, 65, 22)
dlg:add_edit("interval", "", "220", 355, y, 60, 22)

-- ========== 显示对话框 ==========

local result = dlg:do_modal()
if not result then
    print("用户取消了操作，脚本已中止")
    return
end

-- ========== 解析输入 ==========

-- 解析所属方
local selected_value = result.house
local selected_house = ""
for _, c in ipairs(countries) do
    if c.value == selected_value then
        selected_house = c.key
        break
    end
end
if selected_house == "" then
    print("未选择有效的所属方，脚本已中止")
    return
end

-- 解析单位输入
local parsed_units = {}
for i = 1, 6 do
    local num_str = (result["num_" .. i] or ""):match("^%s*(.-)%s*$") or ""
    local unit_str = (result["unit_" .. i] or ""):match("^%s*(.-)%s*$") or ""
    if num_str ~= "" and unit_str ~= "" then
        local num = tonumber(num_str)
        local id = unit_str:match("%((.+)%)$")
        if num and num > 0 and id then
            table.insert(parsed_units, {number = num, id = id})
            print(string.format("  %d) 数量: %d | 单位: %s", i, num, id))
        end
    end
end

if #parsed_units == 0 then
    print("未输入任何有效的特遣部队，脚本已中止")
    return
end

-- 解析重复类型
local repeat_type_str = result.repeat_type or ""
local repeat_value = 0
if repeat_type_str:find("1") then
    repeat_value = 1
elseif repeat_type_str:find("2") then
    repeat_value = 2
end

-- 解析间隔
local interval_str = (result.interval or ""):match("^%s*(.-)%s*$") or ""
local interval = tonumber(interval_str)
if not interval or interval < 0 then
    print("刷兵间隔无效，使用默认值 220")
    interval = 220
end

-- ========== 创建特遣部队、脚本、小队、触发 ==========

local name_parts = {}
for _, pu in ipairs(parsed_units) do
    table.insert(name_parts, pu.number .. pu.id)
end
local name_suffix = table.concat(name_parts, "")
local base_name = selected_house .. "_" .. name_suffix
local trigger_name = "[刷兵]" .. base_name
if repeat_value == 2 then
    trigger_name = trigger_name .. "_重复"
end

-- 创建特遣部队
local task = task_force:new()
task.name = base_name
for _, pu in ipairs(parsed_units) do
    task:add_number(pu.number, pu.id)
end
task:apply()

-- 创建脚本（0-0：攻击移动）
local s = script:new()
s.name = base_name
s:add_action(0, 0)
s:apply()

-- 创建小队
local t = team:new()
t.name = base_name
t.country = selected_house
t.task_force = task.id
t.script = s.id
t.recruiter = true
t:apply()

-- 创建触发
local trig = trigger:new()
trig.name = trigger_name
trig.country = "Neutral"
trig.easy = true
trig.medium = true
trig.hard = true
trig.disabled = true

trig:add_event("13,0," .. interval)
trig:add_action("4,1," .. t.id .. ",0,0,0,0,A")
trig:add_tag("", trigger_name, repeat_value)
trig:apply()

-- ========== 输出结果 ==========

print("=======================")
print("刷兵触发已生成：")
print("  特遣部队: " .. task.name .. " (ID: " .. task.id .. ")")
print("  脚本: " .. s.name .. " (ID: " .. s.id .. ")")
print("  小队: " .. t.name .. " (ID: " .. t.id .. ")")
print("  触发: " .. trig.name .. " (ID: " .. trig.id .. ")")
print("-----------------------")
if repeat_value == 0 then print("重复类型: 任一满足，单次")
elseif repeat_value == 1 then print("重复类型: 所有满足，单次")
else print("重复类型: 任一满足，重复") end
print("刷兵间隔: " .. interval .. " 秒")
print("脚本行为：0-0（攻击移动）")
print("注意：触发默认禁用，请手动允许触发")
print("=======================")

message_box(string.format("刷兵触发已成功生成！\n小队: %s\n触发: %s\n\n触发默认禁用，请手动允许触发",
    t.name, trig.name), "执行成功", 1)