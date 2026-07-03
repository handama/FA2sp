-- 生成循环招募指定单位的触发模板.lua

-- ========== 自动创建快照 ==========

create_snapshot()

-- ========== 获取数据 ==========

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

-- 合并所有单位类型（步兵+车辆+飞行器）
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

-- ========== 对话框 ==========

local dlg = LuaDialog:new("循环招募生成器", false, 520, 120)

-- 所属方
dlg:add_label("lbl_house", "所属方", 10, 28, 50, 20)
local country_items = {}
for _, c in ipairs(countries) do
    table.insert(country_items, c.value)
end
dlg:add_combobox("house", "", country_items, country_items[1] or "", true, 65, 10, 180, 22)

-- 单位
dlg:add_label("lbl_unit", "招募单位", 270, 28, 70, 20)
dlg:add_combobox("unit", "", unit_items, "", false, 340, 10, 160, 22)

local result = dlg:do_modal()
if not result then
    print("用户取消了操作，脚本已中止")
    return
end

-- ========== 解析输入 ==========

local selected_house = ""
local selected_value = result.house
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

local unit_str = result.unit or ""
local selected_unit = unit_str:match("%((.+)%)$")
if not selected_unit then
    print("未选择有效的单位，脚本已中止")
    return
end

local name = "[LoopRecruit]" .. selected_house .. "_" .. selected_unit
print(string.format("所属方: %s | 单位: %s (%s)", selected_house, get_uiname(selected_unit), selected_unit))

-- ========== 创建变量 ==========

local variable_index = tonumber(get_free_key("VariableNames"))
write_string("VariableNames", tostring(variable_index), name .. ",0")
print(string.format("变量 [%d] %s", variable_index, name))

-- ========== 创建特遣部队、脚本、小队 ==========

local task = task_force:new()
task.name = name
task:add_number(1, selected_unit)
task:apply()

local s = script:new()
s.name = name
s:add_action(39, variable_index)  -- 39: 设置变量为0（清空招募标记）
s:add_action(31, 0)               -- 31: 招募单位
s:apply()

local t = team:new()
t.name = name
t.country = selected_house
t.task_force = task.id
t.script = s.id
t.recruiter = true
t:apply()

-- ========== 创建触发 ==========

local trig = trigger:new()
trig.name = name
trig.country = "Neutral"
trig.easy = true
trig.medium = true
trig.hard = true
trig.disabled = false

-- 事件 36: 变量 > 0 时触发（变量被设置为1时启动）
trig:add_event("36,0," .. variable_index)

-- Action 57: 将变量重置为0
trig:add_action("57,0," .. variable_index .. ",0,0,0,0,A")
-- Action 4: 建立小队
trig:add_action("4,1," .. t.id .. ",0,0,0,0,A")
trig:add_tag("", name .. " 1", 2)  -- 重复类型 2（循环）
trig:apply()

-- ========== 输出结果 ==========

print("=======================")
print("循环招募已生成：")
print(string.format("  特遣部队: %s (ID: %s)", task.name, task.id))
print(string.format("  脚本: %s (ID: %s)", s.name, s.id))
print(string.format("  小队: %s (ID: %s)", t.name, t.id))
print(string.format("  触发: %s (ID: %s)", trig.name, trig.id))
print("-----------------------")
print("使用说明：")
print(string.format("  将局部变量 [%d] %s", variable_index, name))
print("  设置为 1 即可启动循环招募")
print("  设置为 0 即可停止")
print("=======================")

message_box(string.format("循环招募已成功生成！\n\n将局部变量\n[%d] %s\n设置为 1 即可启动，0 即可停止。",
    variable_index, name), "执行成功", 1)
